#include "banknode.h"
#include "activebanknode.h"
#include "darksend.h"
#include "primitives/transaction.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>


int CBankNode::minProtoVersion = MIN_MN_PROTO_VERSION;

CCriticalSection cs_banknodes;

/** The list of active banknodes */
std::vector<CBankNode> vecBanknodes;
/** Object for who's going to get paid on which blocks */
CBanknodePayments banknodePayments;
// keep track of banknode votes I've seen
map<uint256, CBanknodePaymentWinner> mapSeenBanknodeVotes;
// keep track of the scanning errors I've seen
map<uint256, int> mapSeenBanknodeScanningErrors;
// who's asked for the banknode list and the last time
std::map<CNetAddr, int64_t> askedForBanknodeList;
// which banknodes we've asked for
std::map<COutPoint, int64_t> askedForBanknodeListEntry;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

// manage the banknode connections
void ProcessBanknodeConnections(){
    LOCK(cs_vNodes);

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        //if it's our banknode, let it be
        if(darkSendPool.submittedToBanknode == pnode->addr) continue;

        if(pnode->fDarkSendMaster){
            LogPrintf("Closing banknode connection %s \n", pnode->addr.ToString().c_str());
            pnode->CloseSocketDisconnect();
        }
    }
}

void ProcessMessageBanknode(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if (strCommand == "dsee") { //DarkSend Election Entry
        if(fLiteMode) return; //disable all darksend/banknode related functionality

        bool fIsInitialDownload = IsInitialBlockDownload();
        if(fIsInitialDownload) return;

        CTxIn vin;
        CService addr;
        CPubKey pubkey;
        CPubKey pubkey2;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        int count;
        int current;
        int64_t lastUpdated;
        int protocolVersion;
        std::string strMessage;

        // 70047 and greater
        vRecv >> vin >> addr >> vchSig >> sigTime >> pubkey >> pubkey2 >> count >> current >> lastUpdated >> protocolVersion;

        // make sure signature isn't in the future (past is OK)
        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dsee - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        bool isLocal = addr.IsRFC1918() || addr.IsLocal();
        
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

        strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        if(protocolVersion < MIN_MN_PROTO_VERSION) {
            LogPrintf("dsee - ignoring outdated banknode %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
            return;
        }

        CScript pubkeyScript;
        pubkeyScript =GetScriptForDestination(pubkey.GetID());

        if(pubkeyScript.size() != 25) {
            LogPrintf("dsee - pubkey the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CScript pubkeyScript2;
        pubkeyScript2 =GetScriptForDestination(pubkey2.GetID());

        if(pubkeyScript2.size() != 25) {
            LogPrintf("dsee - pubkey2 the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)){
            LogPrintf("dsee - Got bad banknode address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        

        //search existing banknode list, this is where we update existing banknodes with new dsee broadcasts
	LOCK(cs_banknodes);
        BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
            if(mn.vin.prevout == vin.prevout) {
                // count == -1 when it's a new entry
                //   e.g. We don't want the entry relayed/time updated when we're syncing the list
                // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
                //   after that they just need to match
                if(count == -1 && mn.pubkey == pubkey && !mn.UpdatedWithin(BANKNODE_MIN_DSEE_SECONDS)){
                    mn.UpdateLastSeen();

                    if(mn.now < sigTime){ //take the newest entry
                        LogPrintf("dsee - Got updated entry for %s\n", addr.ToString().c_str());
                        mn.pubkey2 = pubkey2;
                        mn.now = sigTime;
                        mn.sig = vchSig;
                        mn.protocolVersion = protocolVersion;
                        mn.addr = addr;

                        RelayDarkSendElectionEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
                    }
                }

                return;
            }
        }

        // make sure the vout that was signed is related to the transaction that spawned the banknode
        //  - this is expensive, so it's only done once per banknode
        if(!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(fDebug) LogPrintf("dsee - Got NEW banknode entry %s\n", addr.ToString().c_str());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()

        CValidationState state;
        CMutableTransaction tx = CTransaction();
        CTxOut vout = CTxOut(249999.99*COIN, darkSendPool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);
        if(AcceptableInputs(mempool, state, tx)){
            if(fDebug) LogPrintf("dsee - Accepted banknode entry %i %i\n", count, current);

            if(GetInputAge(vin) < BANKNODE_MIN_CONFIRMATIONS){
                LogPrintf("dsee - Input must have least %d confirmations\n", BANKNODE_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }

            // use this as a peer
            addrman.Add(CAddress(addr), pfrom->addr, 2*60*60);

            // add our banknode
            CBankNode mn(addr, vin, pubkey, vchSig, sigTime, pubkey2, protocolVersion);
            mn.UpdateLastSeen(lastUpdated);
            vecBanknodes.push_back(mn);

            // if it matches our banknodeprivkey, then we've been remotely activated
            if(pubkey2 == activeBanknode.pubKeyBanknode && protocolVersion == PROTOCOL_VERSION){
                activeBanknode.EnableHotColdBankNode(vin, addr);
            }

            if(count == -1 && !isLocal)
                RelayDarkSendElectionEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);

        } else {
            LogPrintf("dsee - Rejected banknode entry %s\n", addr.ToString().c_str());

            int nDoS = 0;
            if (state.IsInvalid(nDoS))
            {
                LogPrintf("dsee - %s from %s %s was not accepted into the memory pool\n", tx.GetHash().ToString().c_str(),
                    pfrom->addr.ToString().c_str(), pfrom->cleanSubVer.c_str());
                if (nDoS > 0)
                    Misbehaving(pfrom->GetId(), nDoS);
            }
        }
    }

    else if (strCommand == "dseep") { //DarkSend Election Entry Ping
        if(fLiteMode) return; //disable all darksend/banknode related functionality
        bool fIsInitialDownload = IsInitialBlockDownload();
        if(fIsInitialDownload) return;

        CTxIn vin;
        vector<unsigned char> vchSig;
        int64_t sigTime;
        bool stop;
        vRecv >> vin >> vchSig >> sigTime >> stop;

        //LogPrintf("dseep - Received: vin: %s sigTime: %lld stop: %s\n", vin.ToString().c_str(), sigTime, stop ? "true" : "false");

        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        if (sigTime <= GetAdjustedTime() - 60 * 60) {
            LogPrintf("dseep - Signature rejected, too far into the past %s - %d %d \n", vin.ToString().c_str(), sigTime, GetAdjustedTime());
            return;
        }

        // see if we have this banknode
	LOCK(cs_banknodes);
        BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
            if(mn.vin.prevout == vin.prevout) {
            	// LogPrintf("dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            	// take this only if it's newer
                if(mn.lastDseep < sigTime){
                    std::string strMessage = mn.addr.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(stop);

                    std::string errorMessage = "";
                    if(!darkSendSigner.VerifyMessage(mn.pubkey2, vchSig, strMessage, errorMessage)){
                        LogPrintf("dseep - Got bad banknode address signature %s \n", vin.ToString().c_str());
                        //Misbehaving(pfrom->GetId(), 100);
                        return;
                    }

                    mn.lastDseep = sigTime;

                    if(!mn.UpdatedWithin(BANKNODE_MIN_DSEEP_SECONDS)){
                        mn.UpdateLastSeen();
                        if(stop) {
                            mn.Disable();
                            mn.Check();
                        }
                        RelayDarkSendElectionEntryPing(vin, vchSig, sigTime, stop);
                    }
                }
                return;
            }
        }

        if(fDebug) LogPrintf("dseep - Couldn't find banknode entry %s\n", vin.ToString().c_str());

        std::map<COutPoint, int64_t>::iterator i = askedForBanknodeListEntry.find(vin.prevout);
        if (i != askedForBanknodeListEntry.end()){
            int64_t t = (*i).second;
            if (GetTime() < t) {
                // we've asked recently
                return;
            }
        }

        // ask for the dsee info once from the node that sent dseep

        LogPrintf("dseep - Asking source node for missing entry %s\n", vin.ToString().c_str());
        pfrom->PushMessage("dseg", vin);
        int64_t askAgain = GetTime()+(60*60*24);
        askedForBanknodeListEntry[vin.prevout] = askAgain;

    } else if (strCommand == "dseg") { //Get banknode list or specific entry
        if(fLiteMode) return; //disable all darksend/banknode related functionality
        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network

                std::map<CNetAddr, int64_t>::iterator i = askedForBanknodeList.find(pfrom->addr);
                if (i != askedForBanknodeList.end())
                {
                    int64_t t = (*i).second;
                    if (GetTime() < t) {

                    }
                }

                int64_t askAgain = GetTime()+(60*60*3);
                askedForBanknodeList[pfrom->addr] = askAgain;
         
        } //else, asking for a specific node which is ok

	LOCK(cs_banknodes);
        int count = vecBanknodes.size();
        int i = 0;

        BOOST_FOREACH(CBankNode mn, vecBanknodes) {

            if(mn.addr.IsRFC1918()) continue; //local network

            if(vin == CTxIn()){
                mn.Check();
                if(mn.IsEnabled()) {
                    if(fDebug) LogPrintf("dseg - Sending banknode entry - %s \n", mn.addr.ToString().c_str());
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.now, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                }
            } else if (vin == mn.vin) {
                if(fDebug) LogPrintf("dseg - Sending banknode entry - %s \n", mn.addr.ToString().c_str());
                pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.now, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                LogPrintf("dseg - Sent 1 banknode entries to %s\n", pfrom->addr.ToString().c_str());
                return;
            }
            i++;
        }

        LogPrintf("dseg - Sent %d banknode entries to %s\n", count, pfrom->addr.ToString().c_str());
    }

    else if (strCommand == "mnget") { //Banknode Payments Request Sync
        if(fLiteMode) return; //disable all darksend/banknode related functionality



        pfrom->FulfilledRequest("mnget");
        banknodePayments.Sync(pfrom);
        LogPrintf("mnget - Sent banknode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "mnw") { //Banknode Payments Declare Winner
        //this is required in litemode
        CBanknodePaymentWinner winner;
        int a = 0;
        vRecv >> winner >> a;

        if(chainActive.Tip() == NULL) return;

        uint256 hash = winner.GetHash();
        if(mapSeenBanknodeVotes.count(hash)) {
            if(fDebug) LogPrintf("mnw - seen vote %s Height %d bestHeight %d\n", hash.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.nBlockHeight < chainActive.Tip()->nHeight - 10 || winner.nBlockHeight > chainActive.Tip()->nHeight+20){
            LogPrintf("mnw - winner out of range %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.vin.nSequence != std::numeric_limits<unsigned int>::max()){
            LogPrintf("mnw - invalid nSequence\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrintf("mnw - winning vote  %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);

        if(!banknodePayments.CheckSignature(winner)){
            LogPrintf("mnw - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSeenBanknodeVotes.insert(make_pair(hash, winner));

        if(banknodePayments.AddWinningBanknode(winner)){
            banknodePayments.Relay(winner);
        }
    }
}

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

struct CompareValueOnly2
{
    bool operator()(const pair<int64_t, int>& t1,
                    const pair<int64_t, int>& t2) const
    {
        return t1.first < t2.first;
    }
};

int CountBanknodesAboveProtocol(int protocolVersion)
{
    int i = 0;
    LOCK(cs_banknodes);
    BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
        if(mn.protocolVersion < protocolVersion) continue;
        i++;
    }

    return i;

}


int GetBanknodeByVin(CTxIn& vin)
{
    int i = 0;
    LOCK(cs_banknodes);
    BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
        if (mn.vin == vin) return i;
        i++;
    }

    return -1;
}

int GetCurrentBankNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    int i = 0;
    unsigned int score = 0;
    int winner = -1;
    LOCK(cs_banknodes);
    // scan for winner
    BOOST_FOREACH(CBankNode mn, vecBanknodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            i++;
            continue;
        }

        // calculate the score for each banknode
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = i;
        }
        i++;
    }

    return winner;
}

int GetBanknodeByRank(int findRank, int64_t nBlockHeight, int minProtocol)
{
    LOCK(cs_banknodes);
    int i = 0;

    std::vector<pair<unsigned int, int> > vecBanknodeScores;

    i = 0;
    BOOST_FOREACH(CBankNode mn, vecBanknodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            i++;
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBanknodeScores.push_back(make_pair(n2, i));
        i++;
    }

    sort(vecBanknodeScores.rbegin(), vecBanknodeScores.rend(), CompareValueOnly2());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, int)& s, vecBanknodeScores){
        rank++;
        if(rank == findRank) return s.second;
    }

    return -1;
}

int GetBanknodeRank(CTxIn& vin, int64_t nBlockHeight, int minProtocol)
{
    LOCK(cs_banknodes);
    std::vector<pair<unsigned int, CTxIn> > vecBanknodeScores;

    BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBanknodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecBanknodeScores.rbegin(), vecBanknodeScores.rend(), CompareValueOnly());

    unsigned int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecBanknodeScores){
        rank++;
        if(s.second == vin) {
            return rank;
        }
    }

    return -1;
}

//Get the last hash that matches the modulus given. Processed in reverse order
bool GetBlockHash(uint256& hash, int nBlockHeight)
{
    if (chainActive.Tip() == NULL) return false;

    if(nBlockHeight == 0)
        nBlockHeight = chainActive.Tip()->nHeight;

    if(mapCacheBlockHashes.count(nBlockHeight)){
        hash = mapCacheBlockHashes[nBlockHeight];
        return true;
    }

    const CBlockIndex *BlockLastSolved = chainActive.Tip();
    const CBlockIndex *BlockReading = chainActive.Tip();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || chainActive.Tip()->nHeight+1 < nBlockHeight) return false;

    int nBlocksAgo = 0;
    if(nBlockHeight > 0) nBlocksAgo = (chainActive.Tip()->nHeight+1)-nBlockHeight;
    assert(nBlocksAgo >= 0);

    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nBlocksAgo){
            hash = BlockReading->GetBlockHash();
            mapCacheBlockHashes[nBlockHeight] = hash;
            return true;
        }
        n++;

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return false;
}

