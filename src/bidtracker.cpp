#include "bidtracker.h"
#include "banknodeman.h"
#include "voting.h"
#include "wallet.h"
#include "base58.h"

#include "util.h"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <sstream>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
//for (int i = 0; std::getline(f, line); ++i)
string remove(string input, char m)
{
  input.erase(std::remove(input.begin(),input.end(), m),input.end());

  return input;
}

std::string replacestring(std::string subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
}

double Bidtracker::getbalance(string url)
{
    const char * c = url.c_str();

      std::string readBuffer;
      CAmount balance;
      curl = curl_easy_init();
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;

      return balance;
}

void Bidtracker::btcsortunspent(){

	ifstream myfile ((GetDataDir()/ "bidtracker/btcunspentraw.dat").string().c_str());
	std::ofstream myfile2;
	myfile2.open((GetDataDir()/ "bidtracker/btcbids.dat").string().c_str(),fstream::out);

	std::string line, txid, url;
    try
    {

	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("			tx_hash_big_endian:");
			size_t g = line.find("			value:");

			search = "tx_hash_big_endian";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("			tx_hash_big_endian:").length(), "");
				semp = remove(semp, ',');
				txid = semp;
				url = "https://blockchain.info/rawtx/"+ txid ;
				const char * d = url.c_str();
				string readBuffer;
				curl = curl_easy_init();
				if(curl) {
					curl_easy_setopt(curl, CURLOPT_URL, d);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
				}
				std::size_t pos1 = readBuffer.find("value");
				readBuffer = readBuffer.substr(0,pos1);
				readBuffer = remove(readBuffer, '"');
				readBuffer = remove(readBuffer, '{');
				readBuffer = remove(readBuffer,'}');
				readBuffer = remove(readBuffer, '[');
				readBuffer = remove(readBuffer, '\n');
				std::string uemp =readBuffer;
				std::size_t pos2 = uemp.find("addr:");
				uemp = uemp.substr(pos2);
				uemp = replacestring(uemp, "addr:", "");
				erase_all(uemp, " ");
				myfile2 << uemp;
			}

			search2 = "value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("			value:").length(), "");
				string value = semp;
				value = remove(value, ',');
				long double amount = atof(value.c_str());
				myfile2 << std::fixed <<  amount << std::endl;
			}
		}
		myfile.close();
		myfile2.close();
	}
	}

    catch (std::exception const &exc)
    {

    }
    catch (...)
    {

    }
}

void Bidtracker::btcsortunspentbackup(){

	ifstream myfile ((GetDataDir()/ "bidtracker/btcunspentrawbackup.dat").string().c_str());
	std::ofstream myfile2;
	myfile2.open((GetDataDir()/ "bidtracker/btcbidsbackup.dat").string().c_str(),fstream::out);
    try
    {
	std::string line, txid, url;
    char * pEnd;
	if (myfile.is_open()){
		while (myfile.good()){
			getline(myfile,line);
			line = line.erase(line.find("txid:"), 5);
			line = line.erase(line.find("amount:"), 7);
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			long double amount = strtoll(strs[5].c_str(),&pEnd,10) *COIN;
			txid = strs[1].c_str();
				url = "https://blockchain.info/rawtx/"+ txid ;

				const char * d = url.c_str();
				string readBuffer;
				curl = curl_easy_init();
				if(curl) {
					curl_easy_setopt(curl, CURLOPT_URL, d);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
					res = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
				}
				std::size_t pos1 = readBuffer.find("value");
				readBuffer = readBuffer.substr(0,pos1);
				readBuffer = remove(readBuffer, '"');
				readBuffer = remove(readBuffer, '{');
				readBuffer = remove(readBuffer,'}');
				readBuffer = remove(readBuffer, '[');
				readBuffer = remove(readBuffer, '\n');
				std::string uemp =readBuffer;
				std::size_t pos2 = uemp.find("addr:");
				uemp = uemp.substr(pos2);
				uemp = replacestring(uemp, "addr:", "");
				erase_all(uemp, " ");
				myfile2 << uemp <<amount<< endl;
			}
	//myfile2 << strs[1].c_str() << "," << std::fixed << amount << std::endl;


	myfile.close();
	myfile2.close();
	}

	}
    catch (std::exception const &exc)
    {

    }
    catch (...)
    {

    }
}

void Bidtracker::btcgetunspentbackup()
{
    string address = "1BCRbid2i3wbgqrKtgLGem6ZchcfYbnhNu";

    string url = "https://blockexplorer.com/api/addr/"+ address + "/utxo";
    try
    {
    const char * c = url.c_str() ;

      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

    readBuffer = remove(readBuffer, '[');
    readBuffer = remove(readBuffer, ']');
    readBuffer = replacestring(readBuffer, "},", "}\n");
    readBuffer = remove(readBuffer, '{');
    readBuffer = remove(readBuffer, '}');
    readBuffer = remove(readBuffer, '"');
	ofstream myfile((GetDataDir().string() + "/bidtracker/btcunspentrawbackup.dat").c_str(),fstream::out);
	myfile << readBuffer<< std::endl;
	myfile.close();
	}

    catch (std::exception const &exc)
    {

    }
    catch (...)
    {

    }
}

