// Copyright (c) 2014 The Memorycoin developers
#include "voting.h"
#include "base58.h"
#include "utilstrencodings.h"
#include "chain.h"
#include "main.h"
#include "activebanknode.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>

#include <sys/stat.h>
#include <stdio.h>

using namespace boost;
using namespace std;

CCriticalSection grantdb;
//SECTION: GrantPrefixes and Grant Block Intervals
static string GRANTPREFIX="6BCR";
static const int64_t GRANTBLOCKINTERVAL = 5;
static int numberOfOffices = 5;
string electedOffices[6];

//= {"cdo","cto","cso","bnk","nsr",cmo, "XFT"};
//Implement in memory for now - this will cause slow startup as recalculation of all votes takes place every startup. These should be persisted in a database or on disk
CBlockIndex* gdBlockPointer = NULL;
int64_t grantDatabaseBlockHeight=-1; //How many blocks processed for grant allocation purposes
ofstream grantAwardsOutput;
bool debugVote = false;
bool debugVoteExtra = false;
int64_t numberCandidatesEliminated = 0;
std::map<int, std::string > awardWinners;
std::map<std::string,int64_t > grantAwards;
std::map<std::string,int64_t>::iterator gait;
std::map<std::string,int64_t > preferenceCount;
std::map<std::string,int64_t> balances; //Balances as at grant allocation block point
std::map<std::string,std::map<int64_t,std::string> > votingPreferences[7]; //Voting prefs as at grant allocation block point
std::map<std::string,std::map<int64_t, std::string> >::iterator ballotit;
std::map<std::string,std::map<int64_t, std::string> > ballots;
std::map<std::string,int64_t > ballotBalances;
std::map<std::string,double > ballotWeights;
std::map<int64_t, std::string>::iterator svpit;
std::map<int64_t, std::string>::iterator svpit2;
std::map<std::string, int64_t>::iterator svpit3;
std::map<int64_t, std::string>::iterator svpit4;
std::map<std::string,int64_t>::iterator it;
std::map<int64_t,std::string>::iterator it2;
std::map<std::string,std::map<int64_t,std::string> >::iterator vpit;
std::map<std::string,int64_t > wastedVotes; //Report on Votes that were wasted
std::map<std::string,std::map<int64_t,std::string> > electedVotes; //Report on where votes went
std::map<std::string,std::map<int64_t,std::string> > supportVotes; //Report on support for candidates

bool isGrantAwardBlock(int64_t nHeight){
	if (nHeight > 210000 && (nHeight % 5 == 0)){
		if(fDebug)LogPrintf("  Is (%ld) a grant block? : Yes \n", nHeight);
		return true;
	}
	return false;
}


void serializeGrantDB(string filename){

		if(fDebug)LogPrintf(" Serialize Grant Info Database: Current Grant Database Block Height: %ld\n",grantDatabaseBlockHeight);

		ofstream grantdb;
		grantdb.open (filename.c_str(), ios::trunc);
		
		//grantDatabaseBlockHeight
		grantdb << grantDatabaseBlockHeight << "\n";
		
		//Balances
		grantdb << balances.size()<< "\n";
		for(it=balances.begin(); it!=balances.end(); ++it){
			grantdb << it->first << "\n" << it->second<< "\n";
		}
		
		//votingPreferences
        for(int i=0;i<numberOfOffices;i++){
            grantdb << votingPreferences[i].size()<< "\n";
            for(vpit=votingPreferences[i].begin(); vpit!=votingPreferences[i].end(); ++vpit){
                grantdb << vpit->first << "\n";
                grantdb << vpit->second.size() << "\n";
                for(it2=vpit->second.begin();it2!=vpit->second.end();++it2){
                    grantdb << it2->first << "\n" << it2->second<< "\n";
                }
            }
        }

		grantdb.flush();
		grantdb.close();
}

