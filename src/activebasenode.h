// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2016 Section-32 Financial Instruments
// Copyright (c) 2014-2015 The Darkcoin developers
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVEBASENODE_H
#define ACTIVEBASENODE_H

#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "primitives/transaction.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"

// Responsible for activating the Basenode and pinging the network
class CActiveBasenode
{
public:
	// Initialized by init.cpp
	// Keys for the main Basenode
	CPubKey pubKeyBasenode;

	// Initialized while registering Basenode
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveBasenode()
    {        
        status = BASENODE_NOT_PROCESSED;
    }

    /// Manage status of main Basenode
    void ManageStatus(); 

    /// Ping for main Basenode
    bool Dseep(std::string& errorMessage); 
    /// Ping for any Basenode
    bool Dseep(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage, bool stop); 

    /// Stop main Basenode
    bool StopBaseNode(std::string& errorMessage); 
    /// Stop remote Basenode
    bool StopBaseNode(std::string strService, std::string strKeyBasenode, std::string& errorMessage); 
    /// Stop any Basenode
    bool StopBaseNode(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string& errorMessage); 

    /// Register remote Basenode
    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string& errorMessage); 
    /// Register any Basenode
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyBasenode, CPubKey pubKeyBasenode, std::string &retErrorMessage); 

    bool RegisterByPubKey(std::string strService, std::string strKeyBasenode, std::string collateralAddress, std::string& errorMessage); // register for a specific collateral address

    // get 250k BCR input that can be used for the basenode
    bool GetBaseNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetBaseNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);

    bool GetBaseNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    bool GetBaseNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);

    vector<COutput> SelectCoinsBasenode();
 
    vector<COutput> SelectCoinsBasenodeForPubKey(std::string collateralAddress);

    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    /// Enable hot wallet mode (run a Basenode with no funds)
    bool EnableHotColdBaseNode(CTxIn& vin, CService& addr);
};

#endif
