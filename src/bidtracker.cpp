#include "bidtracker.h"
#include "util.h"
#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>
#include <stdio.h>
#include <map>
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

std::map<std::string,int64_t> dashtxlist(){
	std::map<std::string,int64_t> dashtxlist;
	
	ifstream myfile ("dashunspentsorted.txt");
	char * pEnd;
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			dashtxlist[strs[0]]=strtoll(strs[1].c_str(),&pEnd,10);
		}
		myfile.close();
	}	
	return dashtxlist;
}

std::map<std::string,int64_t> ltctxlist(){
	std::map<std::string,int64_t> ltctxlist;
	
	ifstream myfile ("ltcunspentsorted.txt");
	char * pEnd;
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			std::vector<std::string> strs;
			boost::split(strs, line, boost::is_any_of(","));
			ltctxlist[strs[0]]=strtoll(strs[1].c_str(),&pEnd,10);
		}
		myfile.close();
	}	
	return ltctxlist;
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
				semp = remove(semp, ',');					
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
	std::ofstream myfile;
	myfile.open("btcsendersraw.txt",fstream::out);				
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


	std::size_t pos = readBuffer.find("script");
	readBuffer = readBuffer.substr(0,pos);
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer,'}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;

  
	}
		myfile.close();		  
}

void btcsortsenders(){

	ifstream myfile ("btcsendersraw.txt");
	std::ofstream myfile2;
	myfile2.open("btcsenderssorted.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			size_t pos;
			size_t f = line.find("            addr:");
	
			search = "addr:";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("            addr:").length(), "");
				myfile2 << semp << std::endl;
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

void btcbids(){

	ifstream myfile ("btcsenderssorted.txt");
	std::ofstream myfile2;	
	myfile2.open("btcbids.txt",fstream::out);

	std::map<std::string,int64_t> unspentlist = btctxlist();
	std::map<std::string,int64_t>::iterator txlistit;
	
	std::string line;
	if (myfile.is_open()){
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{
		getline (myfile,line);
		int64_t value = txlistit->second;
		myfile2 << line << value << std::endl; 			
		}
	
		}
		myfile.close();
		myfile2.close();
}	

void dashbids(){

	ifstream myfile ("dashsenderssorted.txt");
	std::ofstream myfile2;	
	myfile2.open("dashbids.txt",fstream::out);

	std::map<std::string,int64_t> unspentlist = dashtxlist();
	std::map<std::string,int64_t>::iterator txlistit;
	
	std::string line;
	if (myfile.is_open()){
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{
		getline (myfile,line);
		int64_t value = txlistit->second;
		myfile2 << line << value << std::endl; 			
		}
	
		}
		myfile.close();
		myfile2.close();
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
	readBuffer = remove(readBuffer, ' ');
	readBuffer = remove(readBuffer, '"');
	myfile << readBuffer << std::endl;
	myfile.close();
	btcsortunspent();
	btcgetsender();
	btcsortsenders();
	btcbids();
	return readBuffer;
}

double Bidtracker::btcgetprice()
{
	double price; 
    std::string url;
    url = "https://blockchain.info/q/24hrprice";
    
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
     
      if ( ! (istringstream(readBuffer) >> price) ) price = 0;
      
      return price;
}

double Bidtracker::dashgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-DASH";
    
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

    
      if ( ! (istringstream(readBuffer) >> price) ) price = 0;
    cout << price << std::endl;
      return price;
}

double Bidtracker::bcrgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-BCR";
    
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

     
      if ( ! (istringstream(readBuffer) >> price) ) price = 0;
    cout << price << std::endl;
      return price;
}

double Bidtracker::ltcgetprice()
{
	double price; 
    std::string url;
    url = "https://bittrex.com/api/v1.1/public/getticker?market=BTC-LTC";
    
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

      if ( ! (istringstream(readBuffer) >> price) ) price = 0;
    cout << price << std::endl;
      return price;
}

CAmount Bidtracker::fiatgetbalance()
{return 0;}

