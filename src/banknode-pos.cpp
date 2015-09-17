


//#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "protocol.h"
#include "activebanknode.h"
#include "banknodeman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include "banknodeman.h"

using namespace std;
using namespace boost;

std::map<uint256, CBanknodeScanningError> mapBanknodeScanningErrors;
CBanknodeScanning mnscan;

/* 
    Banknode - Proof of Service 

    -- What it checks

    1.) Making sure Banknodes have their ports open
    2.) Are responding to requests made by the network

    -- How it works

    When a block comes in, DoBanknodePOS is executed if the client is a 
    banknode. Using the deterministic ranking algorithm up to 1% of the banknode 
    network is checked each block. 

    A port is opened from Banknode A to Banknode B, if successful then nothing happens. 
    If there is an error, a CBanknodeScanningError object is propagated with an error code.
    Errors are applied to the Banknodes and a score is incremented within the banknode object,
    after a threshold is met, the banknode goes into an error state. Each cycle the score is 
    decreased, so if the banknode comes back online it will return to the list. 

    Banknodes in a error state do not receive payment. 

    -- Future expansion

    We want to be able to prove the nodes have many qualities such as a specific CPU speed, bandwidth,
    and dedicated storage. E.g. We could require a full node be a computer running 2GHz with 10GB of space.

*/

void ProcessMessageBanknodePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/banknode related functionality
    if(chaninActive.Tip()->nHeight<210000) return;
    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnse") //Banknode Scanning Error
    {

        CDataStream vMsg(vRecv);
        CBanknodeScanningError mnse;
        vRecv >> mnse;

        CInv inv(MSG_BANKNODE_SCANNING_ERROR, mnse.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapBanknodeScanningErrors.count(mnse.GetHash())){
            return;
        }
        mapBanknodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));

        if(!mnse.IsValid())
        {
            LogPrintf("BanknodePOS::mnse - Invalid object\n");   
            return;
        }

        CBanknode* pmnA = mnodeman.Find(mnse.vinBanknodeA);
        if(pmnA == NULL) return;
        if(pmnA->protocolVersion < MIN_BANKNODE_POS_PROTO_VERSION) return;

        int nBlockHeight = chainActive.Tip()->nHeight;
        if(nBlockHeight - mnse.nBlockHeight > 10){
            LogPrintf("BanknodePOS::mnse - Too old\n");
            return;   
        }

        // Lowest banknodes in rank check the highest each block
        int a = mnodeman.GetBanknodeRank(mnse.vinBanknodeA, mnse.nBlockHeight, MIN_BANKNODE_POS_PROTO_VERSION);
        if(a == -1 || a > GetCountScanningPerBlock())
        {
            if(a != -1) LogPrintf("BanknodePOS::mnse - BanknodeA ranking is too high\n");
            return;
        }

        int b = mnodeman.GetBanknodeRank(mnse.vinBanknodeB, mnse.nBlockHeight, MIN_BANKNODE_POS_PROTO_VERSION, false);
        if(b == -1 || b < mnodeman.CountBanknodesAboveProtocol(MIN_BANKNODE_POS_PROTO_VERSION)-GetCountScanningPerBlock())
        {
            if(b != -1) LogPrintf("BanknodePOS::mnse - BanknodeB ranking is too low\n");
            return;
        }

        if(!mnse.SignatureValid()){
            LogPrintf("BanknodePOS::mnse - Bad banknode message\n");
            return;
        }

        CBanknode* pmnB = mnodeman.Find(mnse.vinBanknodeB);
        if(pmnB == NULL) return;

        if(fDebug) LogPrintf("ProcessMessageBanknodePOS::mnse - nHeight %d BanknodeA %s BanknodeB %s\n", mnse.nBlockHeight, pmnA->addr.ToString().c_str(), pmnB->addr.ToString().c_str());

        pmnB->ApplyScanningError(mnse);
        mnse.Relay();
    }
}

// Returns how many banknodes are allowed to scan each block
int GetCountScanningPerBlock()
{
    return std::max(1, mnodeman.CountBanknodesAboveProtocol(MIN_BANKNODE_POS_PROTO_VERSION)/100);
}


void CBanknodeScanning::CleanBanknodeScanningErrors()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CBanknodeScanningError>::iterator it = mapBanknodeScanningErrors.begin();

    while(it != mapBanknodeScanningErrors.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old banknode scanning error %s\n", it->second.GetHash().ToString().c_str());

            mapBanknodeScanningErrors.erase(it++);
        } else {
            it++;
        }
    }

}

// Check other banknodes to make sure they're running correctly
void CBanknodeScanning::DoBanknodePOSChecks()
{
    if(!fBankNode) return;
    if(fLiteMode) return; //disable all darksend/banknode related functionality
    if(chaninActive.Tip()->nHeight<210000) return;
    if(IsInitialBlockDownload()) return;

    int nBlockHeight = chainActive.Tip()->nHeight-5;

    int a = mnodeman.GetBanknodeRank(activeBanknode.vin, nBlockHeight, MIN_BANKNODE_POS_PROTO_VERSION);
    if(a == -1 || a > GetCountScanningPerBlock()){
        // we don't need to do anything this block
        return;
    }

    // The lowest ranking nodes (Banknode A) check the highest ranking nodes (Banknode B)
    CBanknode* pmn = mnodeman.GetBanknodeByRank(mnodeman.CountBanknodesAboveProtocol(MIN_BANKNODE_POS_PROTO_VERSION)-a, nBlockHeight, MIN_BANKNODE_POS_PROTO_VERSION, false);
    if(pmn == NULL) return;

    // -- first check : Port is open

    if(!ConnectNode((CAddress)pmn->addr, NULL, true)){
        // we couldn't connect to the node, let's send a scanning error
        CBanknodeScanningError mnse(activeBanknode.vin, pmn->vin, SCANNING_ERROR_NO_RESPONSE, nBlockHeight);
        mnse.Sign();
        mapBanknodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
        mnse.Relay();
    }

    // success
    CBanknodeScanningError mnse(activeBanknode.vin, pmn->vin, SCANNING_SUCCESS, nBlockHeight);
    mnse.Sign();
    mapBanknodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
    mnse.Relay();
}

bool CBanknodeScanningError::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vinBanknodeA.ToString() + vinBanknodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    CBanknode* pmn = mnodeman.Find(vinBanknodeA);

    if(pmn == NULL)
    {
        LogPrintf("CBanknodeScanningError::SignatureValid() - Unknown Banknode\n");
        return false;
    }

    CScript pubkey;
    pubkey=GetScriptForDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcreditAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchBankNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CBanknodeScanningError::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CBanknodeScanningError::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = vinBanknodeA.ToString() + vinBanknodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    if(!darkSendSigner.SetKey(strBankNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CBanknodeScanningError::Sign() - ERROR: Invalid banknodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey=GetScriptForDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcreditAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBankNodeSignature, key2)) {
        LogPrintf("CBanknodeScanningError::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchBankNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CBanknodeScanningError::Sign() - Verify message failed");
        return false;
    }

    return true;
}

void CBanknodeScanningError::Relay()
{
    CInv inv(MSG_BANKNODE_SCANNING_ERROR, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}
