// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BANKNODEMAN_H
#define BANKNODEMAN_H

//#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "main.h"
#include "banknode.h"

#define BANKNODES_DUMP_SECONDS               (15*60)
#define BANKNODES_DSEG_SECONDS               (3*60*60)

using namespace std;

class CBanknodeMan;

extern CBanknodeMan mnodeman;
void DumpBanknodes();

/** Access to the MN database (nodecache.dat)
 */
class CBanknodeDB
{
private:
    boost::filesystem::path pathMN;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CBanknodeDB();
    bool Write(const CBanknodeMan &mnodemanToSave);
    ReadResult Read(CBanknodeMan& mnodemanToLoad);
};

class CBanknodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs

    // who's asked for the Banknode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForBanknodeList;
    // who we asked for the Banknode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForBanknodeList;
    // which Banknodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForBanknodeListEntry;

public:
    // keep track of dsq count to prevent banknodes from gaming darksend queue
    int64_t nDsqCount;

    std::vector<CBanknode> vBanknodes;

ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {

        // serialized format:
        // * version byte (currently 0)
        // * banknodes vector
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(vBanknodes);
                READWRITE(mAskedUsForBanknodeList);
                READWRITE(mWeAskedForBanknodeList);
                READWRITE(mWeAskedForBanknodeListEntry);
                READWRITE(nDsqCount);
        }
 	}

    CBanknodeMan();
    CBanknodeMan(CBanknodeMan& other);

    /// Add an entry
    bool Add(CBanknode &mn);

    /// Check all Banknodes
    void Check();

    /// Check all Banknodes and remove inactive
    void CheckAndRemove();

    /// Clear Banknode vector
    void Clear();

    int CountEnabled();

    int CountBanknodesAboveProtocol(int protocolVersion);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CBanknode* Find(const CTxIn& vin);
    CBanknode* Find(const CPubKey& pubKeyBanknode);

    /// Find an entry thta do not match every entry provided vector
    CBanknode* FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds);

    /// Find a random entry
    CBanknode* FindRandom();

    /// Get the current winner for this block
    CBanknode* GetCurrentBankNode(int mod=1, int64_t nBlockHeight=0, int minProtocol=0);

    std::vector<CBanknode> GetFullBanknodeVector() { Check(); return vBanknodes; }

    std::vector<pair<int, CBanknode> > GetBanknodeRanks(int64_t nBlockHeight, int minProtocol=0);
    int GetBanknodeRank(const CTxIn &vin, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);
    CBanknode* GetBanknodeByRank(int nRank, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);

    void ProcessBanknodeConnections();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Return the number of (unique) Banknodes
    int size() { return vBanknodes.size(); }

    std::string ToString() const;

    //
    // Relay Banknode Messages
    //

    void RelayBanknodeEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion);
    void RelayBanknodeEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);

    void Remove(CTxIn vin);

};

#endif
