#include "trust.h"

#include "bidtracker.h"
#include "voting.h"
#include "util.h"
#include "utilmoneystr.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "primitives/block.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void TrustEngine::sortclientdata(std::string clientAddress){

	sqlite3 *rawdb;
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	int rc;
	const char *sql;

   rc = sqlite3_open((GetDataDir() / "ratings/rawdata.db").string().c_str(), &rawdb);
   if( rc ){
     if (fDebug) LogPrintf("Can't open database: %s\n", sqlite3_errmsg(rawdb));
   }else{
     if (fDebug) LogPrintf("Opened database successfully\n");
   }
	sql="select * from RAWDATA where ADDRESS = ?";

	rc = sqlite3_prepare(rawdb,sql, strlen(sql), &stmt,  0 );
    sqlite3_bind_text(stmt, 1,clientAddress.data(), clientAddress.size(), 0);
        int64_t balance, outgoingtx, firstuse, incomingtx, totalinputs, totaloutputs;
	if (sqlite3_step(stmt) == SQLITE_ROW){
        
		balance = (sqlite3_column_int(stmt, 1))/COIN;
		firstuse = sqlite3_column_int(stmt, 2);
		incomingtx = sqlite3_column_int(stmt, 3);
		outgoingtx = sqlite3_column_int(stmt, 4);
		totalinputs = sqlite3_column_int(stmt, 5);
		totaloutputs = sqlite3_column_int(stmt, 6);

		int64_t totaltx=  incomingtx+outgoingtx;
		double globallife = (GetTime()- 1418504572)/24*3600;
		double lifetime = (GetTime() - firstuse)/24*3600;
		int64_t totalnettx = chainActive.Tip()->nChainTx;
		double txfreq = totaltx/lifetime;
		double nettxfreq = totalnettx / globallife;
		double spendfactor = balance/totaloutputs;
		double savefactor;

				if (totalinputs !=0)
					savefactor =balance/totalinputs;
				else
					savefactor = 0;
			double nettxpart = totaltx/ totalnettx;
			double aveinput = totalinputs/ incomingtx;
			double aveoutput = totaloutputs / outgoingtx;
			double avedailyincome = totalinputs / lifetime;
			double avedailyexpenditure = totaloutputs / lifetime;
//			double minerpoints = minedblocks/chainActive.Tip()->nHeight;

			double trust=0;
			{
				{
					if (lifetime > 360 )
						trust+= 20;
					else if (lifetime > 30 && lifetime < 360 )
						trust+= lifetime*0.05;
					else
						trust+= 0;
				}

				{
					if (totaltx > 10000){
						trust+= 10;
					}
					else if (totaltx>0 && totaltx< 10000){
						trust+= totaltx*0.001;
					}
					else
						trust+= 0;
				}

				{
					if(balance > 1000000){
						trust+= 25;
					}
					else if(balance > 0 && balance <= 1000000){
						trust+= balance/50000;
					}
					else
						trust+= 0;
				}

				{
					if (txfreq > 5)
						trust+=15;
					else if (txfreq> 0.01 && txfreq< 5)
						trust+= txfreq *3;
					else
						trust+= 0;
				}

				{
					if (savefactor > 0.1)
						trust+=20;
					else if (savefactor> 0.001 && savefactor< 0.1)
						trust+= savefactor *200;
					else
						trust+= 0;
				}

				{
					if (avedailyincome > 100)
						trust+=20;
					else if (avedailyincome > 1 && avedailyincome< 100)
						trust+= avedailyincome/5;
					else
						trust+= 0;
				}

				{
                    /*if (minerpoints > 0.001)
						trust+=25;
					else if (minerpoints> 0.00001 && minerpoints< 0.001)
						trust+= minerpoints *25000;
					else
                        trust+= 0;*/
				}
			}
        	//myfile2<<strs[0]<<","<<balance<<","<<avedailyincome<<","<<trust<< ","<<endl;


            CBitcreditAddress address(clientAddress.c_str());
			CTxDestination dest = address.Get();
			std::set<CExtDiskTxPos> setpos;
			if (!FindTransactionsByDestination(dest, setpos)){
				LogPrintf("FindTransactionsByDestination failed \n");
			}
			std::set<CExtDiskTxPos>::const_iterator it = setpos.begin();
			vector<pair<string,CAmount> > bndebits;
			vector<pair<string,CAmount> > bncredits;
			vector<pair<string,CAmount> > bnvoters;
			double bntrust =0;
			while(it != setpos.end()) {
				CTransaction tx;
				uint256 hashBlock;
				if (!ReadTransaction(tx, *it, hashBlock))
					continue;
				BOOST_FOREACH(const CTxOut &txOut, tx.vout){
					CTxDestination address;
					ExtractDestination(txOut.scriptPubKey, address);
					string receiveAddress = CBitcreditAddress( address ).ToString().c_str();
					int64_t theAmount = txOut.nValue;
					if(startsWith(receiveAddress.c_str(), "6BCRbnk")){
						bncredits.push_back(std::make_pair(receiveAddress,theAmount));
					}
				}

				BOOST_FOREACH(const CTxIn& txin, tx.vin) {
					if (tx.IsCoinBase())
						continue;
					const CScript &script = txin.scriptSig;
					opcodetype opcode;
					std::vector<unsigned char> vch;
					uint256 prevoutHash, blockHash;
					string spendAddress;
					int64_t theAmount;
					for (CScript::const_iterator pc = script.begin(); script.GetOp(pc, opcode, vch); ){
						if (opcode == 33){
							CPubKey pubKey(vch);
							prevoutHash = txin.prevout.hash;
							CTransaction txOfPrevOutput;
							if (!GetTransaction(prevoutHash, txOfPrevOutput, blockHash, true)){
									continue;
							}
							unsigned int nOut = txin.prevout.n;
							if (nOut >= txOfPrevOutput.vout.size()){
								if (fDebug)LogPrintf("Output %u, not in transaction: %s\n", nOut, prevoutHash.ToString().c_str());
									continue;
							}
							const CTxOut &txOut = txOfPrevOutput.vout[nOut];
							CTxDestination addressRet;
							if (!ExtractDestination(txOut.scriptPubKey, addressRet)){
								if (fDebug)LogPrintf("ExtractDestination failed: %s\n", prevoutHash.ToString().c_str());
									continue;
							}
							spendAddress = CBitcreditAddress(addressRet).ToString().c_str();
							theAmount =  txOut.nValue;
							if (startsWith(spendAddress.c_str(), "6BCRbnk"))
								bndebits.push_back(std::make_pair(spendAddress,theAmount));
							if (startsWith(spendAddress.c_str(), "6BCRbnk")&& theAmount==1000 && balance> 50000*COIN){
								for(vector<pair<string,CAmount> >::const_iterator it2 = bnvoters.begin();it2 != bnvoters.end () ; it2++){
									if (it2->first != spendAddress){
									bnvoters.push_back(std::make_pair(spendAddress,theAmount));
									bntrust+= 0.1;
									}
								}
							}
						}
					}
				}
				it++;
			}
			double creditscore =0;
			int debits = bncredits.size() - bndebits.size();
			for(vector<pair<string,CAmount> >::const_iterator it = bndebits.begin();it != bndebits.end () ; it++){
				for(vector<pair<string,CAmount> >::const_iterator it2 = bncredits.begin();it2 != bncredits.end () ; it2++){
					if (it->first == it2->first && it->second < it2->second)
						creditscore+= 1;
				}
			}
			if (debits> -2)
				creditscore=0;
			ofstream addrdb;
			addrdb.open ((GetDataDir() / "ratings/creditfinal.dat" ).string().c_str(), std::ofstream::app);
            addrdb<<clientAddress<<","<<balance<<","<<avedailyincome<<","<<trust<< ","<< creditscore << endl;
	}
}