//
// Deterministically calculate a given "score" for a banknode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CBankNode::CalculateScore(int mod, int64_t nBlockHeight)
{
    if(chainActive.Tip() == NULL) return 0;

    uint256 hash = 0;
    uint256 aux = vin.prevout.hash + vin.prevout.n;

    if(!GetBlockHash(hash, nBlockHeight)) return 0;

    uint256 hash2 = Hash(BEGIN(hash), END(hash));
    uint256 hash3 = Hash(BEGIN(hash), END(aux));

    uint256 r = (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);

    return r;
}

void CBankNode::Check()
{
    //once spent, stop doing the checks
    if(enabled==3) return;


    if(!UpdatedWithin(BANKNODE_REMOVAL_SECONDS)){
        enabled = 4;
        return;
    }

    if(!UpdatedWithin(BANKNODE_EXPIRATION_SECONDS)){
        enabled = 2;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CTransaction();
        CTxOut vout = CTxOut(249999.99*COIN, darkSendPool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        if(!AcceptableInputs(mempool, state, tx)){
            enabled = 3;
            return;
        }
    }

    enabled = 1; // OK
}

bool CBanknodePayments::CheckSignature(CBanknodePaymentWinner& winner)
{
    //note: need to investigate why this is failing
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();
    std::string strPubKey = strMainPubKey ;
    CPubKey pubkey(ParseHex(strPubKey));

    std::string errorMessage = "";
    if(!darkSendSigner.VerifyMessage(pubkey, winner.vchSig, strMessage, errorMessage)){
        return false;
    }

    return true;
}

bool CBanknodePayments::Sign(CBanknodePaymentWinner& winner)
{
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CBanknodePayments::Sign - ERROR: Invalid banknodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, winner.vchSig, key2)) {
        LogPrintf("CBanknodePayments::Sign - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, winner.vchSig, strMessage, errorMessage)) {
        LogPrintf("CBanknodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

uint64_t CBanknodePayments::CalculateScore(uint256 blockHash, CTxIn& vin)
{
    uint256 n1 = blockHash;
    uint256 n2 = Hash(BEGIN(n1), END(n1));
    uint256 n3 = Hash(BEGIN(vin.prevout.hash), END(vin.prevout.hash));
    uint256 n4 = n3 > n2 ? (n3 - n2) : (n2 - n3);

    //printf(" -- CBanknodePayments CalculateScore() n2 = %d \n", n2.Get64());
    //printf(" -- CBanknodePayments CalculateScore() n3 = %d \n", n3.Get64());
    //printf(" -- CBanknodePayments CalculateScore() n4 = %d \n", n4.Get64());

    return n4.Get64();
}

bool CBanknodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    BOOST_FOREACH(CBanknodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            payee = winner.payee;
            return true;
        }
    }

    return false;
}

bool CBanknodePayments::GetWinningBanknode(int nBlockHeight, CTxIn& vinOut)
{
    BOOST_FOREACH(CBanknodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            vinOut = winner.vin;
            return true;
        }
    }

    return false;
}

