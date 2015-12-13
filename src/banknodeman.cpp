// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "basenodeman.h"
#include "basenode.h"
#include "activebasenode.h"
#include "darksend.h"
#include "core.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

CCriticalSection cs_process_message;

/** Basenode manager */
CBasenodeMan mnodeman;

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
    bool operator()(const pair<int64_t, CBasenode>& t1,
                    const pair<int64_t, CBasenode>& t2) const
    {
        return t1.first < t2.first;
    }
};

//
// CBasenodeDB
//

CBasenodeDB::CBasenodeDB()
{
    pathMN = GetDataDir() / "nodecache.dat";
    strMagicMessage = "BasenodeCache";
}

bool CBasenodeDB::Write(const CBasenodeMan& mnodemanToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssBasenodes(SER_DISK, CLIENT_VERSION);
    ssBasenodes << strMagicMessage; // basenode cache file specific magic message
    ssBasenodes << FLATDATA(Params().MessageStart()); // network specific magic number
    ssBasenodes << mnodemanToSave;
    uint256 hash = Hash(ssBasenodes.begin(), ssBasenodes.end());
    ssBasenodes << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathMN.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathMN.string());

    // Write and commit header, data
    try {
        fileout << ssBasenodes;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    FileCommit(fileout.Get());
    fileout.fclose();

    if(fDebug)LogPrintf("Written info to nodecache.dat  %dms\n", GetTimeMillis() - nStart);
    if(fDebug)LogPrintf("  %s\n", mnodemanToSave.ToString());

    return true;
}

CBasenodeDB::ReadResult CBasenodeDB::Read(CBasenodeMan& mnodemanToLoad)
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

    CDataStream ssBasenodes(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssBasenodes.begin(), ssBasenodes.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }

    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (basenode cache file specific magic message) and ..

        ssBasenodes >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid basenode cache magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        ssBasenodes >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }
        // de-serialize data into CBasenodeMan object
        ssBasenodes >> mnodemanToLoad;
    }
    catch (std::exception &e) {
        mnodemanToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    mnodemanToLoad.CheckAndRemove(); // clean out expired
    if(fDebug)LogPrintf("Loaded info from nodecache.dat  %dms\n", GetTimeMillis() - nStart);
    if(fDebug)LogPrintf("  %s\n", mnodemanToLoad.ToString());

    return Ok;
}

