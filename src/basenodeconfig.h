// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcredit developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_BASENODECONFIG_H_
#define SRC_BASENODECONFIG_H_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

class CBasenodeConfig;
extern CBasenodeConfig basenodeConfig;

class CBasenodeConfig
{

public:

    class CBasenodeEntry {

    private:
        std::string alias;
        std::string ip;
        std::string privKey;
        std::string txHash;
        std::string outputIndex;
    public:

        CBasenodeEntry(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
            this->alias = alias;
            this->ip = ip;
            this->privKey = privKey;
            this->txHash = txHash;
            this->outputIndex = outputIndex;
        }

        const std::string& getAlias() const {
            return alias;
        }

        void setAlias(const std::string& alias) {
            this->alias = alias;
        }

        const std::string& getOutputIndex() const {
            return outputIndex;
        }

        void setOutputIndex(const std::string& outputIndex) {
            this->outputIndex = outputIndex;
        }

        const std::string& getPrivKey() const {
            return privKey;
        }

        void setPrivKey(const std::string& privKey) {
            this->privKey = privKey;
        }

        const std::string& getTxHash() const {
            return txHash;
        }

        void setTxHash(const std::string& txHash) {
            this->txHash = txHash;
        }

        const std::string& getIp() const {
            return ip;
        }

        void setIp(const std::string& ip) {
            this->ip = ip;
        }
    };

    CBasenodeConfig() {
        entries = std::vector<CBasenodeEntry>();
    }

    void clear();
    bool read(std::string& strErr);
	void add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex);

    std::vector<CBasenodeEntry>& getEntries() {
        return entries;
    }

    int getCount() {
        int c = -1;
        BOOST_FOREACH(CBasenodeEntry e, entries) {
            if(e.getAlias() != "") c++;
        }
        return c;
    }

private:
    std::vector<CBasenodeEntry> entries;


};


#endif /* SRC_BASENODECONFIG_H_ */