bool CBanknodePayments::AddWinningBanknode(CBanknodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-576)) {
        return false;
    }

    winnerIn.score = CalculateScore(blockHash, winnerIn.vin);

    bool foundBlock = false;
    BOOST_FOREACH(CBanknodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == winnerIn.nBlockHeight) {
            foundBlock = true;
            if(winner.score < winnerIn.score){
                winner.score = winnerIn.score;
                winner.vin = winnerIn.vin;
                winner.payee = winnerIn.payee;
                winner.vchSig = winnerIn.vchSig;

                return true;
            }
        }
    }

    // if it's not in the vector
    if(!foundBlock){
        vWinning.push_back(winnerIn);
        mapSeenBanknodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

        return true;
    }

    return false;
}

void CBanknodePayments::CleanPaymentList()
{
    LOCK(cs_banknodes);
    if(chainActive.Tip() == NULL) return;

    int nLimit = std::max(((int)vecBanknodes.size())*2, 1000);

    vector<CBanknodePaymentWinner>::iterator it;
    for(it=vWinning.begin();it<vWinning.end();it++){
        if(chainActive.Tip()->nHeight - (*it).nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CBanknodePayments::CleanPaymentList - Removing old banknode payment - block %d\n", (*it).nBlockHeight);
            vWinning.erase(it);
            break;
        }
    }
}

