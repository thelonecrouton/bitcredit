


//#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "protocol.h"
#include "activebasenode.h"
#include "basenodeman.h"
#include "spork.h"
#include <boost/lexical_cast.hpp>
#include "basenodeman.h"

using namespace std;
using namespace boost;

std::map<uint256, CBasenodeScanningError> mapBasenodeScanningErrors;
CBasenodeScanning mnscan;

/* 
    Basenode - Proof of Service 

    -- What it checks

    1.) Making sure Basenodes have their ports open
    2.) Are responding to requests made by the network

    -- How it works

    When a block comes in, DoBasenodePOS is executed if the client is a 
    basenode. Using the deterministic ranking algorithm up to 1% of the basenode 
    network is checked each block. 

    A port is opened from Basenode A to Basenode B, if successful then nothing happens. 
    If there is an error, a CBasenodeScanningError object is propagated with an error code.
    Errors are applied to the Basenodes and a score is incremented within the basenode object,
    after a threshold is met, the basenode goes into an error state. Each cycle the score is 
    decreased, so if the basenode comes back online it will return to the list. 

    Basenodes in a error state do not receive payment. 

    -- Future expansion

    We want to be able to prove the nodes have many qualities such as a specific CPU speed, bandwidth,
    and dedicated storage. E.g. We could require a full node be a computer running 2GHz with 10GB of space.

*/

void ProcessMessageBasenodePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(fLiteMode) return; //disable all darksend/basenode related functionality

    if(IsInitialBlockDownload()) return;

    if (strCommand == "mnse") //Basenode Scanning Error
    {

        CDataStream vMsg(vRecv);
        CBasenodeScanningError mnse;
        vRecv >> mnse;

        CInv inv(MSG_BASENODE_SCANNING_ERROR, mnse.GetHash());
        pfrom->AddInventoryKnown(inv);

        if(mapBasenodeScanningErrors.count(mnse.GetHash())){
            return;
        }
        mapBasenodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));

        if(!mnse.IsValid())
        {
            LogPrintf("BasenodePOS::mnse - Invalid object\n");   
            return;
        }

        CBasenode* pmnA = mnodeman.Find(mnse.vinBasenodeA);
        if(pmnA == NULL) return;
        if(pmnA->protocolVersion < MIN_BASENODE_POS_PROTO_VERSION) return;

        int nBlockHeight = chainActive.Tip()->nHeight;
        if(nBlockHeight - mnse.nBlockHeight > 10){
            LogPrintf("BasenodePOS::mnse - Too old\n");
            return;   
        }

        // Lowest basenodes in rank check the highest each block
        int a = mnodeman.GetBasenodeRank(mnse.vinBasenodeA, mnse.nBlockHeight, MIN_BASENODE_POS_PROTO_VERSION);
        if(a == -1 || a > GetCountScanningPerBlock())
        {
            if(a != -1) LogPrintf("BasenodePOS::mnse - BasenodeA ranking is too high\n");
            return;
        }

        int b = mnodeman.GetBasenodeRank(mnse.vinBasenodeB, mnse.nBlockHeight, MIN_BASENODE_POS_PROTO_VERSION, false);
        if(b == -1 || b < mnodeman.CountBasenodesAboveProtocol(MIN_BASENODE_POS_PROTO_VERSION)-GetCountScanningPerBlock())
        {
            if(b != -1) LogPrintf("BasenodePOS::mnse - BasenodeB ranking is too low\n");
            return;
        }

        if(!mnse.SignatureValid()){
            LogPrintf("BasenodePOS::mnse - Bad basenode message\n");
            return;
        }

        CBasenode* pmnB = mnodeman.Find(mnse.vinBasenodeB);
        if(pmnB == NULL) return;

        if(fDebug) LogPrintf("ProcessMessageBasenodePOS::mnse - nHeight %d BasenodeA %s BasenodeB %s\n", mnse.nBlockHeight, pmnA->addr.ToString().c_str(), pmnB->addr.ToString().c_str());

        pmnB->ApplyScanningError(mnse);
        mnse.Relay();
    }
}

// Returns how many basenodes are allowed to scan each block
int GetCountScanningPerBlock()
{
    return std::max(1, mnodeman.CountBasenodesAboveProtocol(MIN_BASENODE_POS_PROTO_VERSION)/100);
}


