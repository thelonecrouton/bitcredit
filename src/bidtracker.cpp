#include "bidtracker.h"
#include "banknodeman.h"
#include "wallet.h"
#include "base58.h"
#include "rpcclient.h"
#include "util.h"
#include "json_spirit_reader.h"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
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

double Bidtracker::getbalance(std::string url, double balance)
{
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

void btcsortunspent(){

	ifstream myfile ("btcunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("btcbids.txt",fstream::out);
	
	std::string line, txid, url;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			std::string search3;
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
				amount = amount/COIN;	
				myfile2 << amount << std::endl;				
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

std::string Bidtracker::btcgetunspent()
{
    std::string address = "1LdRcdxfbSnmCYYNdeYpUnztiYzVfBEQeC";
	
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
	readBuffer = remove(readBuffer, ' ');
	readBuffer = remove(readBuffer, '"');
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
}

double btcgetprice()
{
	CAmount price; 
    std::string url;
    url = "https://blockchain.info/q/24hrprice";
    
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
     
      price = atof(readBuffer.c_str());  
      
      return price;
}

double dashgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-DASH";
    
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
   // price = atof(readBuffer.c_str());  
   if ( ! (istringstream(readBuffer) >> price) ) price = 0;

    return price;
}

double bcrgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-BCR";
    
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
    //price = atof(readBuffer.c_str());  

      return price;
}

double ltcgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-LTC";
    
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
    //price = atof(readBuffer.c_str());   
	if ( ! (istringstream(readBuffer) >> price) ) price = 0;

    return price;
}

void dashsortunspent(){

	ifstream myfile ("dashunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("dashbids.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("\"id\":");
			size_t g = line.find("\"tx_address_value\":");
	
			search = "\"id\":";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("\"id\":").length(), "");
				semp = remove(semp, '"');
				semp = remove(semp, ',');
				
				string txid = semp;
				string url;
				url = "http://api.blockstrap.com/v0/drk/transaction/id/"+ txid + "?showtxn=1&showtxnio=1&";
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
			std::size_t pos = readBuffer.find(",{");
			readBuffer = readBuffer.substr(0,pos);
			string hemp = readBuffer;
			std::size_t pos1 = hemp.find("\"address\"");
			hemp = hemp.substr(pos1);			

			string lemp = hemp;
			std::size_t pos2 = lemp.find("\"}");
			lemp = lemp.substr(0,pos2);			
			lemp = replacestring(lemp, "\"address\":\"", "");
			myfile2 << lemp << ",";
			}

			search2 = "\"tx_address_value\":";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("\"tx_address_value\":").length(), "");
				semp = remove(semp, ',');
				long double amount = atof(semp.c_str());
				amount = amount/COIN;	
				amount = amount * dashgetprice();
				myfile2 << amount << std::endl;							
			}
		}
		myfile.close();
		myfile2.close();
	}	
}


std::string Bidtracker::dashgetunspent()
{
    std::string address = "Xj4XjWytC6Etax46JDgwJLPxLSA3XdaQ1C";	 
    std::string url;
    url = "http://api.blockstrap.com/v0/drk/address/unspents/" + address;
    
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
	std::size_t pos = readBuffer.find("[{");
    readBuffer = readBuffer.substr (pos); 
	readBuffer = replacestring(readBuffer, "},{", "\n},{"); 
	readBuffer = replacestring(readBuffer, ",", ",\n");  
	readBuffer = remove(readBuffer, ' ');	
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
} 

std::string Bidtracker::ltcgetunspent()
{
    std::string address = "LgY8ahfHRhvjVQC1zJnBhFMG5pCTMuKRqh";	 
    std::string url;
    url = "http://api.blockstrap.com/v0/ltc/address/unspents/" + address;
    
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
	std::size_t pos = readBuffer.find("[{");
    readBuffer = readBuffer.substr (pos); 
	readBuffer = replacestring(readBuffer, "},{", "\n},{"); 
	readBuffer = replacestring(readBuffer, ",", ",\n");  
	readBuffer = remove(readBuffer, ' ');	
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();

	return readBuffer;
} 

void ltcsortunspent(){

	ifstream myfile ("ltcunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("ltcbids.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("\"id\":");
			size_t g = line.find("\"tx_address_value\":");
	
			search = "\"id\":";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("\"id\":").length(), "");
				semp = remove(semp, '"');
				semp = remove(semp, ',');
				
				string txid = semp;
				string url;
				url = "http://api.blockstrap.com/v0/ltc/transaction/id/"+ txid + "?showtxn=1&showtxnio=1&";
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
			std::size_t pos = readBuffer.find(",{");
			readBuffer = readBuffer.substr(0,pos);
			string hemp = readBuffer;
			std::size_t pos1 = hemp.find("\"address\"");
			hemp = hemp.substr(pos1);			

			string lemp = hemp;
			std::size_t pos2 = lemp.find("\"}");
			lemp = lemp.substr(0,pos2);
			
			lemp = replacestring(lemp, "\"address\":\"", "");
			myfile2 << lemp << ",";

			}

			search2 = "\"tx_address_value\":";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("\"tx_address_value\":").length(), "");
				semp = remove(semp, ',');					
				long double amount = atof(semp.c_str());
				amount = amount/COIN;
				amount = amount *ltcgetprice();	
				myfile2 << amount << std::endl;					
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

double Bidtracker::usdbtc(){
	
return btcgetprice();	

} 

long double Bidtracker::ltcbtc(){
	
return ltcgetprice();

}

long double Bidtracker::dashbtc(){
	
return dashgetprice();

}
 
long double Bidtracker::bcrbtc(){
	
return bcrgetprice();	

} 

void Bidtracker::combine()
{
		
	std::ofstream myfile;
	myfile.open("final.txt",fstream::out);
	ifstream myfile2("btcbids.txt");
	ifstream myfile3("ltcbids.txt");
	ifstream myfile4("dashbids.txt");
	
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
	if (myfile4.is_open()){
		std::string line;
		while ( myfile4.good() ){
			getline (myfile4,line);	
	myfile<<line;
	}	}

	myfile.close();	
	myfile2.close();	
	myfile3.close();	
	myfile4.close();
	remove("btcbids.txt");
	remove("ltcbids.txt");
	remove("dashbids.txt");
	remove("btcunspentraw.txt");
	remove("ltcunspentraw.txt");
	remove("dashunspentraw.txt");			  
}

std::string Bidtracker::getbids(int nHeight){

	ltcgetunspent();
	btcgetunspent();
	dashgetunspent();
	ltcsortunspent();
	btcsortunspent();
	dashsortunspent();
	combine();
	
	return "Bids Updated";
}	