void DumpBasenodes()
{
    int64_t nStart = GetTimeMillis();

    CBasenodeDB mndb;
    CBasenodeMan tempMnodeman;

    if(fDebug)LogPrintf("Verifying nodecache.dat format...\n");
    CBasenodeDB::ReadResult readResult = mndb.Read(tempMnodeman);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CBasenodeDB::FileError)
        LogPrintf("Missing basenode cache file - nodecache.dat, will try to recreate\n");
    else if (readResult != CBasenodeDB::Ok)
    {
        LogPrintf("Error reading nodecache.dat: ");
        if(readResult == CBasenodeDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    if(fDebug)LogPrintf("Writting info to nodecache.dat...\n");
    mndb.Write(mnodeman);

    if(fDebug)LogPrintf("Basenode dump finished  %dms\n", GetTimeMillis() - nStart);
}

CBasenodeMan::CBasenodeMan() {
    nDsqCount = 0;
}

bool CBasenodeMan::Add(CBasenode &mn)
{
    LOCK(cs);

    if (!mn.IsEnabled())
        return false;

    CBasenode *pmn = Find(mn.vin);

    if (pmn == NULL)
    {
        if(fDebug) LogPrintf("CBasenodeMan: Adding new Basenode %s - %i now\n", mn.addr.ToString().c_str(), size() + 1);
        vBasenodes.push_back(mn);
        return true;
    }

    return false;
}

void CBasenodeMan::Check()
{
    LOCK(cs);

    BOOST_FOREACH(CBasenode& mn, vBasenodes)
        mn.Check();
}

void CBasenodeMan::CheckAndRemove()
{
    LOCK(cs);

    Check();

    //remove inactive
    vector<CBasenode>::iterator it = vBasenodes.begin();
    while(it != vBasenodes.end()){
        if((*it).activeState == CBasenode::BASENODE_REMOVE || (*it).activeState == CBasenode::BASENODE_VIN_SPENT){
            if(fDebug) LogPrintf("CBasenodeMan: Removing inactive Basenode %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            it = vBasenodes.erase(it);
        } else {
            ++it;
        }
    }

    // check who's asked for the Basenode list
    map<CNetAddr, int64_t>::iterator it1 = mAskedUsForBasenodeList.begin();
    while(it1 != mAskedUsForBasenodeList.end()){
        if((*it1).second < GetTime()) {
            mAskedUsForBasenodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check who we asked for the Basenode list
    it1 = mWeAskedForBasenodeList.begin();
    while(it1 != mWeAskedForBasenodeList.end()){
        if((*it1).second < GetTime()){
            mWeAskedForBasenodeList.erase(it1++);
        } else {
            ++it1;
        }
    }

    // check which Basenodes we've asked for
    map<COutPoint, int64_t>::iterator it2 = mWeAskedForBasenodeListEntry.begin();
    while(it2 != mWeAskedForBasenodeListEntry.end()){
        if((*it2).second < GetTime()){
            mWeAskedForBasenodeListEntry.erase(it2++);
        } else {
            ++it2;
        }
    }

}

void CBasenodeMan::Clear()
{
    LOCK(cs);
    vBasenodes.clear();
    mAskedUsForBasenodeList.clear();
    mWeAskedForBasenodeList.clear();
    mWeAskedForBasenodeListEntry.clear();
    nDsqCount = 0;
}

int CBasenodeMan::CountEnabled()
{
    int i = 0;

    BOOST_FOREACH(CBasenode& mn, vBasenodes) {
        mn.Check();
        if(mn.IsEnabled()) i++;
    }

    return i;
}

int CBasenodeMan::CountBasenodesAboveProtocol(int protocolVersion)
{
    int i = 0;

    BOOST_FOREACH(CBasenode& mn, vBasenodes) {
        mn.Check();
        if(mn.protocolVersion < protocolVersion || !mn.IsEnabled()) continue;
        i++;
    }

    return i;
}

void CBasenodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    std::map<CNetAddr, int64_t>::iterator it = mWeAskedForBasenodeList.find(pnode->addr);
    if (it != mWeAskedForBasenodeList.end())
    {
        if (GetTime() < (*it).second) {
            LogPrintf("dseg - we already asked %s for the list; skipping...\n", pnode->addr.ToString());
            return;
        }
    }
    pnode->PushMessage("dseg", CTxIn());
    int64_t askAgain = GetTime() + BASENODES_DSEG_SECONDS;
    mWeAskedForBasenodeList[pnode->addr] = askAgain;
}

CBasenode *CBasenodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CBasenode& mn, vBasenodes)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}


CBasenode *CBasenodeMan::Find(const CPubKey &pubKeyBasenode)
{
    LOCK(cs);

    BOOST_FOREACH(CBasenode& mn, vBasenodes)
    {
        if(mn.pubkey2 == pubKeyBasenode)
            return &mn;
    }
    return NULL;
}


CBasenode* CBasenodeMan::FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds)
{
    LOCK(cs);

    CBasenode *pOldestBasenode = NULL;

    BOOST_FOREACH(CBasenode &mn, vBasenodes)
    {
        mn.Check();
        if(!mn.IsEnabled()) continue;

        
            if(mn.GetBasenodeInputAge() < nMinimumAge || mn.lastTimeSeen - mn.sigTime < nMinimumActiveSeconds) continue;
        

        bool found = false;
        BOOST_FOREACH(const CTxIn& vin, vVins)
            if(mn.vin.prevout == vin.prevout)
            {
                found = true;
                break;
            }

        if(found) continue;

        if(pOldestBasenode == NULL || pOldestBasenode->GetBasenodeInputAge() < mn.GetBasenodeInputAge()){
            pOldestBasenode = &mn;
        }
    }

    return pOldestBasenode;
}

CBasenode *CBasenodeMan::FindRandom()
{
    LOCK(cs);

    if(size() == 0) return NULL;

    return &vBasenodes[GetRandInt(vBasenodes.size())];
}

