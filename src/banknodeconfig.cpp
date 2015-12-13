
#include "net.h"
#include "basenodeconfig.h"
#include "util.h"

CBasenodeConfig basenodeConfig;

void CBasenodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CBasenodeEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CBasenodeConfig::read(std::string& strErr) {
    boost::filesystem::ifstream streamConfig(GetBasenodeConfigFile());
    if (!streamConfig.good()) {
        return true; // No basenode.conf file is OK
    }

    for(std::string line; std::getline(streamConfig, line); )
    {
        if(line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::string alias, ip, privKey, txHash, outputIndex;
        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            strErr = "Could not parse basenode.conf line: " + line;
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}
