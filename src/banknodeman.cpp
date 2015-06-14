// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "banknodeman.h"
#include "banknode.h"
#include "activebanknode.h"
#include "darksend.h"
#include "core.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

CCriticalSection cs_process_message;

/** Banknode manager */
CBanknodeMan mnodeman;

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};
struct CompareValueOnlyMN
{
    bool operator()(const pair<int64_t, CBanknode>& t1,
                    const pair<int64_t, CBanknode>& t2) const
    {
        return t1.first < t2.first;
    }
};

//
// CBanknodeDB
//

CBanknodeDB::CBanknodeDB()
{
    pathMN = GetDataDir() / "mncache.dat";
    strMagicMessage = "BanknodeCache";
}

bool CBanknodeDB::Write(const CBanknodeMan& mnodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssBanknodes(SER_DISK, CLIENT_VERSION);
    ssBanknodes << strMagicMessage; // banknode cache file specific magic message
    ssBanknodes << FLATDATA(Params().MessageStart()); // network specific magic number
    ssBanknodes << mnodemanToSave;
    uint256 hash = Hash(ssBanknodes.begin(), ssBanknodes.end());
    ssBanknodes << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssBanknodes;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout.Get());
    fileout.fclose();

    LogPrintf("Written info to mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToSave.ToString());

    return true;
}

CBanknodeDB::ReadResult CBanknodeDB::Read(CBanknodeMan& mnodemanToLoad)
{
    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s : Failed to open file %s", __func__, pathMN.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathMN);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssBanknodes(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssBanknodes.begin(), ssBanknodes.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (banknode cache file specific magic message) and ..

        ssBanknodes >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid banknode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssBanknodes >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CBanknodeMan object
        ssBanknodes >> mnodemanToLoad;
    }
    catch (std::exception &e) {
        mnodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    mnodemanToLoad.CheckAndRemove(); // clean out expired
    LogPrintf("Loaded info from mncache.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", mnodemanToLoad.ToString());

    return Ok;
}

void DumpBanknodes()
{
    int64_t nStart = GetTimeMillis();

    CBanknodeDB mndb;
    CBanknodeMan tempMnodeman;

    LogPrintf("Verifying mncache.dat format...\n");
    CBanknodeDB::ReadResult readResult = mndb.Read(tempMnodeman);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CBanknodeDB::FileError)
        LogPrintf("Missing banknode cache file - mncache.dat, will try to recreate\n");
    else if (readResult != CBanknodeDB::Ok)
    {
        LogPrintf("Error reading mncache.dat: ");
        if(readResult == CBanknodeDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mncache.dat...\n");
    mndb.Write(mnodeman);

    LogPrintf("Banknode dump finished  %dms\n", GetTimeMillis() - nStart);
}

CBanknodeMan::CBanknodeMan() {
    nDsqCount = 0;
}

bool CBanknodeMan::Add(CBanknode &mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CBanknode *pmn = Find(mn.vin);

    if (pmn == NULL)
    {
        if(fDebug) LogPrintf("CBanknodeMan: Adding new Banknode %s - %i now\n", mn.addr.ToString().c_str(), size() + 1);
        vBanknodes.push_back(mn);
        return true;
    }

    return false;
}

void CBanknodeMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CBanknode& mn, vBanknodes)
        mn.Check();
}

