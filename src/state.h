// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_STATE_H
#define COIN_STATE_H

#include <string>
#include "sync.h"

enum eNodeType
{
    NT_FULL = 1,
    NT_THIN,
    NT_UNKNOWN // end marker
};

enum eNodeState
{
    NS_STARTUP = 1,
    NS_GET_HEADERS,
    NS_GET_FILTERED_BLOCKS,
    NS_READY,
    
    NS_UNKNOWN // end marker
};

enum eBlockFlags
{
    BLOCK_PROOF_OF_STAKE = (1 << 0), // is proof-of-stake block
    BLOCK_STAKE_ENTROPY  = (1 << 1), // entropy bit for stake modifier
    BLOCK_STAKE_MODIFIER = (1 << 2), // regenerated stake modifier
};


/*  nServices flags
    top 32 bits of CNode::nServices are used to mark services required 
*/



extern int nNodeMode;
extern int nNodeState;

extern int nMaxThinPeers;
extern int nBloomFilterElements;
extern int nThinIndexWindow;

extern bool fDebugChain;
extern bool fDebugRingSig;
extern bool fPrintToDebugger;
extern bool fRequestShutdown;
extern bool fShutdown;
extern bool fDaemon;

extern bool fCommandLine;

extern bool fNoListen;
extern bool fLogTimestamps;
extern bool fThinFullIndex;
extern bool fReindexing;




#endif /* COIN_STATE_H */