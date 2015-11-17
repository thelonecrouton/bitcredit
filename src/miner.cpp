// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"
#include "activebanknode.h"
#include "amount.h"
#include "base58.h"
#include "bidtracker.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "hash.h"
#include "main.h"
#include "net.h"
#include "pow.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#include "voting.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif
#include "banknodeman.h"
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>
#include <stdio.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcreditMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.
// The COrphan class keeps track of these 'temporary orphans' while
// CreateBlock is figuring out which transactions to include.
//
class COrphan
{
public:
    const CTransaction* ptx;
    set<uint256> setDependsOn;
    CFeeRate feeRate;
    double dPriority;

    COrphan(const CTransaction* ptxIn) : ptx(ptxIn), feeRate(0), dPriority(0)
    {
    }
};

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

// We want to sort transactions by priority and fee rate, so:
typedef boost::tuple<double, CFeeRate, const CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;

public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }

    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

string convertAddress(const char address[], char newVersionByte){
    std::vector<unsigned char> v;
    DecodeBase58Check(address,v);
    v[0]=newVersionByte;
    string result = EncodeBase58Check(v);
    return result;
}

void UpdateTime(CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    pblock->nTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    // Updating time can change work required on testnet:
    if (Params().AllowMinDifficultyBlocks())
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock);
}

CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn)
{
    // Create new block
    auto_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if(!pblocktemplate.get())
        return NULL;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (Params().MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);
	int payments = 0;
	double bidstotal= 0;
    bool hasPayment = false;
    bool isgrantblock = false;
    bool ispayoutblock = false;
	std::map<std::string,double> bidtracker = getbidtracker();
	std::map<std::string,double>::iterator balit;
    // Create coinbase tx
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();

    if(GetTimeMicros() > 1427803200){
            hasPayment = true;
            if(!banknodePayments.GetBlockPayee(chainActive.Tip()->nHeight+1, pblock->payee)){
                CBanknode* winningNode = mnodeman.GetCurrentBankNode(1);
                if(winningNode){
                    pblock->payee =GetScriptForDestination(winningNode->pubkey.GetID());
                } else {
                    LogPrintf("CreateNewBlock: Failed to detect banknode to pay\n");
                    hasPayment = false;
                }
            }
        }

	if((chainActive.Tip()->nHeight +1)% 900==0){
		ispayoutblock=true;
	}

	if(isGrantAwardBlock(chainActive.Tip()->nHeight +1)){
		if( !getGrantAwards(chainActive.Tip()->nHeight+1) ){
			throw std::runtime_error( "ConnectBlock() : Connect Block grant awards error.\n" );
		}
		isgrantblock = true;
		if(fDebug)LogPrintf("Retrieved Grant Rewards, Add to Block %d \n", chainActive.Tip()->nHeight+1);
	}

   { //coinbase size
	    LOCK(grantdb);
		if(hasPayment && ispayoutblock && isgrantblock){
			payments++;
			txNew.vout.resize(bidtracker.size()+ payments+ 3 + grantAwards.size());
		}
		else if(hasPayment && ispayoutblock){
			payments++;
			txNew.vout.resize(bidtracker.size()+ payments+ 3 );
		}
		else if(hasPayment && isgrantblock){
			payments++;
			txNew.vout.resize(payments+ 3 + grantAwards.size());
		}
		else if(ispayoutblock && isgrantblock){
			txNew.vout.resize(3+ bidtracker.size() + grantAwards.size());
		}
		else if(ispayoutblock){
			txNew.vout.resize(3+ bidtracker.size());
		}
		else if(isgrantblock){
			txNew.vout.resize(3+ grantAwards.size());
		}
		else if(hasPayment){
			payments++;
			txNew.vout.resize(3+ payments);
		}
		else{
			txNew.vout.resize(3);
		}
   }

   { //coinbase address

	txNew.vout[0].scriptPubKey = scriptPubKeyIn;
	txNew.vout[1].scriptPubKey = BANK_SCRIPT;
	txNew.vout[2].scriptPubKey = RESERVE_SCRIPT;

	if(hasPayment && ispayoutblock && isgrantblock){
		txNew.vout[2+ payments].scriptPubKey = pblock->payee;
		int j = 3+ payments;
		for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
				CBitcreditAddress address(convertAddress(gait->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[j].scriptPubKey= GetScriptForDestination(dest);
				j++;
			}
		int i = 3+ payments + grantAwards.size();
		for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
				CBitcreditAddress address(convertAddress(balit->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				bidstotal+=balit->second;
				i++;
			}
		}
	else if(hasPayment && ispayoutblock){
		txNew.vout[2+ payments].scriptPubKey = pblock->payee;
		int i = 3+ payments;
		for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
				CBitcreditAddress address(convertAddress(balit->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				bidstotal+=balit->second;
				i++;
			}
		}
	else if(hasPayment && isgrantblock){
		txNew.vout[2+ payments].scriptPubKey = pblock->payee;
		int i = 3+ payments;
		for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
				CBitcreditAddress address(convertAddress(gait->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				i++;
			}
		}
	else if(ispayoutblock && isgrantblock){
		int j = 3;
		for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
				CBitcreditAddress address(convertAddress(gait->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[j].scriptPubKey= GetScriptForDestination(dest);
				j++;
			}
		int i = 3+ grantAwards.size();
		for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
				CBitcreditAddress address(convertAddress(balit->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				bidstotal+=balit->second;
				i++;
			}
		}
	else if(ispayoutblock){
		int i = 3;
		for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
				CBitcreditAddress address(convertAddress(balit->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				bidstotal+=balit->second;
				i++;
			}
		}
	else if(isgrantblock){
		int i = 3;
		for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
				CBitcreditAddress address(convertAddress(gait->first.c_str(),0x0c));
				CTxDestination dest = address.Get();
				txNew.vout[i].scriptPubKey= GetScriptForDestination(dest);
				i++;
			}
		}
	else if(hasPayment){
		txNew.vout[2+ payments].scriptPubKey = pblock->payee;
		}
   }

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        CCoinsViewCache view(pcoinsTip);

        UpdateTime(pblock, pindexPrev);
		uint64_t nBlockTime = pblock->GetBlockTime();

        // Largest block you're willing to create:
        unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
        // Limit to betweeen 1K and max-size-minus-1K for sanity:
        nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MaxBlockSize(nBlockTime)), nBlockMaxSize));

        // How much of the block should be dedicated to high-priority transactions,
        // included regardless of the fees they pay
        unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
        nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

        // Minimum block size you want to create; block will be filled with free transactions
        // until there are no more or the block reaches this size:
        unsigned int nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
        nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

        // Collect memory pool transactions into the block
        CAmount nFees = 0;

        // Priority order to process transactions
        list<COrphan> vOrphan; // list memory doesn't move
        map<uint256, vector<COrphan*> > mapDependers;
        bool fPrintPriority = GetBoolArg("-printpriority", false);

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());
        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin();
             mi != mempool.mapTx.end(); ++mi)
        {
            const CTransaction& tx = mi->second.GetTx();
            if (tx.IsCoinBase() || !IsFinalTx(tx, nHeight))
                continue;

            COrphan* porphan = NULL;
            double dPriority = 0;
            CAmount nTotalIn = 0;
            bool fMissingInputs = false;
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
            {
                // Read prev transaction
                if (!view.HaveCoins(txin.prevout.hash))
                {
                    // This should never happen; all transactions in the memory
                    // pool should connect to either transactions in the chain
                    // or other transactions in the memory pool.
                    if (!mempool.mapTx.count(txin.prevout.hash))
                    {
                        LogPrintf("ERROR: mempool transaction missing input\n");
                        if (fDebug) assert("mempool transaction missing input" == 0);
                        fMissingInputs = true;
                        if (porphan)
                            vOrphan.pop_back();
                        break;
                    }

                    // Has to wait for dependencies
                    if (!porphan)
                    {
                        // Use list for automatic deletion
                        vOrphan.push_back(COrphan(&tx));
                        porphan = &vOrphan.back();
                    }
                    mapDependers[txin.prevout.hash].push_back(porphan);
                    porphan->setDependsOn.insert(txin.prevout.hash);
                    nTotalIn += mempool.mapTx[txin.prevout.hash].GetTx().vout[txin.prevout.n].nValue;
                    continue;
                }
                const CCoins* coins = view.AccessCoins(txin.prevout.hash);
                assert(coins);

                CAmount nValueIn = coins->vout[txin.prevout.n].nValue;
                nTotalIn += nValueIn;

                int nConf = nHeight - coins->nHeight;

                dPriority += (double)nValueIn * nConf;
            }
            if (fMissingInputs) continue;

            // Priority is sum(valuein * age) / modified_txsize
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority = tx.ComputePriority(dPriority, nTxSize);

            uint256 hash = tx.GetHash();
            mempool.ApplyDeltas(hash, dPriority, nTotalIn);

            CFeeRate feeRate(nTotalIn-tx.GetValueOut(), nTxSize);

            if (porphan)
            {
                porphan->dPriority = dPriority;
                porphan->feeRate = feeRate;
            }
            else
                vecPriority.push_back(TxPriority(dPriority, feeRate, &mi->second.GetTx()));
        }

        // Collect transactions into block
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;
        bool fSortedByFee = (nBlockPrioritySize <= 0);

        TxPriorityCompare comparer(fSortedByFee);
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        while (!vecPriority.empty())
        {
            // Take highest priority transaction off the priority queue:
            double dPriority = vecPriority.front().get<0>();
            CFeeRate feeRate = vecPriority.front().get<1>();
            const CTransaction& tx = *(vecPriority.front().get<2>());

            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nTxSigOps = GetLegacySigOpCount(tx);
            if (nBlockSigOps + nTxSigOps >= MaxBlockSigops(nBlockTime))
                continue;

            // Skip free transactions if we're past the minimum block size:
            const uint256& hash = tx.GetHash();
            double dPriorityDelta = 0;
            CAmount nFeeDelta = 0;
            mempool.ApplyDeltas(hash, dPriorityDelta, nFeeDelta);
            if (fSortedByFee && (dPriorityDelta <= 0) && (nFeeDelta <= 0) && (feeRate < ::minRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            // Prioritise by fee once past the priority size or we run out of high-priority
            // transactions:
            if (!fSortedByFee &&
                ((nBlockSize + nTxSize >= nBlockPrioritySize) || !AllowFree(dPriority)))
            {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            }

            if (!view.HaveInputs(tx))
                continue;

            CAmount nTxFees = view.GetValueIn(tx)-tx.GetValueOut();

            nTxSigOps += GetP2SHSigOpCount(tx, view);
            if (nBlockSigOps + nTxSigOps >= MaxBlockSigops(nBlockTime))
                continue;

            // Note that flags: we don't want to set mempool/IsStandard()
            // policy here, but we still have to ensure that the block we
            // create only contains transactions that are valid in new blocks.
            CValidationState state;
            if (!CheckInputs(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS, true))
                continue;

            CTxUndo txundo;
            UpdateCoins(tx, state, view, txundo, nHeight);

            // Added
            pblock->vtx.push_back(tx);
            pblocktemplate->vTxFees.push_back(nTxFees);
            pblocktemplate->vTxSigOps.push_back(nTxSigOps);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            if (fPrintPriority)
            {
                LogPrintf("priority %.1f fee %s txid %s\n",
                    dPriority, feeRate.ToString(), tx.GetHash().ToString());
            }

            // Add transactions that depend on this one to the priority queue
            if (mapDependers.count(hash))
            {
                BOOST_FOREACH(COrphan* porphan, mapDependers[hash])
                {
                    if (!porphan->setDependsOn.empty())
                    {
                        porphan->setDependsOn.erase(hash);
                        if (porphan->setDependsOn.empty())
                        {
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->feeRate, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        }
                    }
                }
            }
        }

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;
        LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);
        CAmount blockValue = GetBlockValue(pindexPrev->nHeight+1, nFees);
        CAmount banknodePayment = GetBanknodePayment(pindexPrev->nHeight+1, blockValue);
        CAmount bank = GetBlockValue(pindexPrev->nHeight+1, nFees) *(0.1);

        // Compute final coinbase transaction.
		{
				txNew.vout[1].nValue = bank;
				blockValue -= bank;

				txNew.vout[2].nValue = bank;
				blockValue -= bank;
				if (hasPayment && ispayoutblock && isgrantblock){
					txNew.vout[2+ payments].nValue = banknodePayment;
					blockValue -= banknodePayment;
					int j = 3 + payments;
					for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
						txNew.vout[j].nValue= gait->second;
						blockValue -= gait->second;
						j++;
					}
					int i=3+payments + grantAwards.size();
					unsigned long int py = blockValue *0.99;
					for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
						unsigned long int bb =(balit->second)* py;
						txNew.vout[i].nValue = bb ;
						blockValue -= bb;
						i++;
					}

				}
				else if (ispayoutblock && isgrantblock){
					int j = 3;
					for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
						txNew.vout[j].nValue= gait->second;
						blockValue -= gait->second;
						j++;
					}
					int i=3 + grantAwards.size();
					unsigned long int py = blockValue *0.99;
					for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
						unsigned long int bb =balit->second* py;
						txNew.vout[i].nValue = bb ;
						blockValue -= bb;
						i++;
					}
				}
				else if (hasPayment && ispayoutblock){
					txNew.vout[2+ payments].nValue = banknodePayment;
					blockValue -= banknodePayment;
					unsigned long int py = blockValue *0.99;
					int i=3+payments;
					for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
						unsigned long int bb = balit->second * py;
						txNew.vout[i].nValue = bb ;
						blockValue -= bb;
						i++;
					}
				}
				else if (hasPayment && isgrantblock){
					txNew.vout[2+ payments].nValue = banknodePayment;
					blockValue -= banknodePayment;
					int i = 3+ payments;
					for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
						txNew.vout[i].nValue= gait->second;
						blockValue -= gait->second;
						i++;
					}
				}
				else if (ispayoutblock){
					int i=3;
					unsigned long int py = blockValue *0.99;
					for(balit = bidtracker.begin(); balit != bidtracker.end();balit++){
						unsigned long int bb =balit->second* py;
						txNew.vout[i].nValue = bb ;
						blockValue -= bb;
						i++;
					}
				}
				else if (isgrantblock){
					int i=3;
					for(gait = grantAwards.begin(); gait != grantAwards.end();gait++){
						txNew.vout[i].nValue= gait->second;
						blockValue -= gait->second;
						i++;
					}
				}
				else if(hasPayment){
					txNew.vout[2+ payments].nValue = banknodePayment;
					blockValue -= banknodePayment;
				}
				txNew.vout[0].nValue = blockValue;
		}

		if (fDebug){//debug payouts
			LogPrintf(" Bidtracker size:  %ld\n",bidtracker.size());
			for(unsigned int i=0; i < txNew.vout.size();i++){
				CAmount payout = txNew.vout[i].nValue;
				CTxDestination address;
				ExtractDestination(txNew.vout[i].scriptPubKey, address);
				string receiveAddress = CBitcreditAddress( address ).ToString().c_str();
				LogPrintf(" Payouts: %s :-, %ld\n",receiveAddress,payout);
				}
		}

        txNew.vin[0].scriptSig = CScript() << nHeight << OP_0;
        pblock->vtx[0] = txNew;
        pblocktemplate->vTxFees[0] = -nFees;
        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        UpdateTime(pblock, pindexPrev);
        pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock);
        pblock->nNonce         = 0;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        CValidationState state;
        if (!TestBlockValidity(state, *pblock, pindexPrev, false, false))
            throw std::runtime_error("CreateNewBlock() : TestBlockValidity failed");
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerMin = 0.0;
int64_t nHPSTimerStart = 0;
std::vector<std::string> miningkeys;

