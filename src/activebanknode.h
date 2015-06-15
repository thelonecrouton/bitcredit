// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The Bitcredit developers
// Copyright (c) 2014-2015 The Darkcoin developers
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVEBANKNODE_H
#define ACTIVEBANKNODE_H

#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "primitives/transaction.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"

// Responsible for activating the Banknode and pinging the network
class CActiveBanknode
{
public:
	// Initialized by init.cpp
	// Keys for the main Banknode
	CPubKey pubKeyBanknode;

	// Initialized while registering Banknode
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveBanknode()
    {        
        status = BANKNODE_NOT_PROCESSED;
    }

    /// Manage status of main Banknode
    void ManageStatus(); 

    /// Ping for main Banknode
    bool Dseep(std::string& errorMessage); 
    /// Ping for any Banknode
    bool Dseep(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage, bool stop); 

    /// Stop main Banknode
    bool StopBankNode(std::string& errorMessage); 
    /// Stop remote Banknode
    bool StopBankNode(std::string strService, std::string strKeyBanknode, std::string& errorMessage); 
    /// Stop any Banknode
    bool StopBankNode(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string& errorMessage); 

    /// Register remote Banknode
    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string& errorMessage); 
    /// Register any Banknode
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyBanknode, CPubKey pubKeyBanknode, std::string &retErrorMessage); 

    bool RegisterByPubKey(std::string strService, std::string strKeyBanknode, std::string collateralAddress, std::string& errorMessage); // register for a specific collateral address

    // get 250k BCR input that can be used for the banknode
    bool GetBankNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetBankNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);

    bool GetBankNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    bool GetBankNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);

    vector<COutput> SelectCoinsBanknode();
 
    vector<COutput> SelectCoinsBanknodeForPubKey(std::string collateralAddress);

    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    /// Enable hot wallet mode (run a Banknode with no funds)
    bool EnableHotColdBankNode(CTxIn& vin, CService& addr);
};

#endif
