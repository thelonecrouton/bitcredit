// Copyright (c) 2014 The BCR developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef RPCWALLET_H
#define RPCWALLET_H

#include "wallet.h"
#include "walletdb.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"

void SendMoney(const CTxDestination &address, CAmount nValue, CWalletTx& wtxNew);

#endif 