bool CBanknodePayments::ProcessBlock(int nBlockHeight)
{
    LOCK(cs_banknodes);
    if(!enabled) return false;
    CBanknodePaymentWinner winner;

    std::vector<CTxIn> vecLastPayments;
    int c = 0;
    BOOST_REVERSE_FOREACH(CBanknodePaymentWinner& winner, vWinning){
        vecLastPayments.push_back(winner.vin);
        //if we have one full payment cycle, break
        if(++c > (int)vecBanknodes.size()) break;
    }

    std::random_shuffle ( vecBanknodes.begin(), vecBanknodes.end() );
    BOOST_FOREACH(CBankNode& mn, vecBanknodes) {
        bool found = false;
        BOOST_FOREACH(CTxIn& vin, vecLastPayments)
            if(mn.vin == vin) found = true;

        if(found) continue;

        mn.Check();
        if(!mn.IsEnabled()) {
            continue;
        }

        winner.score = 0;
        winner.nBlockHeight = nBlockHeight;
        winner.vin = mn.vin;
        winner.payee =GetScriptForDestination(mn.pubkey.GetID());

        break;
    }

    //if we can't find someone to get paid, pick randomly
    if(winner.nBlockHeight == 0 && vecBanknodes.size() > 0) {
        winner.score = 0;
        winner.nBlockHeight = nBlockHeight;
        winner.vin = vecBanknodes[0].vin;
        winner.payee =GetScriptForDestination(vecBanknodes[0].pubkey.GetID());
    }

    if(Sign(winner)){
        if(AddWinningBanknode(winner)){
            Relay(winner);
            return true;
        }
    }

    return false;
}

void CBanknodePayments::Relay(CBanknodePaymentWinner& winner)
{
    CInv inv(MSG_BANKNODE_WINNER, winner.GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

void CBanknodePayments::Sync(CNode* node)
{
    int a = 0;
    BOOST_FOREACH(CBanknodePaymentWinner& winner, vWinning)
        if(winner.nBlockHeight >= chainActive.Tip()->nHeight-10 && winner.nBlockHeight <= chainActive.Tip()->nHeight + 20)
            node->PushMessage("mnw", winner, a);
}


bool CBanknodePayments::SetPrivKey(std::string strPrivKey)
{
    CBanknodePaymentWinner winner;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(winner);

    if(CheckSignature(winner)){
        LogPrintf("CBanknodePayments::SetPrivKey - Successfully initialized as banknode payments master\n");
        enabled = true;
        return true;
    } else {
        return false;
    }
}
