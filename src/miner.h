// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_MINER_H
#define BITCREDIT_MINER_H

#include <stdint.h>
#include "bidtracker.h"
class CBlock;
class CBlockHeader;
class CBlockIndex;
class CReserveKey;
class CScript;
class CWallet;

struct CBlockTemplate;
extern std::map<std::string,long double> getbidtracker();
/** Run the miner threads */
void GenerateBitcredits(bool fGenerate, CWallet* pwallet, int nThreads);
/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn);
CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
/** Check mined block */
bool ProcessBlockFound(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey);
void UpdateTime(CBlockHeader* block, const CBlockIndex* pindexPrev);
extern double dHashesPerMin;
extern int64_t nHPSTimerStart;
void miningbanknodelist();
#endif // BITCREDIT_MINER_H
