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
std::string btcbalance;
//static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

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

void Bidtracker::getbalance()
{
    std::string address = "1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9";
	 
    std::string url;
    url = "https://blockchain.info/address/" + address + "?format=json";
    
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
      btcbalance = response;
      
}

void Bidtracker::getunspent()
{
    std::string address = "1NFPKQdfigWdfGwZmhSZKomvoUYvJWUqW9";
	 
    std::string url;
    url = "https://blockchain.info/unspent?active= "+ address;
    
    const char * c = url.c_str();

    
      CURLcode res;
      string readBuffer;

      curl = curl_easy_init();
      if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, c);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        }

      cout << readBuffer << std::endl;
      string response = readBuffer;
		
      boost::replace_all(response, "{\"unspent_outputs\"", "");
      boost::replace_all(response, ":[", "");
	  vector<string> strs;
      boost::split(strs,response,boost::is_any_of("{"));
	  std::ofstream myfile ("unspentraw.txt",fstream::out); 
	        
    for(int i = 0; i < response.size(); i++){
        mValue jsonResponse = new mValue();

        //Make sure the response is valid
        if(read_string(response, jsonResponse)) {
            mObject jsonObject = jsonResponse.get_obj();

            if (getPairValue(jsonObject, "script").get_str() == "76a914e9131832e473f3bb90f145385440e366d5c9a86488ac" ) {
                try {
                    myfile <<(getPairValue(jsonObject, "tx_hash_big_endian").get_str());
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
	 
	if (myfile.is_open()){
		while ( myfile.good() ){
			myfile << readBuffer << std::endl;
		}
		myfile.close();
	}
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
		int i=1;
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
	        
    for(int i = 0; i < response.size(); i++){
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
    
			printf("bid:%s,%llu",unspentit->first.c_str(),unspentit->second);
			CBitcreditAddress address(unspentit->first);
			//printf("bidcheckaddr:%x,%s",address,unspentit->first.c_str());
			total = total+unspentit->second;
			i++;
		}
		  
}
