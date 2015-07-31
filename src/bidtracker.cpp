#include "bidtracker.h"
#include "util.h"
#include "base58.h"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <map>
#include "json/json_spirit.h"
#include "rpcclient.h"
#include <sstream>


using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

const mValue& Bidtracker::getPairValue(const mObject& obj, const std::string& name)
{
    mObject::const_iterator iter = obj.find(name);

    assert(iter != obj.end());
    assert(iter->first == name);

    return iter->second;
}

string removeSpaces(string input)
{
  input.erase(std::remove(input.begin(),input.end(),' '),input.end());
 
  return input;
}

string removequotes(string input)
{
  input.erase(std::remove(input.begin(),input.end(),'"'),input.end());
  
  return input;
}

std::map<std::string,int64_t> btctxlist(){
	std::map<std::string,int64_t> btctxlist;
	
	ifstream myfile ("btcunspentsorted.txt");
	char * pEnd;
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			btctxlist[strs[0]]=strtoll(strs[1].c_str(),&pEnd,10);
		}
		myfile.close();
	}	
	return btctxlist;
}

CAmount Bidtracker::btcgetbalance()
{
    std::string address = "1K4pzmPeBAVPDzd7ee9zdXReFHxmXaWZYK";
	CAmount balance; 
    std::string url;
    url = "https://blockchain.info/q/addressbalance/" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

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

void btcsortunspent(){

	ifstream myfile ("btcunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("btcunspentsorted.txt",fstream::out);
	
	std::string line;
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
				myfile2 << semp;
			}

			search2 = "value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("			value:").length(), "");					
				myfile2 << semp << std::endl;				
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

void btcgetsender()
{
		std::map<std::string,int64_t> unspentlist = btctxlist();
		std::map<std::string,int64_t>::iterator txlistit;
				
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{

		std::string txid = txlistit->first;
	 	std::string url;
		url = "https://blockchain.info/rawtx/"+ txid ;
    	const char * d = url.c_str();
    CURL *curl;
      CURLcode res;
      string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, d);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

	std::ofstream myfile;
	myfile.open("btcunspentfinal.txt",fstream::out);
	readBuffer = removeSpaces(readBuffer);
	readBuffer = removequotes(readBuffer);
	myfile << readBuffer << std::endl;
	myfile.close();
	//btcsortunspent();
	//return readBuffer;   
		}		  
}

std::string Bidtracker::btcgetunspent()
{
    std::string address = "1K4pzmPeBAVPDzd7ee9zdXReFHxmXaWZYK";
	
    std::string url;
    url = "https://blockchain.info/unspent?active=" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      
	std::ofstream myfile;
	myfile.open("btcunspentraw.txt",fstream::out);
	readBuffer = removeSpaces(readBuffer);
	readBuffer = removequotes(readBuffer);
	myfile << readBuffer << std::endl;
	myfile.close();
	btcsortunspent();
	btcgetsender();
	return readBuffer;
}

CAmount Bidtracker::fiatgetbalance()
{return 0;}

CAmount Bidtracker::dashgetbalance()
{
    std::string address = "XwvGM1XvrDGQ3MLX8aats2jG33kjGupBWX";
	CAmount balance; 
    std::string url;
    url = "http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      std::cout << readBuffer << std::endl;
      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;
      
      return balance;
}
 
std::string Bidtracker::dashgetunspent()
{
    std::string address = "XyHvWG9iAhtmk31gMymsfdhuC1DQg1Sr8K";
	 
    std::string url;
    url = "http://chainz.cryptoid.info/dash/api.dws?q=unspent&key=bd7fe7c156cd&a=" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
    
	std::ofstream myfile;
	myfile.open("dashunspentraw.txt",fstream::out); 
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
} 

CAmount Bidtracker::ltcgetbalance()
{
    std::string address = "LS18wropMM8VwT8YFiEZJE7LK8UdZVXXyE";
	CAmount balance; 
    std::string url;
    url = "http://ltc.blockr.io/api/v1/address/balance/" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

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

CAmount Bidtracker::bcrgetbalance()
{
    std::string address = "5qH4yHaaaRuX1qKCZdUHXNJdesssNQcUct";
	CAmount balance; 
    std::string url;
    url = "http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=" + address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      std::cout << readBuffer << std::endl;
      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;
      
      return balance;
}



std::string Bidtracker::ltcgetunspent()
{
    std::string address = "LS1kA3ohQLuCKJ4oxjNbJ8a5XBArACtZPj";
	 
    std::string url;
    url = "http://btc.blockr.io/api/v1/address/unspent/ "+ address +"?unconfirmed=1";
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
    
	std::ofstream myfile;
	myfile.open("ltcunspentraw.txt",fstream::out); 
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
}

std::string Bidtracker::bcrgetunspent()
{
    std::string address = "5qH4yHaaaRuX1qKCZdUHXNJdesssNQcUct";
	 
    std::string url;
    url = "http://chainz.cryptoid.info/bcr/api.dws?q=unspent&key=bd7fe7c156cd&a="+ address;
    
    const char * c = url.c_str();

      CURLcode res;
      std::string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }
    
	std::ofstream myfile;
	myfile.open("bcrunspentraw.txt",fstream::out); 
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
}

