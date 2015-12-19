// Copyright (c) 2014-2015 The Darkcoin developers
// Copyright (c) 2009-2015 The Bitcoin developers
// Copyright (c) 2014-2015 The Bitcredit developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "activebasenode.h"
#include "basenodeman.h"
#include <boost/lexical_cast.hpp>
#include "clientversion.h"

//
// Bootup the Basenode, look for a 50 000 BCR input and register on the network
//
void CActiveBasenode::ManageStatus()
{
    std::string errorMessage;

    if(!fBaseNode) return;

    if (fDebug) LogPrintf("CActiveBasenode::ManageStatus() - Begin\n");

    //need correct adjusted time to send ping
    bool fIsInitialDownload = IsInitialBlockDownload();
    if(fIsInitialDownload) {
        status = BASENODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveBasenode::ManageStatus() - Sync in progress. Must wait until sync is complete to start Basenode.\n");
        return;
    }

    if(status == BASENODE_INPUT_TOO_NEW || status == BASENODE_NOT_CAPABLE || status == BASENODE_SYNC_IN_PROCESS){
        status = BASENODE_NOT_PROCESSED;
    }

    if(status == BASENODE_NOT_PROCESSED) {
        if(strBaseNodeAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the Basenodeaddr configuration option.";
                status = BASENODE_NOT_CAPABLE;
                LogPrintf("CActiveBasenode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        } else {
            service = CService(strBaseNodeAddr);
        }

        LogPrintf("CActiveBasenode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString().c_str());
      
            if(!ConnectNode((CAddress)service, service.ToString().c_str())){
                notCapableReason = "Could not connect to " + service.ToString();
                status = BASENODE_NOT_CAPABLE;
                LogPrintf("CActiveBasenode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        

        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            status = BASENODE_NOT_CAPABLE;
            LogPrintf("CActiveBasenode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }

        // Set defaults
        status = BASENODE_NOT_CAPABLE;
        notCapableReason = "Unknown. Check debug.log for more information.\n";

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(GetBaseNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < BASENODE_MIN_CONFIRMATIONS){
                notCapableReason = "Input must have least " + boost::lexical_cast<string>(BASENODE_MIN_CONFIRMATIONS) +
                        " confirmations - " + boost::lexical_cast<string>(GetInputAge(vin)) + " confirmations";
                LogPrintf("CActiveBasenode::ManageStatus() - %s\n", notCapableReason.c_str());
                status = BASENODE_INPUT_TOO_NEW;
                return;
            }

            LogPrintf("CActiveBasenode::ManageStatus() - Is capable bank node!\n");

            status = BASENODE_IS_CAPABLE;
            notCapableReason = "";

            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyBasenode;
            CKey keyBasenode;

            if(!darkSendSigner.SetKey(strBaseNodePrivKey, errorMessage, keyBasenode, pubKeyBasenode))
            {
                LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
                return;
            }

            if(!Register(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyBasenode, pubKeyBasenode, errorMessage)) {
            	LogPrintf("CActiveBasenode::ManageStatus() - Error on Register: %s\n", errorMessage.c_str());
            }

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveBasenode::ManageStatus() - %s\n", notCapableReason.c_str());
        }
    }

    //send to all peers
    if(!Dseep(errorMessage)) {
        LogPrintf("CActiveBasenode::ManageStatus() - Error on Ping: %s\n", errorMessage.c_str());
    }
}

// Send stop dseep to network for remote Basenode
bool CActiveBasenode::StopBaseNode(std::string strService, std::string strKeyBasenode, std::string& errorMessage) {
    CTxIn vin;
    CKey keyBasenode;
    CPubKey pubKeyBasenode;

    if(!darkSendSigner.SetKey(strKeyBasenode, errorMessage, keyBasenode, pubKeyBasenode)) {
        LogPrintf("CActiveBasenode::StopBaseNode() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return StopBaseNode(vin, CService(strService), keyBasenode, pubKeyBasenode, errorMessage);
}

// Send stop dseep to network for main Basenode
bool CActiveBasenode::StopBaseNode(std::string& errorMessage) {
    if(status != BASENODE_IS_CAPABLE && status != BASENODE_REMOTELY_ENABLED) {
        errorMessage = "Basenode is not in a running status";
        LogPrintf("CActiveBasenode::StopBaseNode() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    status = BASENODE_STOPPED;

    CPubKey pubKeyBasenode;
    CKey keyBasenode;

    if(!darkSendSigner.SetKey(strBaseNodePrivKey, errorMessage, keyBasenode, pubKeyBasenode))
    {
        LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    return StopBaseNode(vin, service, keyBasenode, pubKeyBasenode, errorMessage);
}

// Send stop dseep to network for any Basenode
bool CActiveBasenode::StopBaseNode(CTxIn vin, CService service, CKey keyBasenode, CPubKey pubKeyBasenode, std::string& errorMessage) {
    pwalletMain->UnlockCoin(vin.prevout);
    return Dseep(vin, service, keyBasenode, pubKeyBasenode, errorMessage, true);
}

bool CActiveBasenode::Dseep(std::string& errorMessage) {
    if(status != BASENODE_IS_CAPABLE && status != BASENODE_REMOTELY_ENABLED) {
        errorMessage = "Basenode is not in a running status";
        LogPrintf("CActiveBasenode::Dseep() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    CPubKey pubKeyBasenode;
    CKey keyBasenode;

    if(!darkSendSigner.SetKey(strBaseNodePrivKey, errorMessage, keyBasenode, pubKeyBasenode))
    {
        LogPrintf("CActiveBasenode::Dseep() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    return Dseep(vin, service, keyBasenode, pubKeyBasenode, errorMessage, false);
}

bool CActiveBasenode::Dseep(CTxIn vin, CService service, CKey keyBasenode, CPubKey pubKeyBasenode, std::string &retErrorMessage, bool stop) {
    std::string errorMessage;
    std::vector<unsigned char> vchBaseNodeSignature;
    std::string strBaseNodeSignMessage;
    int64_t baseNodeSignatureTime = GetAdjustedTime();

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(baseNodeSignatureTime) + boost::lexical_cast<std::string>(stop);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBaseNodeSignature, keyBasenode)) {
        retErrorMessage = "sign message failed: " + errorMessage;
        LogPrintf("CActiveBasenode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyBasenode, vchBaseNodeSignature, strMessage, errorMessage)) {
        retErrorMessage = "Verify message failed: " + errorMessage;
        LogPrintf("CActiveBasenode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    // Update Last Seen timestamp in Basenode list
    CBasenode* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(stop)
            mnodeman.Remove(pmn->vin);
        else
            pmn->UpdateLastSeen();
    }
    else
    {
        // Seems like we are trying to send a ping while the Basenode is not registered in the network
        retErrorMessage = "Darksend Basenode List doesn't include our Basenode, Shutting down Basenode pinging service! " + vin.ToString();
        LogPrintf("CActiveBasenode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        status = BASENODE_NOT_CAPABLE;
        notCapableReason = retErrorMessage;
        return false;
    }

    //send to all peers
    LogPrintf("CActiveBasenode::Dseep() - RelayBasenodeEntryPing vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayBasenodeEntryPing(vin, vchBaseNodeSignature, baseNodeSignatureTime, stop);

    return true;
}

bool CActiveBasenode::Register(std::string strService, std::string strKeyBasenode, std::string txHash, std::string strOutputIndex, std::string& errorMessage) {
	CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyBasenode;
    CKey keyBasenode;

    if(!darkSendSigner.SetKey(strKeyBasenode, errorMessage, keyBasenode, pubKeyBasenode))
    {
        LogPrintf("CActiveBasenode::Register() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }

    if(!GetBaseNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, txHash, strOutputIndex)) {
		errorMessage = "could not allocate vin";
    	LogPrintf("CActiveBasenode::Register() - Error: %s\n", errorMessage.c_str());
		return false;
	}
	return Register(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyBasenode, pubKeyBasenode, errorMessage);
}

bool CActiveBasenode::RegisterByPubKey(std::string strService, std::string strKeyBasenode, std::string collateralAddress, std::string& errorMessage) {
	CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyBasenode;
    CKey keyBasenode;

    if(!darkSendSigner.SetKey(strKeyBasenode, errorMessage, keyBasenode, pubKeyBasenode))
    {
    	LogPrintf("CActiveBasenode::RegisterByPubKey() - Error upon calling SetKey: %s\n", errorMessage.c_str());
    	return false;
    }

    if(!GetBaseNodeVinForPubKey(collateralAddress, vin, pubKeyCollateralAddress, keyCollateralAddress)) {
		errorMessage = "could not allocate vin for collateralAddress";
    	LogPrintf("Register::Register() - Error: %s\n", errorMessage.c_str());
		return false;
	}
	return Register(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyBasenode, pubKeyBasenode, errorMessage);
}

bool CActiveBasenode::Register(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyBasenode, CPubKey pubKeyBasenode, std::string &retErrorMessage) {
    std::string errorMessage;
    std::vector<unsigned char> vchBaseNodeSignature;
    std::string strBaseNodeSignMessage;
    int64_t baseNodeSignatureTime = GetAdjustedTime();

    std::string vchPubKey(pubKeyCollateralAddress.begin(), pubKeyCollateralAddress.end());
    std::string vchPubKey2(pubKeyBasenode.begin(), pubKeyBasenode.end());

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(baseNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(PROTOCOL_VERSION);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchBaseNodeSignature, keyCollateralAddress)) {
        retErrorMessage = "sign message failed: " + errorMessage;
        LogPrintf("CActiveBasenode::Register() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyCollateralAddress, vchBaseNodeSignature, strMessage, errorMessage)) {
        retErrorMessage = "Verify message failed: " + errorMessage;
        LogPrintf("CActiveBasenode::Register() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    CBasenode* pmn = mnodeman.Find(vin);
    if(pmn == NULL)
    {
        LogPrintf("CActiveBasenode::Register() - Adding to Basenode list service: %s - vin: %s\n", service.ToString().c_str(), vin.ToString().c_str());
        CBasenode mn(service, vin, pubKeyCollateralAddress, vchBaseNodeSignature, baseNodeSignatureTime, pubKeyBasenode, PROTOCOL_VERSION);
        mn.UpdateLastSeen(baseNodeSignatureTime);
        mnodeman.Add(mn);
    }

    //send to all peers
    LogPrintf("CActiveBasenode::Register() - RelayElectionEntry vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayBasenodeEntry(vin, service, vchBaseNodeSignature, baseNodeSignatureTime, pubKeyCollateralAddress, pubKeyBasenode, -1, -1, baseNodeSignatureTime, PROTOCOL_VERSION);

    return true;
}

bool CActiveBasenode::GetBaseNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
	return GetBaseNodeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveBasenode::GetBaseNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsBasenode();
    COutput *selectedOutput;

    // Find the vin
    if(!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
        bool found = false;
        BOOST_FOREACH(COutput& out, possibleCoins) {
            if(out.tx->GetHash() == txHash && out.i == outputIndex)
            {
                selectedOutput = &out;
                found = true;
                break;
            }
        }
        if(!found) {
            LogPrintf("CActiveBasenode::GetBaseNodeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if(possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveBasenode::GetBaseNodeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Basenode vin information from output
bool CActiveBasenode::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {

    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CBitcreditAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveBasenode::GetBaseNodeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveBasenode::GetBaseNodeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

bool CActiveBasenode::GetBaseNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
	return GetBaseNodeVinForPubKey(collateralAddress, vin, pubkey, secretKey, "", "");
}

bool CActiveBasenode::GetBaseNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsBasenodeForPubKey(collateralAddress);
    COutput *selectedOutput;

    // Find the vin
	if(!strTxHash.empty()) {
		// Let's find it
		uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
		bool found = false;
		BOOST_FOREACH(COutput& out, possibleCoins) {
			if(out.tx->GetHash() == txHash && out.i == outputIndex)
			{
				selectedOutput = &out;
				found = true;
				break;
			}
		}
		if(!found) {
			LogPrintf("CActiveBasenode::GetBaseNodeVinForPubKey - Could not locate valid vin\n");
			return false;
		}
	} else {
		// No output specified,  Select the first one
		if(possibleCoins.size() > 0) {
			selectedOutput = &possibleCoins[0];
		} else {
			LogPrintf("CActiveBasenode::GetBaseNodeVinForPubKey - Could not locate specified vin from possible list\n");
			return false;
		}
    }

	// At this point we have a selected output, retrieve the associated info
	return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}




// get all possible outputs for running basenode
vector<COutput> CActiveBasenode::SelectCoinsBasenode()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Filter
    
    if (chainActive.Tip()->nHeight<145000) {
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == 250000*COIN) { //exactly
            filteredCoins.push_back(out);
        }
    }
	}
	else {
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == 50000*COIN) { //exactly
            filteredCoins.push_back(out);
        }
    }
	}
    
    return filteredCoins;
}

// get all possible outputs for running basenode for a specific pubkey
vector<COutput> CActiveBasenode::SelectCoinsBasenodeForPubKey(std::string collateralAddress)
{
    CBitcreditAddress address(collateralAddress);
    CScript scriptPubKey;
    scriptPubKey= GetScriptForDestination(address.Get());
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Filter
    if (chainActive.Tip()->nHeight<145000) {
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].scriptPubKey == scriptPubKey && out.tx->vout[out.i].nValue == 250000*COIN) { //exactly
        	filteredCoins.push_back(out);
        }
    }
	}
	else {
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].scriptPubKey == scriptPubKey && out.tx->vout[out.i].nValue == 50000*COIN) { //exactly
        	filteredCoins.push_back(out);
        }
    }
	} 
    return filteredCoins;
}


/* select coins with specified transaction hash and output index */


// when starting a basenode, this can enable to run as a hot wallet with no funds
bool CActiveBasenode::EnableHotColdBaseNode(CTxIn& newVin, CService& newService)
{
    if(!fBaseNode) return false;

    status = BASENODE_REMOTELY_ENABLED;

    //The values below are needed for signing dseep messages going forward
    this->vin = newVin;
    this->service = newService;

    LogPrintf("CActiveBasenode::EnableHotColdBaseNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