bool deSerializeGrantDB(string filename, int64_t maxWanted){

	if(fDebug)LogPrintf(" De-Serialize Grant Info Database\n Max Blocks Wanted: %ld\n", maxWanted);

	std::string line;
	std::string line2;
	ifstream myfile;
	
	myfile.open (filename.c_str());	

	if (myfile.is_open()){
		getline (myfile,line);
		grantDatabaseBlockHeight=atoi64(line.c_str());
		if(fDebug)LogPrintf("Deserialize Grant Info Database Found Height %ld\n",grantDatabaseBlockHeight);
		
        if(grantDatabaseBlockHeight>maxWanted){
            //vote database later than required - don't load
            grantDatabaseBlockHeight=-1;
            myfile.close();
            return false;
        }

		//Balances
		balances.clear();
		getline (myfile,line);
		int64_t balancesSize=atoi64(line.c_str());
		for(int i=0;i<balancesSize;i++){
			getline(myfile, line );
			getline(myfile, line2 );
			balances[line]=atoi64(line2.c_str());
		}
		
		//votingPreferences
        for(int i=0;i<numberOfOffices;i++){
            votingPreferences[i].clear();
            getline(myfile,line);
            int64_t votingPreferencesSize=atoi64(line.c_str());
            for(int k=0;k<votingPreferencesSize;k++){
                getline(myfile,line);
                std::string vpAddress=line;
                getline(myfile,line);
                int64_t vpAddressSize=atoi64(line.c_str());
			
                for(int j=0;j<vpAddressSize;j++){
                    getline(myfile,line);
                    getline(myfile,line2);
                    votingPreferences[i][vpAddress][atoi64(line.c_str())]=line2;
                }
            }
        }

		myfile.close();
		//Set the pointer to next block to process
		gdBlockPointer=chainActive.Genesis();
		for(int i=0;i<grantDatabaseBlockHeight;i++){
            if(gdBlockPointer == NULL ){
				LogPrintf("Insufficent number of blocks loaded %s\n",filename.c_str());
				return false;
			}
			gdBlockPointer=chainActive.Tip();
		}
		return true;
	}
	return 0;
}

bool getGrantAwards(int64_t nHeight){
	//nHeight is the current block height
	if(!isGrantAwardBlock(nHeight)){
		if(fDebug)LogPrintf("Error - calling getgrantawards for non grant award block, nHeight requested: %ld", nHeight);
		return false;
	}
	return ensureGrantDatabaseUptoDate(nHeight);
}

bool ensureGrantDatabaseUptoDate(int64_t nHeight){

	if(fDebug)LogPrintf(" === Grant Database Block Height %ld === \n", grantDatabaseBlockHeight);
    //This should always be true on startup
    if(grantDatabaseBlockHeight==-1){
		string newCV=GetArg("-custombankprefix","vte");
		electedOffices[0] = "dof";
		electedOffices[1] = "tof";
		electedOffices[2] = "sof";
		electedOffices[3] = "mof";
		electedOffices[4] = "bnk";
		electedOffices[5] = newCV;
    }

    //NOTE: requiredgrantdatabaseheight is 5 less than the current block
	int64_t requiredGrantDatabaseHeight =nHeight-GRANTBLOCKINTERVAL;

 	if(fDebug)LogPrintf("Checking GDB is updated...Required Height : %ld, requested from: %ld \n",requiredGrantDatabaseHeight, nHeight);
    //Maybe we don't have to count votes from the start - let's check if there's a recent vote database stored
    if(grantDatabaseBlockHeight== -1){
			deSerializeGrantDB((GetDataDir() / "ratings/grantdb.dat" ).string().c_str(), requiredGrantDatabaseHeight );
	}
    while(grantDatabaseBlockHeight < requiredGrantDatabaseHeight ){
        processNextBlockIntoGrantDatabase();
	}
    return true;
}

int getOfficeNumberFromAddress(string grantVoteAddress){
	if (!startsWith(grantVoteAddress.c_str(),GRANTPREFIX.c_str())){
		return -1;
	}
    for(int i=0;i<numberOfOffices+1;i++){

		if(grantVoteAddress.substr(4,3)==electedOffices[i]){
			return i;
		}
	}
	return -1;
}

