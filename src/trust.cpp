#include "trust.h"

#include "bidtracker.h"
#include "voting.h"
#include "util.h"
#include "utilmoneystr.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "primitives/block.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void TrustEngine::createdb()
{
   sqlite3 *rawdb;
   char *zErrMsg = 0;
   int rc;

	vector<const char*> sql;

   rc = sqlite3_open((GetDataDir() /"ratings/rawdata.db").string().c_str(), &rawdb);
   if( rc ){
      if(fDebug)LogPrintf("Can't open database: %s\n", sqlite3_errmsg(rawdb));
      exit(0);
   }else{
      if(fDebug)LogPrintf("Opened database successfully\n");
   }

   /* Create SQL statements */
   sql.push_back("CREATE TABLE RAWDATA("  \
         "ADDRESS TEXT PRIMARY KEY      NOT NULL," \
         "BALANCE           INTEGER     DEFAULT 0," \
         "FIRSTSEEN         INTEGER     DEFAULT 0," \
         "TXINCOUNT         INTEGER     DEFAULT 0," \
         "TXOUTCOUNT        INTEGER     DEFAULT 0," \
         "TOTALIN           INTEGER     DEFAULT 0," \
         "TOTALOUT          INTEGER     DEFAULT 0);");

  sql.push_back("CREATE TABLE PEERS("  \
         "ADDRESS TEXT PRIMARY KEY      NOT NULL," \
         "CREDIT            INTEGER     DEFAULT 0," \
         "DEBIT             INTEGER     DEFAULT 0," \
         "TXCOUNT       	INTEGER     DEFAULT 0," \
         "TRUST             INTEGER     DEFAULT 0," \
         "CREDITSCORE       INTEGER     DEFAULT 0," \
         "RATING       		INTEGER     DEFAULT 0," \
         "TXDATE       		INTEGER     DEFAULT 0," \
         "DUEDATE      		INTEGER     DEFAULT 0);");

  sql.push_back("CREATE TABLE BLOCKS(" \
            "    ID INTEGER PRIMARY KEY AUTOINCREMENT," \
            "    HASH TEXT," \
            "    TIME INTEGER," \
            "    MINER TEXT);");


   /* Execute SQL statements */
	for (unsigned int i =0;i < sql.size();i++){
		rc = sqlite3_exec(rawdb, sql[i], callback, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			if (fDebug)LogPrintf("SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else{
			if (fDebug)LogPrintf( "Tables created successfully\n");
		}
	}

	if(sqlite3_close(rawdb) != SQLITE_OK ){
		if (fDebug)LogPrintf("SQL unable to close database %s\n", sqlite3_errmsg(rawdb));
		sqlite3_free(zErrMsg);
	}else{
		if (fDebug)LogPrintf( "database closed successfully\n");
	}
}

string TrustEngine::gettrust(std::string chainID)
{

	sqlite3 *rawdb;
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	int rc;
	const char *sql;

    rc = sqlite3_open((GetDataDir() / "ratings/rawdata.db").string().c_str(), &rawdb);
    if( rc ){
     if (fDebug) LogPrintf("Can't open database: %s\n", sqlite3_errmsg(rawdb));
    }else{
      if (fDebug) LogPrintf("Opened database successfully\n");
    }
	
	sql="select * from RAWDATA where ADDRESS = ?";

	rc = sqlite3_prepare(rawdb,sql, strlen(sql), &stmt,  0 );
    sqlite3_bind_text(stmt, 1,chainID.data(), chainID.size(), 0);
    int64_t balance, outgoingtx, firstuse, incomingtx, totalinputs, totaloutputs;
	
	if (sqlite3_step(stmt) == SQLITE_ROW){

		balance = (sqlite3_column_int(stmt, 1))/COIN;
		firstuse = sqlite3_column_int(stmt, 2);
		incomingtx = sqlite3_column_int(stmt, 3);
		outgoingtx = sqlite3_column_int(stmt, 4);
		totalinputs = sqlite3_column_int(stmt, 5);
		totaloutputs = sqlite3_column_int(stmt, 6);

		int64_t totaltx=  incomingtx+outgoingtx;
		double globallife = (GetTime()- 1418504572)/24*3600;
		double lifetime = (GetTime() - firstuse)/24*3600;
		int64_t totalnettx = chainActive.Tip()->nChainTx;
		double txfreq = totaltx/lifetime;
		double nettxfreq = totalnettx / globallife;
		double spendfactor = balance/totaloutputs;
		double savefactor;

				if (totalinputs !=0)
					savefactor =balance/totalinputs;
				else
					savefactor = 0;

        double nettxpart = totaltx/ totalnettx;
        double aveinput = totalinputs/ incomingtx;
        double aveoutput = totaloutputs / outgoingtx;
        double avedailyincome = totalinputs / lifetime;
        double avedailyexpenditure = totaloutputs / lifetime;

		double trust=0;
			{
				{
					if (lifetime > 360 )
							trust+= 20;
						else if (lifetime > 30 && lifetime < 360 )
							trust+= lifetime*0.055;
						else
							trust+= 0;
						}

					{
					if (totaltx > 10000){
						trust+= 10;
					}
					else if (totaltx>0 && totaltx< 10000){
						trust+= totaltx*0.001;
					}
					else
						trust+= 0;
				}

				{
					if(balance > 1000000){
						trust+= 25;
					}
					else if(balance > 0 && balance <= 1000000){
						trust+= balance/50000;
					}
					else
						trust+= 0;
				}

				{
					if (txfreq > 5)
						trust+=15;
					else if (txfreq> 0.01 && txfreq< 5)
						trust+= txfreq *3;
					else
						trust+= 0;
				}

				{
					if (savefactor > 0.1)
						trust+=20;
					else if (savefactor> 0.001 && savefactor< 0.1)
						trust+= savefactor *200;
					else
						trust+= 0;
				}

				{
					if (avedailyincome > 100)
						trust+=20;
					else if (avedailyincome > 1 && avedailyincome< 100)
						trust+= avedailyincome/5;
					else
						trust+= 0;
				}

				{
					int count = 0;
					char* minerquery = sqlite3_mprintf("select count(*) from BLOCKS where MINER = '%q'",chainID.c_str());
					rc = sqlite3_exec(rawdb, minerquery, callback, &count, &zErrMsg);
					double points = count/chainActive.Tip()->nHeight;

					if (points > 0.01 )
						trust+= 20;
					else if (points > 0.0001 && points < 0.01)
						trust+= points*190;
					else
						trust+= 0;
				}
			}
		}
}
