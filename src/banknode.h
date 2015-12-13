// Copyright (c) 2009-2012 The Darkcoin developers
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The Bitcredit developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BASENODE_H
#define BASENODE_H

#include "uint256.h"
#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "primitives/transaction.h"
#include "primitives/block.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "main.h"
#include "timedata.h"
#include "basenode-pos.h"

#define BASENODE_NOT_PROCESSED               0 // initial state
#define BASENODE_IS_CAPABLE                  1
#define BASENODE_NOT_CAPABLE                 2
#define BASENODE_STOPPED                     3
#define BASENODE_INPUT_TOO_NEW               4
#define BASENODE_PORT_NOT_OPEN               6
#define BASENODE_PORT_OPEN                   7
#define BASENODE_SYNC_IN_PROCESS             8
#define BASENODE_REMOTELY_ENABLED            9

#define BASENODE_MIN_CONFIRMATIONS           15
#define BASENODE_MIN_DSEEP_SECONDS           (30*60)
#define BASENODE_MIN_DSEE_SECONDS            (5*60)
#define BASENODE_PING_SECONDS                (1*60)
#define BASENODE_EXPIRATION_SECONDS          (65*60)
#define BASENODE_REMOVAL_SECONDS             (70*60)

using namespace std;

class CBasenode;
class CBasenodePayments;
class CBasenodePaymentWinner;

extern CCriticalSection cs_basenodepayments;
extern map<uint256, CBasenodePaymentWinner> mapSeenBasenodeVotes;
extern map<int64_t, uint256> mapCacheBlockHashes;
extern CBasenodePayments basenodePayments;

void ProcessMessageBasenodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
bool GetBlockHash(uint256& hash, int nBlockHeight);

//
// The Basenode Class. For managing the Darksend process. It contains the input of the 1000DRK, signature to prove
// it's the one who own that ip address and code for calculating the payment election.
//
class CBasenode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

public:
    enum state {
        BASENODE_ENABLED = 1,
        BASENODE_EXPIRED = 2,
        BASENODE_VIN_SPENT = 3,
        BASENODE_REMOVE = 4,
        BASENODE_POS_ERROR = 5
    };

    CTxIn vin;
    CService addr;
    CPubKey pubkey;
    CPubKey pubkey2;
    std::vector<unsigned char> sig;
    int activeState;
    int64_t sigTime; //dsee message times
    int64_t lastDseep;
    int64_t lastTimeSeen;
    int cacheInputAge;
    int cacheInputAgeBlock;
    bool unitTest;
    bool allowFreeTx;
    int protocolVersion;
    int64_t nLastDsq; //the dsq count from the last dsq broadcast of this node
    int nVote;
    int64_t lastVote;
    int nScanningErrorCount;
    int nLastScanningErrorBlockHeight;

    CBasenode();
    CBasenode(const CBasenode& other);
    CBasenode(CService newAddr, CTxIn newVin, CPubKey newPubkey, std::vector<unsigned char> newSig, int64_t newSigTime, CPubKey newPubkey2, int protocolVersionIn);

    void swap(CBasenode& first, CBasenode& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.vin, second.vin);
        swap(first.addr, second.addr);
        swap(first.pubkey, second.pubkey);
        swap(first.pubkey2, second.pubkey2);
        swap(first.sig, second.sig);
        swap(first.activeState, second.activeState);
        swap(first.sigTime, second.sigTime);
        swap(first.lastDseep, second.lastDseep);
        swap(first.lastTimeSeen, second.lastTimeSeen);
        swap(first.cacheInputAge, second.cacheInputAge);
        swap(first.cacheInputAgeBlock, second.cacheInputAgeBlock);
        swap(first.unitTest, second.unitTest);
        swap(first.allowFreeTx, second.allowFreeTx);
        swap(first.protocolVersion, second.protocolVersion);
        swap(first.nLastDsq, second.nLastDsq);
        swap(first.nVote, second.nVote);
        swap(first.lastVote, second.lastVote);
        swap(first.nScanningErrorCount, second.nScanningErrorCount);
        swap(first.nLastScanningErrorBlockHeight, second.nLastScanningErrorBlockHeight);
    }

    CBasenode& operator=(CBasenode from)
    {
        swap(*this, from);
        return *this;
    }
    friend bool operator==(const CBasenode& a, const CBasenode& b)
    {
        return a.vin == b.vin;
    }
    friend bool operator!=(const CBasenode& a, const CBasenode& b)
    {
        return !(a.vin == b.vin);
    }

    uint256 CalculateScore(int mod=1, int64_t nBlockHeight=0);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion){
        // serialized format:
        // * version byte (currently 0)
        // * all fields (?)
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(vin);
                READWRITE(addr);
                READWRITE(pubkey);
                READWRITE(pubkey2);
                READWRITE(sig);
                READWRITE(activeState);
                READWRITE(sigTime);
                READWRITE(lastDseep);
                READWRITE(lastTimeSeen);
                READWRITE(cacheInputAge);
                READWRITE(cacheInputAgeBlock);
                READWRITE(unitTest);
                READWRITE(allowFreeTx);
                READWRITE(protocolVersion);
                READWRITE(nLastDsq);
                READWRITE(nVote);
                READWRITE(lastVote);
                READWRITE(nScanningErrorCount);
                READWRITE(nLastScanningErrorBlockHeight);
        }
	}

    void UpdateLastSeen(int64_t override=0)
    {
        if(override == 0){
            lastTimeSeen = GetAdjustedTime();
        } else {
            lastTimeSeen = override;
        }
    }

    inline uint64_t SliceHash(uint256& hash, int slice)
    {
        uint64_t n = 0;
        memcpy(&n, &hash+slice*64, 64);
        return n;
    }

    void Check();

    bool UpdatedWithin(int seconds)
    {
        // LogPrintf("UpdatedWithin %d, %d --  %d \n", GetAdjustedTime() , lastTimeSeen, (GetAdjustedTime() - lastTimeSeen) < seconds);

        return (GetAdjustedTime() - lastTimeSeen) < seconds;
    }

    void Disable()
    {
        lastTimeSeen = 0;
    }

    bool IsEnabled()
    {
        return activeState == BASENODE_ENABLED;
    }

    int GetBasenodeInputAge()
    {
        if(chainActive.Tip() == NULL) return 0;

        if(cacheInputAge == 0){
            cacheInputAge = GetInputAge(vin);
            cacheInputAgeBlock = chainActive.Tip()->nHeight;
        }

        return cacheInputAge+(chainActive.Tip()->nHeight-cacheInputAgeBlock);
    }

    void ApplyScanningError(CBasenodeScanningError& mnse)
    {
        if(!mnse.IsValid()) return;

        if(mnse.nBlockHeight == nLastScanningErrorBlockHeight) return;
        nLastScanningErrorBlockHeight = mnse.nBlockHeight;

        if(mnse.nErrorType == SCANNING_SUCCESS){
            nScanningErrorCount--;
            if(nScanningErrorCount < 0) nScanningErrorCount = 0;
        } else { //all other codes are equally as bad
            nScanningErrorCount++;
            if(nScanningErrorCount > BASENODE_SCANNING_ERROR_THESHOLD*2) nScanningErrorCount = BASENODE_SCANNING_ERROR_THESHOLD*2;
        }
    }

    std::string Status() {
        std::string strStatus = "ACTIVE";

        if(activeState == CBasenode::BASENODE_ENABLED) strStatus   = "ENABLED";
        if(activeState == CBasenode::BASENODE_EXPIRED) strStatus   = "EXPIRED";
        if(activeState == CBasenode::BASENODE_VIN_SPENT) strStatus = "VIN_SPENT";
        if(activeState == CBasenode::BASENODE_REMOVE) strStatus    = "REMOVE";
        if(activeState == CBasenode::BASENODE_POS_ERROR) strStatus = "POS_ERROR";

        return strStatus;
    }

};

