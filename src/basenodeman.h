// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BASENODEMAN_H
#define BASENODEMAN_H

//#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script/script.h"
#include "base58.h"
#include "main.h"
#include "basenode.h"

#define BASENODES_DUMP_SECONDS               (15*60)
#define BASENODES_DSEG_SECONDS               (3*60*60)

using namespace std;

class CBasenodeMan;

extern CBasenodeMan mnodeman;
void DumpBasenodes();

/** Access to the MN database (nodecache.dat)
 */
class CBasenodeDB
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

    CBasenodeDB();
    bool Write(const CBasenodeMan &mnodemanToSave);
    ReadResult Read(CBasenodeMan& mnodemanToLoad);
};

class CBasenodeMan
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    // map to hold all MNs

    // who's asked for the Basenode list and the last time
    std::map<CNetAddr, int64_t> mAskedUsForBasenodeList;
    // who we asked for the Basenode list and the last time
    std::map<CNetAddr, int64_t> mWeAskedForBasenodeList;
    // which Basenodes we've asked for
    std::map<COutPoint, int64_t> mWeAskedForBasenodeListEntry;

public:
    // keep track of dsq count to prevent basenodes from gaming darksend queue
    int64_t nDsqCount;

    std::vector<CBasenode> vBasenodes;

ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {

        // serialized format:
        // * version byte (currently 0)
        // * basenodes vector
        {
                LOCK(cs);
                unsigned char nVersion = 0;
                READWRITE(nVersion);
                READWRITE(vBasenodes);
                READWRITE(mAskedUsForBasenodeList);
                READWRITE(mWeAskedForBasenodeList);
                READWRITE(mWeAskedForBasenodeListEntry);
                READWRITE(nDsqCount);
        }
 	}

    CBasenodeMan();
    CBasenodeMan(CBasenodeMan& other);

    /// Add an entry
    bool Add(CBasenode &mn);

    /// Check all Basenodes
    void Check();

    /// Check all Basenodes and remove inactive
    void CheckAndRemove();

    /// Clear Basenode vector
    void Clear();

    int CountEnabled();

    int CountBasenodesAboveProtocol(int protocolVersion);

    void DsegUpdate(CNode* pnode);

    /// Find an entry
    CBasenode* Find(const CTxIn& vin);
    CBasenode* Find(const CPubKey& pubKeyBasenode);

    /// Find an entry thta do not match every entry provided vector
    CBasenode* FindOldestNotInVec(const std::vector<CTxIn> &vVins, int nMinimumAge, int nMinimumActiveSeconds);

    /// Find a random entry
    CBasenode* FindRandom();

    /// Get the current winner for this block
    CBasenode* GetCurrentBaseNode(int mod=1, int64_t nBlockHeight=0, int minProtocol=0);

    std::vector<CBasenode> GetFullBasenodeVector() { Check(); return vBasenodes; }

    std::vector<pair<int, CBasenode> > GetBasenodeRanks(int64_t nBlockHeight, int minProtocol=0);
    int GetBasenodeRank(const CTxIn &vin, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);
    CBasenode* GetBasenodeByRank(int nRank, int64_t nBlockHeight, int minProtocol=0, bool fOnlyActive=true);

    void ProcessBasenodeConnections();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

    /// Return the number of (unique) Basenodes
    int size() { return vBasenodes.size(); }

    std::string ToString() const;

    //
    // Relay Basenode Messages
    //

    void RelayBasenodeEntry(const CTxIn vin, const CService addr, const std::vector<unsigned char> vchSig, const int64_t nNow, const CPubKey pubkey, const CPubKey pubkey2, const int count, const int current, const int64_t lastUpdated, const int protocolVersion);
    void RelayBasenodeEntryPing(const CTxIn vin, const std::vector<unsigned char> vchSig, const int64_t nNow, const bool stop);

    void Remove(CTxIn vin);

};

#endif
