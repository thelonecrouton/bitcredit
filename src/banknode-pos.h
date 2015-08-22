

// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BANKNODE_POS_H
#define BANKNODE_POS_H

//#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "main.h"

using namespace std;
using namespace boost;

class CBanknodeScanning;
class CBanknodeScanningError;

extern map<uint256, CBanknodeScanningError> mapBanknodeScanningErrors;
extern CBanknodeScanning mnscan;

static const int MIN_BANKNODE_POS_PROTO_VERSION = 70076;

/*
	1% of the network is scanned every 2.5 minutes, making a full
	round of scanning take about 4.16 hours. We're targeting about
	a day of proof-of-service errors for complete removal from the
	banknode system.
*/
static const int BANKNODE_SCANNING_ERROR_THESHOLD = 6;

#define SCANNING_SUCCESS                       1
#define SCANNING_ERROR_NO_RESPONSE             2
#define SCANNING_ERROR_IX_NO_RESPONSE          3
#define SCANNING_ERROR_MAX                     3

void ProcessMessageBanknodePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

class CBanknodeScanning
{
public:
    void DoBanknodePOSChecks();
    void CleanBanknodeScanningErrors();
};

// Returns how many banknodes are allowed to scan each block
int GetCountScanningPerBlock();

class CBanknodeScanningError
{
public:
    CTxIn vinBanknodeA;
    CTxIn vinBanknodeB;
    int nErrorType;
    int nExpiration;
    int nBlockHeight;
    std::vector<unsigned char> vchBankNodeSignature;

    CBanknodeScanningError ()
    {
        vinBanknodeA = CTxIn();
        vinBanknodeB = CTxIn();
        nErrorType = 0;
        nExpiration = GetTime()+(60*60);
        nBlockHeight = 0;
    }

    CBanknodeScanningError (CTxIn& vinBanknodeAIn, CTxIn& vinBanknodeBIn, int nErrorTypeIn, int nBlockHeightIn)
    {
    	vinBanknodeA = vinBanknodeAIn;
    	vinBanknodeB = vinBanknodeBIn;
    	nErrorType = nErrorTypeIn;
    	nExpiration = GetTime()+(60*60);
    	nBlockHeight = nBlockHeightIn;
    }

    CBanknodeScanningError (CTxIn& vinBanknodeBIn, int nErrorTypeIn, int nBlockHeightIn)
    {
        //just used for IX, BanknodeA is any client
        vinBanknodeA = CTxIn();
        vinBanknodeB = vinBanknodeBIn;
        nErrorType = nErrorTypeIn;
        nExpiration = GetTime()+(60*60);
        nBlockHeight = nBlockHeightIn;
    }

    uint256 GetHash() const {return SerializeHash(*this);}

    bool SignatureValid();
    bool Sign();
    bool IsExpired() {return GetTime() > nExpiration;}
    void Relay();
    bool IsValid() {
    	return (nErrorType > 0 && nErrorType <= SCANNING_ERROR_MAX);
    }

	ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {

        READWRITE(vinBanknodeA);
        READWRITE(vinBanknodeB);
        READWRITE(nErrorType);
        READWRITE(nExpiration);
        READWRITE(nBlockHeight);
        READWRITE(vchBankNodeSignature);
	}
};


#endif