void Bidtracker::btcgetunspent()
{
    std::string address = "1BCRbid2i3wbgqrKtgLGem6ZchcfYbnhNu";

    std::string url;
    url = "https://blockchain.info/unspent?active=" + address;
    try
    {
    const char * c = url.c_str();

      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

    boost::filesystem::path biddir = GetDataDir() / "bidtracker";

    if(!(boost::filesystem::exists(biddir))){
        if(fDebug)LogPrintf("Biddir Doesn't Exists\n");

        if (boost::filesystem::create_directory(biddir))
            if(fDebug)LogPrintf("Biddir....Successfully Created !\n");
    }

	ofstream myfile((GetDataDir().string() + "/bidtracker/btcunspentraw.dat").c_str(),fstream::out);
	readBuffer = remove(readBuffer, ' ');
	readBuffer = remove(readBuffer, '"');
	myfile << readBuffer << std::endl;
	myfile.close();
	}

    catch (std::exception const &exc)
    {

    }
    catch (...)
    {

    }
}

double Bidtracker::btcgetprice()
{
	CAmount price;
    std::string url;
    url = "https://blockchain.info/q/24hrprice";

    const char * c = url.c_str();

      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      price = atof(readBuffer.c_str());

      return price;
}

double Bidtracker::bcrgetprice()
{
	double price;
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-BCR";

    const char * c = url.c_str();

      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::size_t pos = readBuffer.find(",\"A");
	readBuffer = readBuffer.substr(0,pos);
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, ',');
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, ':');
	readBuffer = remove(readBuffer, '{');
	readBuffer = replacestring(readBuffer, "successtrue", "");
	readBuffer = replacestring(readBuffer, "message", "");
	readBuffer = replacestring(readBuffer, "resultBid", "");
	readBuffer = remove(readBuffer, '\n');
    if ( ! (istringstream(readBuffer) >> price) ) price = 0;

      return price;
}

double Bidtracker::usdbtc(){

return btcgetprice();

}

long double Bidtracker::bcrbtc(){

return bcrgetprice();

}

void Bidtracker::combine()
{
	std::ofstream myfile;
	myfile.open((GetDataDir() /"bidtracker/prefinal.dat").string().c_str(),fstream::out);
	ifstream myfile2((GetDataDir() /"bidtracker/btcbids.dat").string().c_str());
	ifstream myfile3((GetDataDir() /"bidtracker/btcbidsbackup.dat").string().c_str());


	if (myfile2.is_open()){
		std::string line;
		while ( myfile2.good() ){
			getline (myfile2,line);
	myfile<<line<<endl;
	}	}
	if (myfile3.is_open()){
		std::string line;
		while ( myfile3.good() ){
			getline (myfile3,line);
	myfile<<line<<endl;
	}	}

	myfile.close();
	myfile2.close();
	myfile3.close();
	remove((GetDataDir() /"bidtracker/btcbids.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/btcbidsbackup.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/btcunspentraw.dat").string().c_str());
	remove((GetDataDir() /"bidtracker/btcunspentrawbackup.dat").string().c_str());
}

int totalbid;
std::map<std::string,double>::iterator brit;
void sortbidtracker(){
	std::map<std::string,double> finalbids;
	fstream myfile2((GetDataDir() /"bidtracker/prefinal.dat").string().c_str());
	totalbid=0;
	char * pEnd;
	std::string line;
	while (getline(myfile2, line)){
		if (!line.empty()) {
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			totalbid+=strtoll(strs[1].c_str(),&pEnd,10);
			finalbids[strs[0]]+=strtoll(strs[1].c_str(),&pEnd,10);
		}
	}

	ofstream myfile;
	myfile.open((GetDataDir() /"bidtracker/final.dat").string().c_str(), std::ofstream::trunc);
	myfile << std::fixed << setprecision(8);
	for(brit = finalbids.begin();brit != finalbids.end(); ++brit){
		myfile << brit->first << "," << (brit->second)/totalbid << endl;
	}

	myfile2.close();
	myfile.close();
}

std::map<std::string,double> getbidtracker(){

	std::map<std::string,double> finals;
	fstream myfile((GetDataDir() /"bidtracker/final.dat").string().c_str());
	char * pEnd;
	std::string line;
	while (getline(myfile, line)){
		if (!line.empty()) {
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			finals[strs[0]]=strtod(strs[1].c_str(),&pEnd);
		}
	}
	return finals;
}

void getbids(){

	int64_t nStart = GetTimeMillis();
	Bidtracker h;
	h.btcgetunspent();
	h.btcgetunspentbackup();
	h.btcsortunspent();
	h.btcsortunspentbackup();
	h.combine();
	sortbidtracker();
	if(fDebug)LogPrintf("Bids dump finished  %dms\n", GetTimeMillis() - nStart);

}