CBasenode* CBasenodeMan::GetCurrentBaseNode(int mod, int64_t nBlockHeight, int minProtocol)
{
    unsigned int score = 0;
    CBasenode* winner = NULL;

    // scan for winner
    BOOST_FOREACH(CBasenode& mn, vBasenodes) {
        mn.Check();
        if(mn.protocolVersion < minProtocol || !mn.IsEnabled()) continue;

        // calculate the score for each Basenode
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

int CBasenodeMan::GetBasenodeRank(const CTxIn& vin, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecBasenodeScores;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return -1;

    // scan for winner
    BOOST_FOREACH(CBasenode& mn, vBasenodes) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBasenodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecBasenodeScores.rbegin(), vecBasenodeScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecBasenodeScores){
        rank++;
        if(s.second == vin) {
            return rank;
        }
    }

    return -1;
}

std::vector<pair<int, CBasenode> > CBasenodeMan::GetBasenodeRanks(int64_t nBlockHeight, int minProtocol)
{
    std::vector<pair<unsigned int, CBasenode> > vecBasenodeScores;
    std::vector<pair<int, CBasenode> > vecBasenodeRanks;

    //make sure we know about this block
    uint256 hash = 0;
    if(!GetBlockHash(hash, nBlockHeight)) return vecBasenodeRanks;

    // scan for winner
    BOOST_FOREACH(CBasenode& mn, vBasenodes) {

        mn.Check();

        if(mn.protocolVersion < minProtocol) continue;
        if(!mn.IsEnabled()) {
            continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBasenodeScores.push_back(make_pair(n2, mn));
    }

    sort(vecBasenodeScores.rbegin(), vecBasenodeScores.rend(), CompareValueOnlyMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CBasenode)& s, vecBasenodeScores){
        rank++;
        vecBasenodeRanks.push_back(make_pair(rank, s.second));
    }

    return vecBasenodeRanks;
}

CBasenode* CBasenodeMan::GetBasenodeByRank(int nRank, int64_t nBlockHeight, int minProtocol, bool fOnlyActive)
{
    std::vector<pair<unsigned int, CTxIn> > vecBasenodeScores;

    // scan for winner
    BOOST_FOREACH(CBasenode& mn, vBasenodes) {

        if(mn.protocolVersion < minProtocol) continue;
        if(fOnlyActive) {
            mn.Check();
            if(!mn.IsEnabled()) continue;
        }

        uint256 n = mn.CalculateScore(1, nBlockHeight);
        unsigned int n2 = 0;
        memcpy(&n2, &n, sizeof(n2));

        vecBasenodeScores.push_back(make_pair(n2, mn.vin));
    }

    sort(vecBasenodeScores.rbegin(), vecBasenodeScores.rend(), CompareValueOnly());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(unsigned int, CTxIn)& s, vecBasenodeScores){
        rank++;
        if(rank == nRank) {
            return Find(s.second);
        }
    }

    return NULL;
}

void CBasenodeMan::ProcessBasenodeConnections()
{
    //we don't care about this for regtest
    
    LOCK(cs_vNodes);

    if(!darkSendPool.pSubmittedToBasenode) return;

    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(darkSendPool.pSubmittedToBasenode->addr == pnode->addr) continue;

        if(pnode->fDarkSendMaster){
            LogPrintf("Closing Basenode connection %s \n", pnode->addr.ToString().c_str());
            pnode->CloseSocketDisconnect();
        }
    }
}

void CBasenodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

    if(fLiteMode) return; //disable all Darksend/Basenode related functionality
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

        if(protocolVersion < nBasenodeMinProtocol) {
            LogPrintf("dsee - ignoring outdated Basenode %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
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
            LogPrintf("dsee - Got bad Basenode address signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        //search existing Basenode list, this is where we update existing Basenodes with new dsee broadcasts
        CBasenode* pmn = this->Find(vin);
        // if we are basenode but with undefined vin and this dsee is ours (matches our Basenode privkey) then just skip this part
        if(pmn != NULL && !(fBaseNode && activeBasenode.vin == CTxIn() && pubkey2 == activeBasenode.pubKeyBasenode))
        {
            // count == -1 when it's a new entry
            //   e.g. We don't want the entry relayed/time updated when we're syncing the list
            // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
            //   after that they just need to match
            if(count == -1 && pmn->pubkey == pubkey && !pmn->UpdatedWithin(BASENODE_MIN_DSEE_SECONDS)){
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
                        mnodeman.RelayBasenodeEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
                }
            }

            return;
        }

        // make sure the vout that was signed is related to the transaction that spawned the Basenode
        //  - this is expensive, so it's only done once per Basenode
        if(!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
            LogPrintf("dsee - Got mismatched pubkey and vin\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(fDebug) LogPrintf("dsee - Got NEW Basenode entry %s\n", addr.ToString().c_str());

        // make sure it's still unspent
        //  - this is checked later by .check() in many places and by ThreadCheckDarkSendPool()

        CValidationState state;
        CMutableTransaction tx = CTransaction();
        if (chainActive.Tip()->nHeight<145000) {
        CTxOut vout = CTxOut(249999.99*COIN, darkSendPool.collateralPubKey);
                tx.vin.push_back(vin);
        tx.vout.push_back(vout);
		}else {
        CTxOut vout = CTxOut(49999.99*COIN, darkSendPool.collateralPubKey);
                tx.vin.push_back(vin);
        tx.vout.push_back(vout);
		}

        if(AcceptableInputs(mempool, state, tx)){
            if(fDebug) LogPrintf("dsee - Accepted Basenode entry %i %i\n", count, current);

            if(GetInputAge(vin) < BASENODE_MIN_CONFIRMATIONS){
                LogPrintf("dsee - Input must have least %d confirmations\n", BASENODE_MIN_CONFIRMATIONS);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }

            // verify that sig time is legit in past
            // should be at least not earlier than block when 250K BCR tx got BASENODE_MIN_CONFIRMATIONS
            uint256 hashBlock = 0;
            CTransaction txVin;
            GetTransaction(vin.prevout.hash, txVin, hashBlock, true);
            BlockMap::const_iterator t = mapBlockIndex.find(hashBlock);
            if (t != mapBlockIndex.end() && (*t).second)
            {
                CBlockIndex* pMNIndex = (*t).second; // block for 250K BCR tx -> 1 confirmation
                CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + BASENODE_MIN_CONFIRMATIONS - 1]; // block where tx got BASENODE_MIN_CONFIRMATIONS
                if(pConfIndex->GetBlockTime() > sigTime)
                {
                    LogPrintf("dsee - Bad sigTime %d for Basenode %20s %105s (%i conf block is at %d)\n",
                              sigTime, addr.ToString(), vin.ToString(), BASENODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                    return;
                }
            }


            // use this as a peer
            addrman.Add(CAddress(addr), pfrom->addr, 2*60*60);

            //doesn't support multisig addresses

            // add our Basenode
            CBasenode mn(addr, vin, pubkey, vchSig, sigTime, pubkey2, protocolVersion);
            mn.UpdateLastSeen(lastUpdated);
            this->Add(mn);

            // if it matches our Basenode privkey, then we've been remotely activated
            if(pubkey2 == activeBasenode.pubKeyBasenode && protocolVersion == PROTOCOL_VERSION){
                activeBasenode.EnableHotColdBaseNode(vin, addr);
            }

            if(count == -1 && !isLocal)
                mnodeman.RelayBasenodeEntry(vin, addr, vchSig, sigTime, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);

        } else {
            LogPrintf("dsee - Rejected Basenode entry %s\n", addr.ToString().c_str());

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

        // see if we have this Basenode
        CBasenode* pmn = this->Find(vin);
        if(pmn != NULL && pmn->protocolVersion >= nBasenodeMinProtocol)
        {
            // LogPrintf("dseep - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
            // take this only if it's newer
            if(pmn->lastDseep < sigTime)
            {
                std::string strMessage = pmn->addr.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(stop);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("dseep - Got bad Basenode address signature %s \n", vin.ToString().c_str());
                    //Misbehaving(pfrom->GetId(), 100);
                    return;
                }

                pmn->lastDseep = sigTime;

                if(!pmn->UpdatedWithin(BASENODE_MIN_DSEEP_SECONDS))
                {
                    if(stop) pmn->Disable();
                    else
                    {
                        pmn->UpdateLastSeen();
                        pmn->Check();
                        if(!pmn->IsEnabled()) return;
                    }
                    mnodeman.RelayBasenodeEntryPing(vin, vchSig, sigTime, stop);
                }
            }
            return;
        }

        if(fDebug) LogPrintf("dseep - Couldn't find Basenode entry %s\n", vin.ToString().c_str());

        std::map<COutPoint, int64_t>::iterator i = mWeAskedForBasenodeListEntry.find(vin.prevout);
        if (i != mWeAskedForBasenodeListEntry.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t) return; // we've asked recently
        }

        // ask for the dsee info once from the node that sent dseep

        LogPrintf("dseep - Asking source node for missing entry %s\n", vin.ToString().c_str());
        pfrom->PushMessage("dseg", vin);
        int64_t askAgain = GetTime() + BASENODE_MIN_DSEEP_SECONDS;
        mWeAskedForBasenodeListEntry[vin.prevout] = askAgain;

    } else if (strCommand == "mvote") { //Basenode Vote

        CTxIn vin;
        vector<unsigned char> vchSig;
        int nVote;
        vRecv >> vin >> vchSig >> nVote;

        // see if we have this Basenode
        CBasenode* pmn = this->Find(vin);
        if(pmn != NULL)
        {
            if((GetAdjustedTime() - pmn->lastVote) > (60*60))
            {
                std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(nVote);

                std::string errorMessage = "";
                if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
                {
                    LogPrintf("mvote - Got bad Basenode address signature %s \n", vin.ToString().c_str());
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

    } else if (strCommand == "dseg") { //Get Basenode list or specific entry

        CTxIn vin;
        vRecv >> vin;

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            if(!pfrom->addr.IsRFC1918())
            {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForBasenodeList.find(pfrom->addr);
                if (i != mAskedUsForBasenodeList.end())
                {
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("dseg - peer already asked me for the list\n");
                        return;
                    }
                }
                int64_t askAgain = GetTime() + BASENODES_DSEG_SECONDS;
                mAskedUsForBasenodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok

        int count = this->size();
        int i = 0;

        BOOST_FOREACH(CBasenode& mn, vBasenodes) {

            if(mn.addr.IsRFC1918()) continue; //local network

            if(mn.IsEnabled())
            {
                if(fDebug) LogPrintf("dseg - Sending Basenode entry - %s \n", mn.addr.ToString().c_str());
                if(vin == CTxIn()){
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                } else if (vin == mn.vin) {
                    pfrom->PushMessage("dsee", mn.vin, mn.addr, mn.sig, mn.sigTime, mn.pubkey, mn.pubkey2, count, i, mn.lastTimeSeen, mn.protocolVersion);
                    LogPrintf("dseg - Sent 1 Basenode entries to %s\n", pfrom->addr.ToString().c_str());
                    return;
                }
                i++;
            }
        }

        LogPrintf("dseg - Sent %d Basenode entries to %s\n", i, pfrom->addr.ToString().c_str());
    }

}

void CBasenodeMan::RelayBasenodeEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dsee", vin, addr, vchSig, nNow, pubkey, pubkey2, count, current, lastUpdated, protocolVersion);
}

void CBasenodeMan::RelayBasenodeEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        pnode->PushMessage("dseep", vin, vchSig, nNow, stop);
}

void CBasenodeMan::Remove(CTxIn vin)
{
    LOCK(cs);

    vector<CBasenode>::iterator it = vBasenodes.begin();
    while(it != vBasenodes.end()){
        if((*it).vin == vin){
            if(fDebug) LogPrintf("CBasenodeMan: Removing Basenode %s - %i now\n", (*it).addr.ToString().c_str(), size() - 1);
            vBasenodes.erase(it);
            break;
        }
    }
}

std::string CBasenodeMan::ToString() const
{
    std::ostringstream info;

    info << "Basenodes: " << (int)vBasenodes.size() <<
            ", peers who asked us for Basenode list: " << (int)mAskedUsForBasenodeList.size() <<
            ", peers we asked for Basenode list: " << (int)mWeAskedForBasenodeList.size() <<
            ", entries in Basenode list we asked for: " << (int)mWeAskedForBasenodeListEntry.size() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}
