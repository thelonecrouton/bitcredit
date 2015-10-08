// Copyright (c) 2012-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_VERSION_H
#define BITCREDIT_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 70013;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 70013;

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 31811;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70013;

static const int MIN_INSTANTX_PROTO_VERSION = 70013;

static const int MIN_POOL_PEER_PROTO_VERSION = 70013;

static const int MIN_MN_PROTO_VERSION = 70013;

static const int MIN_SMSG_PROTO_VERSION = 70013;

static const int MIN_IBTP_PROTO_VERSION = 70013;

static const int MIN_VOTE_PROTO_VERSION = 70013;

static const int CADDR_ADVERTISED_BALANCE_VERSION = 70013;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 32000;
static const int NOBLKS_VERSION_END = 32400;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "mempool" command, enhanced "getdata" behavior starts with this version
static const int MEMPOOL_GD_VERSION = 60002;


//!use this file for advanced version control, as well as tracking
#endif // BITCREDIT_VERSION_H