void printVotingPrefs(std::string address){

    //This is slow and iterates too much, but on the plus side it doesn't crash the program.
    //This crash probably caused by eliminate candidate corrupting the ballot structure.

    int pref=1;
    for(ballotit=ballots.begin(); ballotit!=ballots.end(); ++ballotit){
        if(address==ballotit->first){
            for(svpit4=ballotit->second.begin();svpit4!=ballotit->second.end();++svpit4){
                grantAwardsOutput<<"--Preference "<<pref<<" "<<svpit4->first<<" "<<svpit4->second.c_str()<<" \n";
                pref++;
            }
        }
    }

}

void processNextBlockIntoGrantDatabase(){	
	CBlock block;

	if(gdBlockPointer != NULL){
		gdBlockPointer = chainActive.Tip();
	}else{
		gdBlockPointer = chainActive.Genesis();
	}
/*
 * 	CBlock block;
    CBlockIndex* pblockindex = chainActive[209980];
    std::string hex = pblockindex->phashBlock->GetHex();     
    uint256 hash(hex);

	if(gdBlockPointer != NULL){
		gdBlockPointer = chainActive.Tip();
	}else{
		gdBlockPointer = mapBlockIndex[hash];
	}
 * further study required
*/
	ReadBlockFromDisk(block, gdBlockPointer);
	
		std::map<std::string,int64_t > votes;
		std::map<std::string,int64_t >::iterator votesit;
		
    BOOST_FOREACH(const CTransaction& tx, block.vtx){

		for (unsigned int j = 0; j < tx.vout.size();j++){
			CTxDestination address;
			ExtractDestination(tx.vout[j].scriptPubKey, address);
			string receiveAddress = CBitcreditAddress( address ).ToString().c_str();
			int64_t theAmount = tx.vout[ j ].nValue;
			balances[ receiveAddress ] = balances[ receiveAddress ] + theAmount;
			if(theAmount == 1000 &&	startsWith(receiveAddress.c_str(), GRANTPREFIX.c_str())){
				votes[receiveAddress] = theAmount;
			}
		}

        for (size_t i = 0; i < tx.vin.size(); i++){
			if (tx.IsCoinBase())
				continue; 			
            const CScript &script = tx.vin[i].scriptSig;
            opcodetype opcode;
            std::vector<unsigned char> vch;
            uint256 prevoutHash, blockHash;
            string spendAddress;
            int64_t theAmount;
            for (CScript::const_iterator pc = script.begin(); script.GetOp(pc, opcode, vch); ){
                if (opcode == 33){
                    CPubKey pubKey(vch);
                    prevoutHash = tx.vin[i].prevout.hash;
                    CTransaction txOfPrevOutput;
                    if (!GetTransaction(prevoutHash, txOfPrevOutput, blockHash, true))
                    {
                        continue;
                    }                               
                    unsigned int nOut = tx.vin[i].prevout.n;
                    if (nOut >= txOfPrevOutput.vout.size())
                    {
                        continue;
                    }
                    const CTxOut &txOut = txOfPrevOutput.vout[nOut];
                    CTxDestination addressRet;
                    if (!ExtractDestination(txOut.scriptPubKey, addressRet))
                    {
                        continue;
                    }                    
                    spendAddress = CBitcreditAddress(addressRet).ToString().c_str();
					theAmount =  txOut.nValue;
					balances[ spendAddress ] = balances[ spendAddress ] - theAmount;

				for( votesit = votes.begin();votesit != votes.end(); ++votesit){
                    if(fDebug)LogPrintf(" Vote found: %s, %ld\n",votesit->first.c_str(),votesit->second);
					string grantVoteAddress = ( votesit->first );
					int electedOfficeNumber = getOfficeNumberFromAddress(grantVoteAddress);

					if( electedOfficeNumber > -1 ){
                        votingPreferences[ electedOfficeNumber ][ spendAddress ][ votesit->second ] = grantVoteAddress;
                    }
				}
                }
            }
        }
    }
	grantDatabaseBlockHeight++;

	if(fDebug)LogPrintf("Block has been processed. Grant Database Block Height is now updated to Block # %ld\n", grantDatabaseBlockHeight);
	if (isGrantAwardBlock(grantDatabaseBlockHeight + GRANTBLOCKINTERVAL)) {
		getGrantAwardsFromDatabaseForBlock( grantDatabaseBlockHeight + GRANTBLOCKINTERVAL );

	}

    boost::filesystem::path biddir = GetDataDir() / "ratings";

    if(!(boost::filesystem::exists(biddir))){
        if(fDebug)LogPrintf("Grants dir Doesn't Exists\n");

        if (boost::filesystem::create_directory(biddir))
            if(fDebug)LogPrintf("Grants dir....Successfully Created !\n");
    }
	serializeGrantDB( (GetDataDir() / "ratings/grantdb.dat" ).string().c_str() );
}