void TrustEngine::createdb()
{
   sqlite3 *rawdb;
   char *zErrMsg = 0;
   int rc;

	vector<const char*> sql;

   rc = sqlite3_open((GetDataDir() /"ratings/rawdata.db").string().c_str(), &rawdb);
   if( rc ){
      if(fDebug)LogPrintf("Can't open database: %s\n", sqlite3_errmsg(rawdb));
      exit(0);
   }else{
      if(fDebug)LogPrintf("Opened database successfully\n");
   }

   /* Create SQL statements */
   sql.push_back("CREATE TABLE RAWDATA("  \
         "ADDRESS TEXT PRIMARY KEY      NOT NULL," \
         "BALANCE           INTEGER     DEFAULT 0," \
         "FIRSTSEEN         INTEGER     DEFAULT 0," \
         "TXINCOUNT         INTEGER     DEFAULT 0," \
         "TXOUTCOUNT        INTEGER     DEFAULT 0," \
         "TOTALIN           INTEGER     DEFAULT 0," \
         "TOTALOUT          INTEGER     DEFAULT 0);");

  sql.push_back("CREATE TABLE CLIENTS("  \
         "ADDRESS TEXT PRIMARY KEY      NOT NULL," \
         "BALANCE           INTEGER     DEFAULT 0," \
         "INCOME            INTEGER     DEFAULT 0," \
         "EXPENDITURE       INTEGER     DEFAULT 0," \
         "TRUST             INTEGER     DEFAULT 0," \
         "CREDITSCORE       INTEGER     DEFAULT 0," \
         "RATING            INTEGER     DEFAULT 0);");

  sql.push_back("CREATE TABLE TRANSACTIONS(" \
            "    ID INTEGER PRIMARY KEY AUTOINCREMENT," \
            "    HASH TEXT," \
            "    BLOCKNUM INTEGER);");

  sql.push_back("CREATE TABLE BLOCKS(" \
            "    ID INTEGER PRIMARY KEY AUTOINCREMENT," \
            "    HASH TEXT," \
            "    TIME INTEGER," \
            "    MINER TEXT);");

  sql.push_back("CREATE TABLE OUTPUTS(" \
            "    ID INTEGER PRIMARY KEY AUTOINCREMENT," \
            "    TXID TEXT," \
            "    DADDR TEXT," \
            "    VALUE INTEGER," \
            "    OFFSET INTEGER);");

  sql.push_back("CREATE TABLE INPUTS(" \
			"    ID INTEGER PRIMARY KEY AUTOINCREMENT," \
            "    TXID TEXT ," \
            "    SRCADD TEXT," \
            "    VALUE INTEGER," \
            "    OFFSET INTEGER);");

   /* Execute SQL statements */
	for (unsigned int i =0;i < sql.size();i++){
		rc = sqlite3_exec(rawdb, sql[i], callback, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			if (fDebug)LogPrintf("SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else{
			if (fDebug)LogPrintf( "Tables created successfully\n");
		}
	}

	if(sqlite3_close(rawdb) != SQLITE_OK ){
		if (fDebug)LogPrintf("SQL unable to close database %s\n", sqlite3_errmsg(rawdb));
		sqlite3_free(zErrMsg);
	}else{
		if (fDebug)LogPrintf( "database closed successfully\n");
	}
}
