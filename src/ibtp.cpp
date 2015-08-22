// Copyright (c) 2014 The Sapience AIFX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ibtp.h"
#include "main.h"
#include "util.h"
#include "protocol.h"
#include "chainparams.h"
#include "net.h"
#include "utilstrencodings.h"
#include <map>

using namespace std;

void CIbtp::LoadMsgStart()
{
    vChains.push_back(SChain("Litecoin ", "LTC", 0xfb, 0xc0, 0xb6, 0xdb));
    vChains.push_back(SChain("Bitcoin", "BTC", 0xfa, 0xb2, 0xef, 0xf2));
    vChains.push_back(SChain("Dash", "DSH", 0xbf, 0x0c, 0x6b, 0xbd));
	vChains.push_back(SChain("Bitcredit", "BCR", 0xf9, 0xbe, 0xb4, 0xd9));

}

bool CIbtp::IsIbtpChain(const unsigned char msgStart[], std::string& chainName)
{
    bool bFound = false;
    CMessageHeader hdr;
    BOOST_FOREACH(SChain p, vChains)
    {
        unsigned char pchMsg[4] = { p.pchMessageOne, p.pchMessageTwo, p.pchMessageThree, p.pchMessageFour };
        //if(memcmp(hdr.pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE))
		if(memcmp(msgStart, Params().MessageStart(), MESSAGE_START_SIZE !=0))
        {            
			if(memcmp(msgStart, pchMsg, sizeof(pchMsg)) == 0)
            {
                bFound = true;
                chainName = p.sChainName;
                printf("Found IBTP chain: %s\n", p.sChainName.c_str());
            }
        }
    }

    return bFound;
}