void printCandidateSupport(){
	std::map<int64_t,std::string>::reverse_iterator itpv2;

	grantAwardsOutput<<"\nWinner Support: \n";
		
	for(ballotit=supportVotes.begin(); ballotit!=supportVotes.end(); ++ballotit){
		grantAwardsOutput<<"\n--"<<ballotit->first<<" \n";
		for(itpv2=ballotit->second.rbegin();itpv2!=ballotit->second.rend();++itpv2){
            grantAwardsOutput<<"-->("<< itpv2->first/COIN <<"/"<<balances[itpv2->second.c_str()]/COIN <<") "<<itpv2->second.c_str()<<" \n";
		}		
	}
}

void printBalances( int64_t howMany, bool printVoting, bool printWasted ){
	grantAwardsOutput<<"---Current Balances------\n";
	std::multimap<int64_t, std::string > sortByBalance;

	std::map<std::string,int64_t>::iterator itpv;
	std::map<int64_t,std::string>::reverse_iterator itpv2;

	for(itpv=balances.begin(); itpv!=balances.end(); ++itpv){
		if(itpv->second>COIN){
			sortByBalance.insert(pair<int64_t, std::string>(itpv->second,itpv->first));
		}
	}

	std::multimap<int64_t, std::string >::reverse_iterator sbbit;
	int64_t count = 0;
	for (sbbit =  sortByBalance.rbegin(); sbbit !=  sortByBalance.rend();++sbbit){
		if(howMany>count){
			grantAwardsOutput<<"\n->Balance:"<<sbbit->first/COIN<<" - "<<sbbit->second.c_str()<<"\n";
			if(printWasted){
				for(itpv2=electedVotes[sbbit->second.c_str()].rbegin(); itpv2!=electedVotes[sbbit->second.c_str()].rend(); ++itpv2){
					grantAwardsOutput << "---->" << itpv2->first/COIN << " supported " << itpv2->second << "\n";
				}
				if(wastedVotes[sbbit->second.c_str()]/COIN>0){
					grantAwardsOutput<<"---->"<<wastedVotes[sbbit->second.c_str()]/COIN<<"  wasted (Add More Preferences)\n";
				}
				if(votingPreferences[0].find(sbbit->second.c_str())==votingPreferences[0].end()){
					grantAwardsOutput<<"---->No Vote: (Add Some Voting Preferences)\n";
				}
			}
			
			if(printVoting){
				try{
					printVotingPrefs(sbbit->second);
				}catch (std::exception &e) {
					grantAwardsOutput<<"Print Voting Prefs Exception\n";
				}
			}
			count++;
		}
	}
	grantAwardsOutput<<"---End Balances------\n";
}