void dashsortsenders(){

	ifstream myfile ("dashsendersraw.txt");
	std::ofstream myfile2;
	myfile2.open("dashsenderssorted.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			size_t pos;
			size_t f = line.find("address:");
	
			search = "address:";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = remove(semp, ']');
				semp = remove(semp, ',');
				semp = semp.replace(f, std::string("address:").length(), "");
				myfile2 << semp << "," << std::endl;
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

void dashsortunspent(){

	ifstream myfile ("dashunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("dashunspentsorted.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("id:");
			size_t g = line.find("tx_address_value:");
	
			search = "id:";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("id:").length(), "");
				myfile2 << semp;
			}

			search2 = "tx_address_value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("tx_address_value:").length(), "");
				semp = remove(semp, ',');					
				myfile2 << semp << std::endl;				
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

void dashgetsender()
{
		std::map<std::string,int64_t> unspentlist = dashtxlist();
		std::map<std::string,int64_t>::iterator txlistit;
	std::ofstream myfile;
	myfile.open("dashsendersraw.txt",fstream::out);				
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{

		std::string txid = txlistit->first;
	 	std::string url;
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
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer,'}');
	readBuffer = remove(readBuffer, '[');
	readBuffer = replacestring(readBuffer, ",address:", ",\n\address:");
	myfile << readBuffer << std::endl;
  
	}
		myfile.close();		  
}

CAmount Bidtracker::dashgetbalance()
{
    std::string address = "XtrmiNg6bgSjbeGexYcvTBwfUjgQgxGoiW";
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

      std::string response = readBuffer;
      if ( ! (istringstream(response) >> balance) ) balance = 0;
      
      return balance;
}
 
std::string Bidtracker::dashgetunspent()
{
    std::string address = "XtrmiNg6bgSjbeGexYcvTBwfUjgQgxGoiW";	 
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
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();
	dashsortunspent();
	dashgetsender();
	dashsortsenders();
	dashbids();
	return readBuffer;
} 

void ltcsortsenders(){

	ifstream myfile ("ltcsendersraw.txt");
	std::ofstream myfile2;
	myfile2.open("ltcsenderssorted.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			size_t pos;
			size_t f = line.find("address:");
	
			search = "address:";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = remove(semp, ']');
				semp = remove(semp, ',');
				semp = semp.replace(f, std::string("address:").length(), "");
				myfile2 << semp << "," << std::endl;
			}
		}
		myfile.close();
		myfile2.close();
	}	
}

void ltcgetsender()
{
		std::map<std::string,int64_t> unspentlist = ltctxlist();
		std::map<std::string,int64_t>::iterator txlistit;
	std::ofstream myfile;
	myfile.open("ltcsendersraw.txt",fstream::out);				
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{

		std::string txid = txlistit->first;
	 	std::string url;
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
	readBuffer = replacestring(readBuffer, ",", ",\n");
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer,'}');
	readBuffer = remove(readBuffer, '[');
	readBuffer = replacestring(readBuffer, ",address:", ",\n\address:");
	myfile << readBuffer << std::endl;
  
	}
		myfile.close();		  
}



void ltcsortunspent(){

	ifstream myfile ("ltcunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("ltcunspentsorted.txt",fstream::out);
	
	std::string line;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			size_t pos;
			size_t f = line.find("id:");
			size_t g = line.find("tx_address_value:");
	
			search = "id:";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("id:").length(), "");
				myfile2 << semp;
			}

			search2 = "tx_address_value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("tx_address_value:").length(), "");
				semp = remove(semp, ',');					
				myfile2 << semp << std::endl;				
			}
		}
		myfile.close();
		myfile2.close();
	}	
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
    url = "http://chainz.cryptoid.info/bcr/api.dws?q=getbalance&a=" + address;
    
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

void ltcbids(){

	ifstream myfile ("ltcsenderssorted.txt");
	std::ofstream myfile2;	
	myfile2.open("ltcbids.txt",fstream::out);

	std::map<std::string,int64_t> unspentlist = ltctxlist();
	std::map<std::string,int64_t>::iterator txlistit;
	
	std::string line;
	if (myfile.is_open()){
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{
		getline (myfile,line);
		int64_t value = txlistit->second;
		myfile2 << line << value << std::endl; 			
		}
	
		}
		myfile.close();
		myfile2.close();
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
	readBuffer = remove(readBuffer, '"');
	readBuffer = remove(readBuffer, '{');
	readBuffer = remove(readBuffer, '}');
	readBuffer = remove(readBuffer, '[');
	myfile << readBuffer << std::endl;
	myfile.close();
	ltcsortunspent();
	ltcgetsender();
	ltcsortsenders();
	ltcbids();
	return readBuffer;
}