// for storing the winning payments
class CBasenodePaymentWinner
{
public:
    int nBlockHeight;
    CTxIn vin;
    CScript payee;
    std::vector<unsigned char> vchSig;
    uint64_t score;

    CBasenodePaymentWinner() {
        nBlockHeight = 0;
        score = 0;
        vin = CTxIn();
        payee = CScript();
    }

    uint256 GetHash(){
        uint256 n2 = Hash(BEGIN(nBlockHeight), END(nBlockHeight));
        uint256 n3 = vin.prevout.hash > n2 ? (vin.prevout.hash - n2) : (n2 - vin.prevout.hash);

        return n3;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion){
        READWRITE(nBlockHeight);
        READWRITE(payee);
        READWRITE(vin);
        READWRITE(score);
        READWRITE(vchSig);
     }
};

//
// Basenode Payments Class
// Keeps track of who should get paid for which blocks
//

class CBasenodePayments
{
private:
    std::vector<CBasenodePaymentWinner> vWinning;
    int nSyncedFromPeer;
    std::string strMasterPrivKey;
    std::string strTestPubKey;
    std::string strMainPubKey;
    bool enabled;
    int nLastBlockHeight;

public:

    CBasenodePayments() {
        strMainPubKey = "04549ac134f694c0243f503e8c8a9a986f5de6610049c40b07816809b0d1d06a21b07be27b9bb555931773f62ba6cf35a25fd52f694d4e1106ccd237a7bb899fdd";
        strTestPubKey = "046f78dcf911fbd61910136f7f0f8d90578f68d0b3ac973b5040fb7afb501b5939f39b108b0569dca71488f5bbf498d92e4d1194f6f941307ffd95f75e76869f0e";
        enabled = false;
    }

    bool SetPrivKey(std::string strPrivKey);
    bool CheckSignature(CBasenodePaymentWinner& winner);
    bool Sign(CBasenodePaymentWinner& winner);

    // Deterministically calculate a given "score" for a basenode depending on how close it's hash is
    // to the blockHeight. The further away they are the better, the furthest will win the election
    // and get paid this block
    //

    uint64_t CalculateScore(uint256 blockHash, CTxIn& vin);
    bool GetWinningBasenode(int nBlockHeight, CTxIn& vinOut);
    bool AddWinningBasenode(CBasenodePaymentWinner& winner);
    bool ProcessBlock(int nBlockHeight);
    void Relay(CBasenodePaymentWinner& winner);
    void Sync(CNode* node);
    void CleanPaymentList();
    int LastPayment(CBasenode& mn);

    //slow
    bool GetBlockPayee(int nBlockHeight, CScript& payee);
};


#endif