CBlockTemplate* CreateNewBlockWithKey()
{
	std::ifstream file((GetDataDir() / "miningkeys.dat" ).string().c_str());
	CScript scriptPubKey;
	std::string line;
	CTxDestination dest;

	while (std::getline(file, line)){
    if (!line.empty())
        miningkeys.push_back(line);
	}

	for(unsigned int i=0; i < miningkeys.size(); i++){
		CBitcreditAddress address(miningkeys[i]);
		dest = address.Get();
		scriptPubKey =  GetScriptForDestination(dest);

		if (std::find(last40blocks.begin(), last40blocks.end(), miningkeys[i]) != last40blocks.end())
		{
		LogPrintf("CreateNewBlockWithKey(): coinbase key %s detected in 40 block period\n",miningkeys[i]);
		continue;
		}
		
		break;
	}

	return CreateNewBlock(scriptPubKey);
}

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet)
{
    uint256 hash = pblock->GetVerifiedHash();
    uint256 hashTarget = uint256().SetCompact(pblock->nBits);

    if (hash > hashTarget)
        return false;
    LogPrintf("%s\n", pblock->ToString());
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("BitcreditMiner : generated block is stale");
    }

    // Track how many getdata requests this block gets
    {
        LOCK(wallet.cs_wallet);
        wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    // Process this block the same as if we had received it from another node
    CValidationState state;
    if (!ProcessNewBlock(state, NULL, pblock))
        return error("BitcreditMiner : ProcessNewBlock, block not accepted");

    return true;
}