bool getGrantAwardsFromDatabaseForBlock(int64_t nHeight){
    if(fDebug)LogPrintf( "getGrantAwardsFromDatabaseForBlock %ld\n", nHeight );
	if(grantDatabaseBlockHeight!=nHeight-GRANTBLOCKINTERVAL){
        LogPrintf("getGrantAwardsFromDatabase is being called when no awards are due. %ld %ld\n",grantDatabaseBlockHeight,nHeight);
		return false;
	}
	debugVoteExtra = GetBoolArg("-debugvoteex", false);
	debugVote = GetBoolArg("-debugvote", false);
	if(debugVote){
		std::stringstream sstm;
		sstm << "award" << setw(8) << std::setfill('0') << nHeight << ".dat";
		string filename = sstm.str();
		mkdir((GetDataDir() / "grantawards").string().c_str()
#ifndef _WIN32
		,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
#endif
		);
		grantAwardsOutput.open ((GetDataDir() / "grantawards" / filename).string().c_str(), ios::trunc);}
	//save to disk
	
	if(debugVote)grantAwardsOutput << "-------------:\nElection Count:\n-------------:\n\n";
	
	//Clear from last time, ensure nothing left over

	awardWinners.clear();
	grantAwards.clear();

	for( int i = 0;	i < numberOfOffices + 1;i++ ){
		ballots.clear();
		ballotBalances.clear();
		ballotWeights.clear();
		wastedVotes.clear();
		electedVotes.clear();
		supportVotes.clear();

		//Iterate through every vote
		for( vpit = votingPreferences[i].begin();vpit != votingPreferences[i].end();++vpit)	{
			int64_t voterBalance = balances[ vpit->first ];
			//Ignore balances of 0 - they play no part.
			if( voterBalance > 0 ){
				ballotBalances[ vpit->first ] = voterBalance;
				ballotWeights[ vpit->first ] = 1.0;

				//Order preferences by coins sent - lowest number of coins has top preference
				for( it2 = vpit->second.begin();it2 != vpit->second.end();	++it2){
					//Where a voter has voted for more than one preference with the same amount, only the last one (alphabetically) will be valid. The others are discarded.
					ballots[ vpit->first ][ it2->first ] = it2->second;
				}
			}
		}
		//TODO: decrease intensity of this function.
		getWinnersFromBallots( nHeight, i );
		//At this point, we know the vote winners - now to see if grants are to be awarded
		if( i < numberOfOffices ){
			for(int i=0;i<1;i++){
				grantAwards[ awardWinners[ i ] ] = grantAwards[ awardWinners[ i ] ] + GetGrantValue( nHeight, 0 );

				if( debugVote ){
					grantAwardsOutput << "Add grant award to Block "<<awardWinners[0].c_str()<<" ("<<GetGrantValue(nHeight, 0)/COIN<<")\n";
				}
			}
		}

	if(debugVote)printCandidateSupport();
	}

	if(debugVote){grantAwardsOutput.close();}
	return true;
}

void getWinnersFromBallots( int64_t nHeight, int officeNumber ){

	if(debugVote)grantAwardsOutput <<"\n\n\n--------"<< electedOffices[officeNumber]<<"--------\n";	
	if(debugVoteExtra)printBallots();
	
	//Calculate Total in all balances
	int64_t tally=0;
	for(it=balances.begin(); it!=balances.end(); ++it){
		tally=tally+it->second;
	}
	if(debugVote)grantAwardsOutput <<"Total coin issued: " << tally/COIN <<"\n";

	//Calculate Total of balances of voters
	int64_t totalOfVoterBalances=0;

	for( it = ballotBalances.begin();it != ballotBalances.end();++it)	{
		totalOfVoterBalances = totalOfVoterBalances + it->second;
	}

	if( debugVote ){
		grantAwardsOutput <<"Total of Voters' Balances: "<<totalOfVoterBalances/COIN<<"\n";
	}
	//Turnout
	if( debugVote ){
		grantAwardsOutput <<"Percentage of total issued coin voting: "<<(totalOfVoterBalances*100)/tally<<" percent\n";
	}
	//Calculate Droop Quota
	int64_t droopQuota = (totalOfVoterBalances/2) + 1;
	if( debugVote ){
		grantAwardsOutput <<"Droop Quota: "<<droopQuota/COIN<<"\n";
	}
	//Conduct voting rounds until all grants are awarded
	for(int i = 1;i > 0;i--){
		string electedCandidate;
		int voteRoundNumber = 0;
		if( debugVote ){
			grantAwardsOutput <<"-------------:\nRound:"<<1-i<<"\n";
		}
		if( debugVoteExtra ){
			printBallots();
		}
		do{
			electedCandidate = electOrEliminate( droopQuota, i );
			voteRoundNumber++;
		}while( electedCandidate == "" );
		awardWinners[ ( i - 1 ) *- 1 ] = electedCandidate;
	}
}