void CBasenodeScanning::CleanBasenodeScanningErrors()
{
    if(chainActive.Tip() == NULL) return;

    std::map<uint256, CBasenodeScanningError>::iterator it = mapBasenodeScanningErrors.begin();

    while(it != mapBasenodeScanningErrors.end()) {
        if(GetTime() > it->second.nExpiration){ //keep them for an hour
            LogPrintf("Removing old basenode scanning error %s\n", it->second.GetHash().ToString().c_str());

            mapBasenodeScanningErrors.erase(it++);
        } else {
            it++;
        }
    }

}

// Check other basenodes to make sure they're running correctly
void CBasenodeScanning::DoBasenodePOSChecks()
{
    if(!fBaseNode) return;
    if(fLiteMode) return; //disable all darksend/basenode related functionality

    if(IsInitialBlockDownload()) return;

    int nBlockHeight = chainActive.Tip()->nHeight-5;

    int a = mnodeman.GetBasenodeRank(activeBasenode.vin, nBlockHeight, MIN_BASENODE_POS_PROTO_VERSION);
    if(a == -1 || a > GetCountScanningPerBlock()){
        // we don't need to do anything this block
        return;
    }

    // The lowest ranking nodes (Basenode A) check the highest ranking nodes (Basenode B)
    CBasenode* pmn = mnodeman.GetBasenodeByRank(mnodeman.CountBasenodesAboveProtocol(MIN_BASENODE_POS_PROTO_VERSION)-a, nBlockHeight, MIN_BASENODE_POS_PROTO_VERSION, false);
    if(pmn == NULL) return;

    // -- first check : Port is open

    if(!ConnectNode((CAddress)pmn->addr, NULL, true)){
        // we couldn't connect to the node, let's send a scanning error
        CBasenodeScanningError mnse(activeBasenode.vin, pmn->vin, SCANNING_ERROR_NO_RESPONSE, nBlockHeight);
        mnse.Sign();
        mapBasenodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
        mnse.Relay();
    }

    // success
    CBasenodeScanningError mnse(activeBasenode.vin, pmn->vin, SCANNING_SUCCESS, nBlockHeight);
    mnse.Sign();
    mapBasenodeScanningErrors.insert(make_pair(mnse.GetHash(), mnse));
    mnse.Relay();
}

bool CBasenodeScanningError::SignatureValid()
{
    std::string errorMessage;
    std::string strMessage = vinBasenodeA.ToString() + vinBasenodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    CBasenode* pmn = mnodeman.Find(vinBasenodeA);

    if(pmn == NULL)
    {
        LogPrintf("CBasenodeScanningError::SignatureValid() - Unknown Basenode\n");
        return false;
    }

    CScript pubkey;
    pubkey=GetScriptForDestination(pmn->pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcreditAddress address2(address1);

    if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchBaseNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CBasenodeScanningError::SignatureValid() - Verify message failed\n");
        return false;
    }

    return true;
}

bool CBasenodeScanningError::Sign()
{
    std::string errorMessage;

    CKey key2;
    CPubKey pubkey2;
    std::string strMessage = vinBasenodeA.ToString() + vinBasenodeB.ToString() + 
        boost::lexical_cast<std::string>(nBlockHeight) + boost::lexical_cast<std::string>(nErrorType);

    if(!darkSendSigner.SetKey(strBaseNodePrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CBasenodeScanningError::Sign() - ERROR: Invalid basenodeprivkey: '%s'\n", errorMessage.c_str());
        return false;
    }

    CScript pubkey;
    pubkey=GetScriptForDestination(pubkey2.GetID());
    CTxDestination address1;
    ExtractDestination(pubkey, address1);
    CBitcreditAddress address2(address1);
    //LogPrintf("signing pubkey2 %s \n", address2.ToString().c_str());

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBaseNodeSignature, key2)) {
        LogPrintf("CBasenodeScanningError::Sign() - Sign message failed");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey2, vchBaseNodeSignature, strMessage, errorMessage)) {
        LogPrintf("CBasenodeScanningError::Sign() - Verify message failed");
        return false;
    }

    return true;
}

void CBasenodeScanningError::Relay()
{
    CInv inv(MSG_BASENODE_SCANNING_ERROR, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}
