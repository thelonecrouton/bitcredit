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

CAmount Bidtracker::btcgetbalance()
{
    std::string address = "1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9";
	CAmount balance; 
    std::string url;
    url = "https://blockchain.info/q/addressbalance/" + address;
    
    const char * c = url.c_str();

    CURL *curl;
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

      //std::cout << readBuffer << std::endl;
      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;
      
      return balance;
}

std::map<std::string,int64_t> sortbtcunspent(){
	
	std::map<std::string,int64_t> sortunspent;
	
	ifstream myfile ("btcunspentraw.txt");
	
	char * pEnd;
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			std::vector<std::string> strs;
			boost::replace_all(line, "{\"unspent_outputs\"", "");
			boost::replace_all(line, ":[", "");
			boost::split(strs,line,boost::is_any_of("{"));			
			boost::split(strs, line, boost::is_any_of(","));
			sortunspent[strs[0]]=strtoll(strs[1].c_str(),&pEnd,10);
		}
		myfile.close();
	}	
	return sortunspent;
}

std::string Bidtracker::btcgetunspent()
{
    std::string address = "1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9";
	CAmount balance; 
    std::string url;
    url = "https://blockchain.info/unspent?active=" + address;
    
    const char * c = url.c_str();

    CURL *curl;
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
	myfile << readBuffer << std::endl;
	
	std::string line;
	getline (myfile,line);
	boost::replace_all(line, "{\"unspent_outputs\"", "");
	boost::replace_all(line, ":[", "");
				
	myfile.close();

	return readBuffer;
}

CAmount Bidtracker::dashgetbalance()
{
    std::string address = "XwvGM1XvrDGQ3MLX8aats2jG33kjGupBWX";
	CAmount balance; 
    std::string url;
    url = "http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=" + address;
    
    const char * c = url.c_str();

    CURL *curl;
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

CAmount Bidtracker::ltcgetbalance()
{
    std::string address = "LS18wropMM8VwT8YFiEZJE7LK8UdZVXXyE";
	CAmount balance; 
    std::string url;
    url = "http://ltc.blockr.io/api/v1/address/balance/" + address;
    
    const char * c = url.c_str();

    CURL *curl;
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
    std::string address = "LS18wropMM8VwT8YFiEZJE7LK8UdZVXXyE";
	CAmount balance; 
    std::string url;
    url = "http://chainz.cryptoid.info/dash/api.dws?q=getbalance&a=" + address;
    
    const char * c = url.c_str();

    CURL *curl;
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

    CURL *curl;
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

std::string Bidtracker::ltcgetunspent()
{
    std::string address = "LS1kA3ohQLuCKJ4oxjNbJ8a5XBArACtZPj";
	 
    std::string url;
    url = "http://btc.blockr.io/api/v1/address/unspent/ "+ address +"?unconfirmed=1";
    
    const char * c = url.c_str();

    CURL *curl;
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
    std::string address = "1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9";
	 
    std::string url;
    url = "http://chainz.cryptoid.info/bcr/api.dws?q=unspent&key=bd7fe7c156cd&a="+ address;
    
    const char * c = url.c_str();

    CURL *curl;
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


std::map<std::string,int64_t> getsortunspent(){
	
	std::map<std::string,int64_t> sortunspent;
	
	ifstream myfile ("unspentraw.txt");
	
	char * pEnd;
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			std::vector<std::string> strs;			
			boost::split(strs, line, boost::is_any_of(","));
			sortunspent[strs[0]]=strtoll(strs[1].c_str(),&pEnd,10);
		}
		myfile.close();
	}	
	return sortunspent;
}


void Bidtracker::getsender()
{
		std::map<std::string,int64_t> unspentlist = getsortunspent();
		std::map<std::string,int64_t>::iterator unspentit;
		
		int64_t total=0;
		
		for( unspentit = unspentlist.begin(); unspentit != unspentlist.end(); ++unspentit)
		{

		std::string txid = unspentit->first;
	 
		std::string url;
		url = "https://blockchain.info/tx/"+ txid +"?format=json";
    
		const char * d = url.c_str();

    
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

      cout << readBuffer << std::endl;
      string response = readBuffer;

	  vector<string> strs;
      boost::split(strs,response,boost::is_any_of("{"));
	  std::ofstream myfile ("unspentfinal.txt",fstream::out); 
	        
    for(int i = 0; i < (unsigned) response.size(); i++){
        mValue jsonResponse = new mValue();

        //Make sure the response is valid
        if(read_string(response, jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            if (getPairValue(jsonObject, "addr").get_str() =="1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9"){
                try {
					
                    myfile <<(getPairValue(jsonObject, "addr").get_str());
                    //myfile <<(getPairValue(jsonObject, "tx_index").get_real());
                    //myfile <<(getPairValue(jsonObject, "tx_output_n").get_real());
                    //myfile <<(getPairValue(jsonObject, "script").get_real());
                    myfile <<(getPairValue(jsonObject, "value").get_real());
                   // myfile <<(getPairValue(jsonObject, "value_hex").get_hex());
                   // myfile <<(getPairValue(jsonObject, "confirmations").get_real());
                    myfile << "\n";
                }
                catch (std::exception) {} //API did not return all needed data so skip processing 

                break;
            }
        }
    }
    
		}
		  
}