string electOrEliminate( int64_t droopQuota, unsigned int requiredCandidates ){

	std::map<std::string,int64_t >::iterator tpcit;
	//Recalculate the preferences each time as winners and losers are removed from ballots.
	preferenceCount.clear();
	//Calculate support for each candidate. The balance X the weighting for each voter is applied to the total for the candidate currently at the top of the voter's ballot
	for( ballotit = ballots.begin();ballotit != ballots.end();++ballotit){
		//Check: Multiplying int64_t by double here, and representing answer as int64_t.
		preferenceCount[ ballotit->second.begin()->second ] += ( ballotBalances[ ballotit->first ] * ballotWeights[ ballotit->first ] );
	}

	//Find out which remaining candidate has the greatest and least support
	string topOfThePoll;
	int64_t topOfThePollAmount = 0;
	string bottomOfThePoll;
	int64_t bottomOfThePollAmount = 9223372036854775807;

	for( tpcit = preferenceCount.begin();tpcit != preferenceCount.end();++tpcit){
		//Check:When competing candidates have equal votes, the first (sorted by Map) will be chosen for top and bottom of the poll.
		if( tpcit->second > topOfThePollAmount ){
			topOfThePollAmount = tpcit->second;
			topOfThePoll = tpcit->first;
		}
		if( tpcit->second < bottomOfThePollAmount ){
			bottomOfThePollAmount = tpcit->second;
			bottomOfThePoll = tpcit->first;
		}
	}

	//Purely for debugging/information
	if( topOfThePollAmount >= droopQuota ||	requiredCandidates >= preferenceCount.size() ||	bottomOfThePollAmount > droopQuota / 10){
		if( debugVote ){
			grantAwardsOutput <<"Candidates with votes equalling more than 10% of Droop quota\n";
		}
		for( tpcit = preferenceCount.begin();tpcit != preferenceCount.end();++tpcit){
			if( tpcit->second > droopQuota / 10 ){
				if(debugVote)
				grantAwardsOutput <<"Support: "<<tpcit->first<<" ("<<tpcit->second/COIN<<")\n";
			}
		}
	}
		
	if(topOfThePollAmount==0){
		//No ballots left -end -
		if(debugVote)grantAwardsOutput <<"No Candidates with support remaining. Grant awarded to unspendable address 6BCRBKZLmq2JwWLWDtJZL26ao4uHhqG6mH\n";
		return "6BCRBKZLmq2JwWLWDtJZL26ao4uHhqG6mH";
	}
	
	if(topOfThePollAmount>=droopQuota || requiredCandidates>=preferenceCount.size()){
		
		//Note: This is a simplified Gregory Transfer Value - ignoring ballots where there are no other hopefuls.
		double gregorySurplusTransferValue=((double)topOfThePollAmount-(double)droopQuota)/(double)topOfThePollAmount;
		
		//Don't want this value to be negative when candidates are elected with less than a quota
		if(gregorySurplusTransferValue<0){gregorySurplusTransferValue=0;}
		
		electCandidate(topOfThePoll,gregorySurplusTransferValue,(requiredCandidates==1));	
		if(debugVote){
			if(numberCandidatesEliminated>0){
				grantAwardsOutput <<"Candidates Eliminated ("<<numberCandidatesEliminated<<")\n\n";
				numberCandidatesEliminated=0;
			}
			grantAwardsOutput <<"Candidate Elected: "<<topOfThePoll.c_str()<<" ("<<topOfThePollAmount/COIN<<")\n";
			grantAwardsOutput <<"Surplus Transfer Value: "<<gregorySurplusTransferValue<<"\n";				
		}
		return topOfThePoll;
		
	}else{
		eliminateCandidate(bottomOfThePoll,false);
		if(debugVote){
			if(bottomOfThePollAmount>droopQuota/10){
				if(numberCandidatesEliminated>0){
					grantAwardsOutput <<"Candidates Eliminated ("<<numberCandidatesEliminated<<")\n";
					numberCandidatesEliminated=0;
				}
				grantAwardsOutput <<"Candidate Eliminated: "<<bottomOfThePoll.c_str()<<" ("<<bottomOfThePollAmount/COIN<<")\n\n";
			}else{
				numberCandidatesEliminated++;
			}
		}
		return "";
	}
}