void CBanknodeMan::CheckAndRemove()
{
    LOCK(cs);

    Check();

    //remove inactive
    vector<CBanknode>::iterator it = vBanknodes.begin();
    while(it != vBanknodes.end()){
        if((*it).activeState == CBanknode::BANKNODE_REMOVE || (*it).activeState == CBanknode::BANKNODE_VIN_SPENT){
            if(fDebug) LogPrintf("CBanknodeMan: Removing inactive Banknode %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            it = vBanknodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Banknode list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForBanknodeList.begin();
    while(it1 != mAskedUsForBanknodeList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForBanknodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Banknode list
    it1 = mWeAskedForBanknodeList.begin();
    while(it1 != mWeAskedForBanknodeList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForBanknodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Banknodes we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForBanknodeListEntry.begin();
    while(it2 != mWeAskedForBanknodeListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForBanknodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

}

void CBanknodeMan::Clear()
{
    LOCK(cs);
    vBanknodes.clear();
    mAskedUsForBanknodeList.clear();
    mWeAskedForBanknodeList.clear();
    mWeAskedForBanknodeListEntry.clear();
    nDsqCount = 0;
}

int CBanknodeMan::CountEnabled()
{
    int i = 0;

    BOOST_FOREACH(CBanknode& mn, vBanknodes) {
        mn.Check();
        if(mn.IsEnabled()) i++;
    }

    return i;
}

int CBanknodeMan::CountBanknodesAboveProtocol(int protocolVersion)
{
    int i = 0;

    BOOST_FOREACH(CBanknode& mn, vBanknodes) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CBanknodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    std::map<CNetAddr, int64_t>::iterator it = mWeAskedForBanknodeList.find(pnode->addr);
    if (it != mWeAskedForBanknodeList.end())
    {
        if (GetTime() < (*it).second) {
            LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
            return;
        }
    }
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + BANKNODES_DSEG_SECONDS;
    mWeAskedForBanknodeList[pnode->addr] = askAgain;
}

CBanknode *CBanknodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CBanknode& mn, vBanknodes)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}


CBanknode *CBanknodeMan::Find(const CPubKey &pubKeyBanknode)
{
    LOCK(cs);

    BOOST_FOREACH(CBanknode& mn, vBanknodes)
    {
        if(mn.pubkey2 == pubKeyBanknode)
            return &mn;
    }
    return NULL;
}


CBanknode* CBanknodeMan::FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds)
{
    LOCK(cs);

    CBanknode *pOldestBanknode = NULL;

    BOOST_FOREACH(CBanknode &mn, vBanknodes)
    {
        mn.Check();
        if(!mn.IsEnabled()) continue;

        
            if(mn.GetBanknodeInputAge() < nMinimumAge || mn.lastTimeSeen - mn.sigTime < nMinimumActiveSeconds) continue;
        

        bool found = false;
        BOOST_FOREACH(const CTxIn& vin, vVins)
            if(mn.vin.prevout == vin.prevout)
            {
                found = true;
                break;
            }

        if(found) continue;

        if(pOldestBanknode == NULL || pOldestBanknode->GetBanknodeInputAge() < mn.GetBanknodeInputAge()){
            pOldestBanknode = &mn;
        }
    }

    return pOldestBanknode;
}

CBanknode *CBanknodeMan::FindRandom()
{
    LOCK(cs);

    if(size() == 0) return NULL;

    return &vBanknodes[GetRandInt(vBanknodes.size())];
}

CBanknode* CBanknodeMan::GetCurrentBankNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    unsigned int score = 0;
    CBanknode* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CBanknode& mn, vBanknodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Banknode
        uint256 n = mn.CalculateScore(mod, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        // determine the winner
        if(n2 > score){
            score = n2;
            winner = &mn;
        }
    }

    return winner;
}

int CBanknodeMan::GetBanknodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecBanknodeScores;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CBanknode& mn, vBanknodes) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBanknodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecBanknodeScores.rbegin(), vecBanknodeScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecBanknodeScores){
        rank++;
        if(s.second == vin) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CBanknode> > CBanknodeMan::GetBanknodeRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<unsigned int, CBanknode> > vecBanknodeScores;
    std::vector<pair<int, CBanknode> > vecBanknodeRanks;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return vecBanknodeRanks;

    // scan for winner
    BOOST_FOREACH(CBanknode& mn, vBanknodes) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBanknodeScores.push_back(make_pair(n2, mn));
    }

    sort(vecBanknodeScores.rbegin(), vecBanknodeScores.rend(), CompareValueOnlyMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CBanknode)& s, vecBanknodeScores){
        rank++;
        vecBanknodeRanks.push_back(make_pair(rank, s.second));
    }

    return vecBanknodeRanks;
}

