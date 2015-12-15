// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "basenode.h"
#include "basenodeman.h"
#include "darksend.h"
#include "primitives/transaction.h"
#include "util.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

CCriticalSection cs_basenodepayments;

/** Object for who's going to get paid on which blocks */
CBasenodePayments basenodePayments;
// keep track of Basenode votes I've seen
map<uint256, CBasenodePaymentWinner> mapSeenBasenodeVotes;
// keep track of the scanning errors I've seen
map<uint256, int> mapSeenBasenodeScanningErrors;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

void ProcessMessageBasenodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnget") { //Basenode Payments Request Sync
        if(fLiteMode) return; //disable all Darksend/Basenode related functionality

        if(pfrom->HasFulfilledRequest("mnget")) {
            LogPrintf("mnget - peer already asked me for the list\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        pfrom->FulfilledRequest("mnget");
        basenodePayments.Sync(pfrom);
        LogPrintf("mnget - Sent Basenode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "mnw") { //Basenode Payments Declare Winner

        LOCK(cs_basenodepayments);

        //this is required in litemode
        CBasenodePaymentWinner winner;
        vRecv >> winner;

        if(chainActive.Tip() == NULL) return;

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcreditAddress address2(address1);

        uint256 hash = winner.GetHash();
        if(mapSeenBasenodeVotes.count(hash)) {
            if(fDebug) LogPrintf("mnw - seen vote %s Addr %s Height %d bestHeight %d\n", hash.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.nBlockHeight < chainActive.Tip()->nHeight - 10 || winner.nBlockHeight > chainActive.Tip()->nHeight+20){
            LogPrintf("mnw - winner out of range %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);
            return;
        }

        if(winner.vin.nSequence != std::numeric_limits<unsigned int>::max()){
            LogPrintf("mnw - invalid nSequence\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrintf("mnw - winning vote - Vin %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, chainActive.Tip()->nHeight);

        if(!basenodePayments.CheckSignature(winner)){
            LogPrintf("mnw - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSeenBasenodeVotes.insert(make_pair(hash, winner));

        if(basenodePayments.AddWinningBasenode(winner)){
            basenodePayments.Relay(winner);
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

CBasenode::CBasenode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = BASENODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastDseep = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = MIN_PEER_PROTO_VERSION;
    nLastDsq = 0;
    nVote = 0;
    lastVote = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CBasenode::CBasenode(const CBasenode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastDseep = other.lastDseep;
    lastTimeSeen = other.lastTimeSeen;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    nVote = other.nVote;
    lastVote = other.lastVote;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
}

CBasenode::CBasenode(CService newAddr, CTxIn newVin, CPubKey newPubkey, std::vector<unsigned char> newSig, int64_t newSigTime, CPubKey newPubkey2, int protocolVersionIn)
{
    LOCK(cs);
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = newSig;
    activeState = BASENODE_ENABLED;
    sigTime = newSigTime;
    lastDseep = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    nVote = 0;
    lastVote = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

//
// Deterministically calculate a given "score" for a Basenode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CBasenode::CalculateScore(int mod, int64_t nBlockHeight)
{
    if(chainActive.Tip() == NULL) return 0;

    uint256 hash = 0;
    uint256 aux = vin.prevout.hash + vin.prevout.n;

    if(!GetBlockHash(hash, nBlockHeight)) return 0;

    uint256 hash2 = Hash(BEGIN(hash), END(hash));
    uint256 hash3 = Hash(BEGIN(hash), END(hash), BEGIN(aux), END(aux));

    uint256 r = (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);

    return r;
}

void CBasenode::Check()
{
    //TODO: Random segfault with this line removed
    TRY_LOCK(cs_main, lockRecv);
    if(!lockRecv) return;

    if(nScanningErrorCount >= BASENODE_SCANNING_ERROR_THESHOLD)
    {
        activeState = BASENODE_POS_ERROR;
        return;
    }

    //once spent, stop doing the checks
    if(activeState == BASENODE_VIN_SPENT) return;


    if(!UpdatedWithin(BASENODE_REMOVAL_SECONDS)){
        activeState = BASENODE_REMOVE;
        return;
    }

    if(!UpdatedWithin(BASENODE_EXPIRATION_SECONDS)){
        activeState = BASENODE_EXPIRED;
        return;
    }

    if(!unitTest){
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


        if(!AcceptableInputs(mempool, state, tx)){
            activeState = BASENODE_VIN_SPENT;
            return;
        }
    }

    activeState = BASENODE_ENABLED; // OK
}

bool CBasenodePayments::CheckSignature(CBasenodePaymentWinner& winner)
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

bool CBasenodePayments::Sign(CBasenodePaymentWinner& winner)
{
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!darkSendSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CBasenodePayments::Sign - ERROR: Invalid Basenodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, winner.vchSig, key2)) {
        LogPrintf("CBasenodePayments::Sign - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, winner.vchSig, strMessage, errorMessage)) {
        LogPrintf("CBasenodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

uint64_t CBasenodePayments::CalculateScore(uint256 blockHash, CTxIn& vin)
{
    uint256 n1 = blockHash;
    uint256 n2 = Hash(BEGIN(n1), END(n1));
    uint256 n3 = Hash(BEGIN(vin.prevout.hash), END(vin.prevout.hash));
    uint256 n4 = n3 > n2 ? (n3 - n2) : (n2 - n3);

    //printf(" -- CBasenodePayments CalculateScore() n2 = %d \n", n2.Get64());
    //printf(" -- CBasenodePayments CalculateScore() n3 = %d \n", n3.Get64());
    //printf(" -- CBasenodePayments CalculateScore() n4 = %d \n", n4.Get64());

    return n4.Get64();
}

bool CBasenodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    BOOST_FOREACH(CBasenodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            payee = winner.payee;
            return true;
        }
    }

    return false;
}

bool CBasenodePayments::GetWinningBasenode(int nBlockHeight, CTxIn& vinOut)
{
    BOOST_FOREACH(CBasenodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == nBlockHeight) {
            vinOut = winner.vin;
            return true;
        }
    }

    return false;
}

bool CBasenodePayments::AddWinningBasenode(CBasenodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight-576)) {
        return false;
    }

    winnerIn.score = CalculateScore(blockHash, winnerIn.vin);

    bool foundBlock = false;
    BOOST_FOREACH(CBasenodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == winnerIn.nBlockHeight) {
            foundBlock = true;
            if(winner.score < winnerIn.score){
                winner.score = winnerIn.score;
                winner.vin = winnerIn.vin;
                winner.payee = winnerIn.payee;
                winner.vchSig = winnerIn.vchSig;

                mapSeenBasenodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

                return true;
            }
        }
    }

    // if it's not in the vector
    if(!foundBlock){
        vWinning.push_back(winnerIn);
        mapSeenBasenodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

        return true;
    }

    return false;
}

void CBasenodePayments::CleanPaymentList()
{
    LOCK(cs_basenodepayments);

    if(chainActive.Tip() == NULL) return;

    int nLimit = std::max(((int)mnodeman.size())*2, 1000);

    vector<CBasenodePaymentWinner>::iterator it;
    for(it=vWinning.begin();it<vWinning.end();it++){
        if(chainActive.Tip()->nHeight - (*it).nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CBasenodePayments::CleanPaymentList - Removing old Basenode payment - block %d\n", (*it).nBlockHeight);
            vWinning.erase(it);
            break;
        }
    }
}

bool CBasenodePayments::ProcessBlock(int nBlockHeight)
{
    LOCK(cs_basenodepayments);

    if(nBlockHeight <= nLastBlockHeight) return false;
    if(!enabled) return false;
    CBasenodePaymentWinner newWinner;
    unsigned int nMinimumAge = mnodeman.CountEnabled();
    CScript payeeSource;

    uint256 hash;
    if(!GetBlockHash(hash, nBlockHeight-10)) return false;
    unsigned int nHash;
    memcpy(&nHash, &hash, 2);

    LogPrintf(" ProcessBlock Start nHeight %d. \n", nBlockHeight);

    std::vector<CTxIn> vecLastPayments;
    BOOST_REVERSE_FOREACH(CBasenodePaymentWinner& winner, vWinning)
    {
        //if we already have the same vin - we have one full payment cycle, break
        if(vecLastPayments.size() > nMinimumAge) break;
        vecLastPayments.push_back(winner.vin);
    }

    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    CBasenode *pmn = mnodeman.FindOldestNotInVec(vecLastPayments, nMinimumAge, 0);
    if(pmn != NULL)
    {
        LogPrintf(" Found by FindOldestNotInVec \n");

        newWinner.score = 0;
        newWinner.nBlockHeight = nBlockHeight;
        newWinner.vin = pmn->vin;

        {   newWinner.payee=GetScriptForDestination(pmn->pubkey.GetID());
        }

        payeeSource=GetScriptForDestination(pmn->pubkey.GetID());
    }

    //if we can't find new MN to get paid, pick first active MN counting back from the end of vecLastPayments list
    if(newWinner.nBlockHeight == 0 && nMinimumAge > 0)
    {
        LogPrintf(" Find by reverse \n");

        BOOST_REVERSE_FOREACH(CTxIn& vinLP, vecLastPayments)
        {
            CBasenode* pmn = mnodeman.Find(vinLP);
            if(pmn != NULL)
            {
                pmn->Check();
                if(!pmn->IsEnabled()) continue;

                newWinner.score = 0;
                newWinner.nBlockHeight = nBlockHeight;
                newWinner.vin = pmn->vin;

                {
                    newWinner.payee=GetScriptForDestination(pmn->pubkey.GetID());
                }

                payeeSource=GetScriptForDestination(pmn->pubkey.GetID());

                break; // we found active MN
            }
        }
    }

    if(newWinner.nBlockHeight == 0) return false;

    CTxDestination address1;
    ExtractDestination(newWinner.payee, address1);
    CBitcreditAddress address2(address1);

    CTxDestination address3;
    ExtractDestination(payeeSource, address3);
    CBitcreditAddress address4(address3);

    LogPrintf("Winner payee %s nHeight %d vin source %s. \n", address2.ToString().c_str(), newWinner.nBlockHeight, address4.ToString().c_str());

    if(Sign(newWinner))
    {
        if(AddWinningBasenode(newWinner))
        {
            Relay(newWinner);
            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}

void CBasenodePayments::Relay(CBasenodePaymentWinner& winner)
{
    CInv inv(MSG_BASENODE_WINNER, winner.GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

void CBasenodePayments::Sync(CNode* node)
{
    LOCK(cs_basenodepayments);

    BOOST_FOREACH(CBasenodePaymentWinner& winner, vWinning)
        if(winner.nBlockHeight >= chainActive.Tip()->nHeight-10 && winner.nBlockHeight <= chainActive.Tip()->nHeight + 20)
            node->PushMessage("mnw", winner);
}


bool CBasenodePayments::SetPrivKey(std::string strPrivKey)
{
    CBasenodePaymentWinner winner;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(winner);

    if(CheckSignature(winner)){
        LogPrintf("CBasenodePayments::SetPrivKey - Successfully initialized as Basenode payments master\n");
        enabled = true;
        return true;
    } else {
        return false;
    }
}
