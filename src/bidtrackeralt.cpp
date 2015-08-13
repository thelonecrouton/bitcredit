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

void btcsortunspent(){

	ifstream myfile ("btcunspentraw.txt");
	std::ofstream myfile2;
	myfile2.open("btcunspentsorted.txt",fstream::out);
	
	std::string line, txid, value, url, readBuffer;
	if (myfile.is_open()){
		while ( myfile.good() ){
			getline (myfile,line);
			string temp = line;					
			std::string search;
			std::string search2;
			std::string search3;
			size_t pos;
			size_t f = line.find("			tx_hash_big_endian:");
			size_t h = line.find("            addr:");
			size_t g = line.find("			value:");
	
			search = "tx_hash_big_endian";
			pos = temp.find(search);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(f, std::string("			tx_hash_big_endian:").length(), "");
				myfile2 << semp;
				semp = remove(semp, ',');
				txid = semp;
				cout << "txid - "<< txid <<endl;
				url = "https://blockchain.info/rawtx/"+ txid ;
				const char * d = url.c_str();
				CURL *curl;
				CURLcode res;

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
				cout << readBuffer <<endl;
				readBuffer = remove(readBuffer, '"');
				readBuffer = remove(readBuffer, '{');
				readBuffer = remove(readBuffer,'}');
				readBuffer = remove(readBuffer, '[');
				myfile2 << readBuffer << std::endl;

				search3 = "addr:";
				pos = readBuffer.find(search3);
				if (pos != std::string::npos){
				std::string semp =readBuffer;
				semp = semp.replace(h, std::string("            addr:").length(), "");
				semp = remove(semp, ',');					
				myfile2 << semp << std::endl;
				//value = semp;				
			}



			}

			search2 = "value:";
			pos = temp.find(search2);
			if (pos != std::string::npos){
				std::string semp =line;
				semp = semp.replace(g, std::string("			value:").length(), "");
				semp = remove(semp, ',');					
				myfile2 <<"actual value...ignore for now"<< semp << std::endl;
				value = semp;				
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

	ifstream myfile3 ("btcsendersraw.txt");
	std::ofstream myfile2;
	myfile2.open("btcsenderssorted.txt",fstream::out);
	
	std::string line;
	if (myfile3.is_open()){
		while ( myfile3.good() ){
			getline (myfile3,line);
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
		myfile3.close();
		myfile2.close();
	}

	ifstream myfile4 ("btcsenderssorted.txt");
	std::ofstream myfile5;	
	myfile5.open("btcbids.txt",fstream::out);

	std::string line2;
	if (myfile4.is_open()){
		for( txlistit = unspentlist.begin(); txlistit != unspentlist.end(); ++txlistit)
		{
		getline (myfile4,line2);
		int64_t value = txlistit->second;
		myfile5 << line << value << std::endl; 			
		}
	
		}
		myfile4.close();
		myfile5.close();

}