CBanknode* CBanknodeMan::GetBanknodeByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecBanknodeScores;

    // scan for winner
    BOOST_FOREACH(CBanknode& mn, vBanknodes) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBanknodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecBanknodeScores.rbegin(), vecBanknodeScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecBanknodeScores){
        rank++;
        if(rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CBanknodeMan::ProcessBanknodeConnections()
{
    //we don't care about this for regtest
    
    LOCK(cs_vNodes);

    if(!darkSendPool.pSubmittedToBanknode) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(darkSendPool.pSubmittedToBanknode->addr == pnode->addr) continue;

        if(pnode->fDarkSendMaster){
            LogPrintf("Closing Banknode connection %s \n", pnode->addr.ToString().c_str());
            pnode->CloseSocketDisconnect();
        }
    }
}

void CBanknodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if(fLiteMode) return; //disable all Darksend/Banknode related functionality
    if(IsInitialBlockDownload()) return;

    LOCK(cs_process_message);

    if (strCommand == "dsee") { //DarkSend Election Entry

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
        vRecv >> vin >> addr >> vchSig >> sigTime >> pubkey >> pubkey2 >> count >> current >> lastUpdated >> protocolVersion ;

        // make sure signature isn't in the future (past is OK)
        if (sigTime > GetAdjustedTime() + 60 * 60) {
            LogPrintf("dsee - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
            return;
        }

        bool isLocal = addr.IsRFC1918() || addr.IsLocal();
        
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

        strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion) ;

        if(protocolVersion < MIN_MN_PROTO_VERSION) {
            LogPrintf("dsee - ignoring outdated Banknode %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
            return;
        }

        CScript pubkeyScript;
        pubkeyScript= GetScriptForDestination(pubkey.GetID());

        if(pubkeyScript.size() != 25) {
            LogPrintf("dsee - pubkey the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        CScript pubkeyScript2;
        pubkeyScript2= GetScriptForDestination(pubkey2.GetID());

        if(pubkeyScript2.size() != 25) {
            LogPrintf("dsee - pubkey2 the wrong size\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(!vin.scriptSig.empty()) {
            LogPrintf("dsee - Ignore Not Empty ScriptSig %s\n",vin.ToString().c_str());
            return;
        }

        std::string errorMessage = "";
        if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)){
            LogPrintf("dsee - Got bad Banknode address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        //search existing Banknode list, this is where we update existing Banknodes with new dsee broadcasts
        CBanknode* pmn = this->Find(vin);
        // if we are banknode but with undefined vin and this dsee is ours (matches our Banknode privkey) then just skip this part
        if(pmn != NULL && !(fBankNode && activeBanknode.vin == CTxIn() && pubkey2 == activeBanknode.pubKeyBanknode))
        {
            // count == -1 when it's a new entry
            //   e.g. We don't want the entry relayed/time updated when we're syncing the list
            // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
            //   after that they just need to match
            if(count == -1 && pmn->pubkey == pubkey && !pmn->UpdatedWithin(BANKNODE_MIN_DSEE_SECONDS)){
                pmn->UpdateLastSeen();

                if(pmn->sigTime < sigTime){ //take the newest entry
                    LogPrintf("dsee - Got updated entry for %s\n", addr.ToString().c_str());
                    pmn->pubkey2 = pubkey2;
                    pmn->sigTime = sigTime;
                    pmn->sig = vchSig;
                    pmn->protocolVersion = protocolVersion;
                    pmn->addr = addr;
                    pmn->Check();
                    if(pmn->IsEnabled())
                        mnodeman.RelayBanknodeEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
                }
            }

            return;
        }

        // make sure the vout that was signed is related to the transaction that spawned the Banknode
        //  - this is expensive, so it's only done once per Banknode
        if(!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(fDebug) LogPrintf("dsee - Got NEW Banknode entry %s\n", addr.ToString().c_str());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()

        CValidationState state;
        CMutableTransaction tx = CTransaction();
        if (chainActive.Tip()->nHeight<150000) {
        CTxOut vout = CTxOut(249999.99*COIN, darkSendPool.collateralPubKey);
                tx.vin.push_back(vin);
        tx.vout.push_back(vout);
		}else {
        CTxOut vout = CTxOut(49999.99*COIN, darkSendPool.collateralPubKey);
                tx.vin.push_back(vin);
        tx.vout.push_back(vout);
		}

        if(AcceptableInputs(mempool, state, tx)){
            if(fDebug) LogPrintf("dsee - Accepted Banknode entry %i %i\n", count, current);

            if(GetInputAge(vin) < BANKNODE_MIN_CONFIRMATIONS){
                LogPrintf("dsee - Input must have least %d confirmations\n", BANKNODE_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }

            // verify that sig time is legit in past
            // should be at least not earlier than block when 250K BCR tx got BANKNODE_MIN_CONFIRMATIONS
            uint256 hashBlock = 0;
            CTransaction txVin;
            GetTransaction(vin.prevout.hash, txVin, hashBlock, true);
            BlockMap::const_iterator t = mapBlockIndex.find(hashBlock);
            if (t != mapBlockIndex.end() && (*t).second)
            {
                CBlockIndex* pMNIndex = (*t).second; // block for 250K BCR tx -> 1 confirmation
                CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + BANKNODE_MIN_CONFIRMATIONS - 1]; // block where tx got BANKNODE_MIN_CONFIRMATIONS
                if(pConfIndex->GetBlockTime() > sigTime)
                {
                    LogPrintf("dsee - Bad sigTime %d for Banknode %20s %105s (%i conf block is at %d)\n",
                              sigTime, addr.ToString(), vin.ToString(), BANKNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                    return;
                }
            }


            // use this as a peer
            addrman.Add(CAddress(addr), pfrom->addr, 2*60*60);

            //doesn't support multisig addresses

            // add our Banknode
            CBanknode mn(addr, vin, pubkey, vchSig, sigTime, pubkey2, protocolVersion);
            mn.UpdateLastSeen(lastUpdated);
            this->Add(mn);

            // if it matches our Banknode privkey, then we've been remotely activated
            if(pubkey2 == activeBanknode.pubKeyBanknode && protocolVersion == PROTOCOL_VERSION){
                activeBanknode.EnableHotColdBankNode(vin, addr);
            }

            if(count == -1 && !isLocal)
                mnodeman.RelayBanknodeEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);

        } else {
            LogPrintf("dsee - Rejected Banknode entry %s\n", addr.ToString().c_str());

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

        // see if we have this Banknode
        CBanknode* pmn = this->Find(vin);
        if(pmn != NULL && pmn->protocolVersion >= MIN_MN_PROTO_VERSION)
        {
            // LogPrintf("dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            // take this only if it's newer
            if(pmn->lastDseep < sigTime)
            {
                std::string strMessage = pmn->addr.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(stop);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("dseep - Got bad Banknode address signature %s \n", vin.ToString().c_str());
                    //Misbehaving(pfrom->GetId(), 100);
                    return;
                }

                pmn->lastDseep = sigTime;

                if(!pmn->UpdatedWithin(BANKNODE_MIN_DSEEP_SECONDS))
                {
                    if(stop) pmn->Disable();
                    else
                    {
                        pmn->UpdateLastSeen();
                        pmn->Check();
                        if(!pmn->IsEnabled()) return;
                    }
                    mnodeman.RelayBanknodeEntryPing(vin, vchSig, sigTime, stop);
                }
            }
            return;
        }

        if(fDebug) LogPrintf("dseep - Couldn't find Banknode entry %s\n", vin.ToString().c_str());

        std::map<COutPoint, int64_t>::iterator i = mWeAskedForBanknodeListEntry.find(vin.prevout);
        if (i != mWeAskedForBanknodeListEntry.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t) return; // we've asked recently
        }

        // ask for the dsee info once from the node that sent dseep

        LogPrintf("dseep - Asking source node for missing entry %s\n", vin.ToString().c_str());
        pfrom->PushMessage("dseg", vin);
        int64_t askAgain = GetTime() + BANKNODE_MIN_DSEEP_SECONDS;
        mWeAskedForBanknodeListEntry[vin.prevout] = askAgain;

    } else if (strCommand == "mvote") { //Banknode Vote

        CTxIn vin;
        vector<unsigned char> vchSig;
        int nVote;
        vRecv >> vin >> vchSig >> nVote;

        // see if we have this Banknode
        CBanknode* pmn = this->Find(vin);
        if(pmn != NULL)
        {
            if((GetAdjustedTime() - pmn->lastVote) > (60*60))
            {
                std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nVote);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("mvote - Got bad Banknode address signature %s \n", vin.ToString().c_str());
                    return;
                }

                pmn->nVote = nVote;
                pmn->lastVote = GetAdjustedTime();

                //send to all peers
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    pnode->PushMessage("mvote", vin, vchSig, nVote);
            }

            return;
        }

    } else if (strCommand == "dseg") { //Get Banknode list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            if(!pfrom->addr.IsRFC1918())
            {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForBanknodeList.find(pfrom->addr);
                if (i != mAskedUsForBanknodeList.end())
                {
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("dseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + BANKNODES_DSEG_SECONDS;
                mAskedUsForBanknodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok

        int count = this->size();
        int i = 0;

        BOOST_FOREACH(CBanknode& mn, vBanknodes) {

            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled())
            {
                if(fDebug) LogPrintf("dseg - Sending Banknode entry - %s \n", mn.addr.ToString().c_str());
                if(vin == CTxIn()){
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                } else if (vin == mn.vin) {
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                    LogPrintf("dseg - Sent 1 Banknode entries to %s\n", pfrom->addr.ToString().c_str());
                    return;
                }
                i++;
            }
        }

        LogPrintf("dseg - Sent %d Banknode entries to %s\n", i, pfrom->addr.ToString().c_str());
    }

}

void CBanknodeMan::RelayBanknodeEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dsee", vin, addr, vchSig, nNow, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
}

void CBanknodeMan::RelayBanknodeEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dseep", vin, vchSig, nNow, stop);
}

void CBanknodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CBanknode>::iterator it = vBanknodes.begin();
    while(it != vBanknodes.end()){
        if((*it).vin == vin){
            if(fDebug) LogPrintf("CBanknodeMan: Removing Banknode %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            vBanknodes.erase(it);
            break;
        }
    }
}

std::string CBanknodeMan::ToString() const
{
    std::ostringstream info;

    info << "Banknodes: " << (int)vBanknodes.size() <<
            ", peers who asked us for Banknode list: " << (int)mAskedUsForBanknodeList.size() <<
            ", peers we asked for Banknode list: " << (int)mWeAskedForBanknodeList.size() <<
            ", entries in Banknode list we asked for: " << (int)mWeAskedForBanknodeListEntry.size() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}