void electCandidate(string topOfThePoll, double gregorySurplusTransferValue,bool isLastCandidate){
	
	//Apply fraction to weights where the candidate was top of the preference list
	for(ballotit=ballots.begin(); ballotit!=ballots.end(); ++ballotit){
		svpit2=ballotit->second.begin();
		if(svpit2->second==topOfThePoll){
			//Record how many votes went towards electing this candidate for each user
			electedVotes[ballotit->first][balances[ballotit->first]*(ballotWeights[ballotit->first]*(1-gregorySurplusTransferValue))]=svpit2->second;
			//Record the support for each candidate elected
			supportVotes[topOfThePoll][balances[ballotit->first]*(ballotWeights[ballotit->first]*(1-gregorySurplusTransferValue))]=ballotit->first;
			
			//This voter had the elected candidate at the top of the ballot. Adjust weight for future preferences.
			ballotWeights[ballotit->first]=ballotWeights[ballotit->first]*gregorySurplusTransferValue;
		}
	}
	eliminateCandidate(topOfThePoll,isLastCandidate);
}

void eliminateCandidate(string removeid,bool isLastCandidate){

	std::map<std::string, int64_t> ballotsToRemove;
	std::map<std::string, int64_t>::iterator btrit;
	
	//Remove candidate from all ballots - note the candidate may be way down the preference list
	for(ballotit=ballots.begin(); ballotit!=ballots.end(); ++ballotit){
		int64_t markForRemoval = 0;
		for(svpit2=ballotit->second.begin();svpit2!=ballotit->second.end();++svpit2){
			if(svpit2->second==removeid){
				markForRemoval = svpit2->first;
			}
		}
		
		if(markForRemoval!=0){
			ballotit->second.erase(markForRemoval);
		}
		
		//Make a note of ballot to remove
		if(ballotit->second.size()==0){
			if(!isLastCandidate){
				wastedVotes[ballotit->first]=(ballotBalances[ballotit->first]*ballotWeights[ballotit->first]);
			}
			ballotsToRemove[ballotit->first]=1;
		}
	}
	for(btrit=ballotsToRemove.begin(); btrit!=ballotsToRemove.end(); ++btrit){
		ballots.erase(btrit->first);
	}
}

void printBallots(){
	LogPrintf("Current Ballot State\n");
	int cutOff = 0;
	for( ballotit = ballots.begin();ballotit != ballots.end();++ballotit){
			LogPrintf("Voter: %s Balance: %ld Weight: %f Total: %f\n",ballotit->first.c_str(),ballotBalances[ ballotit->first ] / COIN,ballotWeights[ ballotit->first ],
				(ballotBalances[ ballotit->first ] / COIN ) * ballotWeights[ ballotit->first ]);
			int cutOff2 = 0;
				for( svpit2 = ballotit->second.begin();	svpit2 != ballotit->second.end();++svpit2){
					LogPrintf( "Preference: (%d) %ld %s \n",cutOff2,svpit2->first,svpit2->second.c_str());
				}
			cutOff2++;
		cutOff++;
	}
}

bool startsWith( const char *str, const char *pre ){
    size_t lenpre = strlen( pre ),
           lenstr = strlen( str );

    return lenstr < lenpre ? false : strncmp( pre, str, lenpre ) == 0;
}
