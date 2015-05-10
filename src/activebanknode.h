// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcredit developers
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

// Responsible for activating the banknode and pinging the network
class CActiveBanknode
{
public:
	// Initialized by init.cpp
	// Keys for the main banknode
	CPubKey pubKeyBanknode;

	// Initialized while registering banknode
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveBanknode()
    {        
        status = BANKNODE_NOT_PROCESSED;
    }

    void ManageStatus(); // manage status of main banknode

    bool Dseep(std::string& errorMessage); // ping for main banknode
    bool Dseep(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage, bool stop); // ping for any banknode

    bool StopBankNode(std::string& errorMessage); // stop main banknode
    bool StopBankNode(std::string strService, std::string strKeyBanknode, std::string& errorMessage); // stop remote banknode
    bool StopBankNode(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string& errorMessage); // stop any banknode

    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string& errorMessage); // register remote banknode
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyBanknode, CPubKey pubKeyBanknode, std::string &retErrorMessage); // register any banknode
    bool RegisterByPubKey(std::string strService, std::string strKeyBanknode, std::string collateralAddress, std::string& errorMessage); // register for a specific collateral address

    // get 250k BCR input that can be used for the banknode
    bool GetBankNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetBankNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    bool GetBankNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetBankNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    vector<COutput> SelectCoinsBanknode();
    vector<COutput> SelectCoinsBanknodeForPubKey(std::string collateralAddress);
    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    //bool SelectCoinsBanknode(CTxIn& vin, int64& nValueIn, CScript& pubScript, std::string strTxHash, std::string strOutputIndex);

    // enable hot wallet mode (run a banknode with no funds)
    bool EnableHotColdBankNode(CTxIn& vin, CService& addr);
};

#endif