void static BitcreditMiner(CWallet *pwallet)
{
    LogPrintf("BitcreditMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("bitcredit-miner");

    unsigned int nExtraNonce = 0;

    // get the address used for the last block, don't bother checking address validity,
    // that will be done in TestBlockValidity

    try {
        while (true) {
            if (Params().MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    bool fvNodesEmpty;
                    {
                        LOCK(cs_vNodes);
                        fvNodesEmpty = vNodes.empty();
                    }
                    if (!fvNodesEmpty )
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            auto_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey());
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in BitcreditMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }
            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            LogPrintf("Running BitcreditMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //
            // Search
            //
            int64_t nStart = GetTime();
            uint256 hashTarget = uint256().SetCompact(pblock->nBits);
            uint256 testHash;
        for (;;)
        {
            unsigned int nHashesDone = 0;
            unsigned int nNonceFound = (unsigned int) -1;

        for(int i=0;i<1;i++){
            pblock->nNonce=pblock->nNonce+1;
            testHash=pblock->CalculateBestBirthdayHash();
            nHashesDone++;
            LogPrintf("testHash %s\n", testHash.ToString().c_str());
            LogPrintf("Hash Target %s\n", hashTarget.ToString().c_str());

            if(testHash<hashTarget){
                nNonceFound=pblock->nNonce;
                LogPrintf("Found Hash %s\n", testHash.ToString().c_str());
                break;
            }
        }
            // Check if something found
            if (nNonceFound != (unsigned int) -1)
            {

                if (testHash <= hashTarget)
                {
                    // Found a solution

                    LogPrintf("hash %s\n", testHash.ToString().c_str());
                    LogPrintf("hash2 %s\n", pblock->GetHash().ToString().c_str());
                    assert(testHash == pblock->GetHash());

                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    ProcessBlockFound(pblock, *pwallet);
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);
                    break;
                }
            }

                // Meter hashes/sec
                static int64_t nHashCounter;
                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nHashCounter = 0;
                }
                else
                    nHashCounter += nHashesDone;
                if (GetTimeMillis() - nHPSTimerStart > 4000*60)
                {
                    static CCriticalSection cs;
                    {
                        LOCK(cs);
                        if (GetTimeMillis() - nHPSTimerStart > 4000*60)
                        {
                            dHashesPerMin = 1000.0 * nHashCounter *60 / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nHashCounter = 0;
                            static int64_t nLogTime;
                            if (GetTime() - nLogTime > 30 * 60)
                            {
                                nLogTime = GetTime();
                                LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerMin/1000.0);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();
                // Regtest mode doesn't require peers
                if (vNodes.empty() && Params().MiningRequiresPeers())
                    break;
                if (nNonceFound >= 0xffff0000)
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != chainActive.Tip())
                    break;

                // Update nTime every few seconds
                UpdateTime(pblock, pindexPrev);
                if (Params().AllowMinDifficultyBlocks())
                {
                    // Changing pblock->nTime can change work required on testnet:
                    hashTarget.SetCompact(pblock->nBits);
                }
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("BitcreditMiner terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("BitcreditMiner runtime error: %s\n", e.what());
        return;
    }

}

void GenerateBitcredits(bool fGenerate, CWallet* pwallet, int nThreads)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) {
        // In regtest threads defaults to 1
        if (Params().DefaultMinerThreads())
            nThreads = Params().DefaultMinerThreads();
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&BitcreditMiner, pwallet));
}

#endif // ENABLE_WALLET

