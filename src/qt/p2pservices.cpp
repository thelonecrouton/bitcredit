#include "p2pservices.h"
#include "ui_p2pservices.h"
#include "trust.h"
#include "util.h"
#include "addressbookpage.h"
#include "bidtracker.h"
#include "utilmoneystr.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "primitives/block.h"

#include <QtSql>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>


p2pservices::p2pservices(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::p2pservices)
{
    ui->setupUi(this);
}

p2pservices::~p2pservices()
{
    delete ui;
}

void p2pservices::gettrust()
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
/*	QString b = ui->query->toPlainText();
	std::string chainID = b.toStdString();

	sql="select * from RAWDATA where ADDRESS = ?";

	rc = sqlite3_prepare(rawdb,sql, strlen(sql), &stmt,  0 );
    sqlite3_bind_text(stmt, 1,chainID.data(), chainID.size(), 0);*/

	QString defaultdb = (GetDataDir() /"ratings/rawdata.db").string().c_str();
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","Default");
	db.setDatabaseName(defaultdb);
 
    QSqlQuery q(db);
    q.prepare("SELECT * from RAWDATA WHERE ADDRESS = ?");
    q.addBindValue(ui->sqlEdit->toPlainText());
    unsigned long int balance, outgoingtx, firstuse, incomingtx, totalinputs, totaloutputs, credit, votes;
	double avedailyincome;
	double trust=0;
    if(q.exec())
      {
        balance = q.value(1).toInt();
        firstuse = q.value(2).toInt();
        incomingtx = q.value(3).toInt();
        outgoingtx = q.value(4).toInt();
        totalinputs = q.value(5).toInt();
        totaloutputs = q.value(6).toInt();

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
        avedailyincome = totalinputs / lifetime;
        double avedailyexpenditure = totaloutputs / lifetime;

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
					QSqlQuery query(db);
					query.prepare("SELECT * from BLOCKS WHERE ADDRESS = ?");
					query.addBindValue(ui->sqlEdit->toPlainText());	
					
                    if(!query.exec())
					{
					QMessageBox::warning(this, tr("Unable to open database"), tr("Miner points error ") + db.lastError().text());
					}						

					while (query.next()) {
						count++;
					}
					
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

    QString ntrust = QString::number(trust, 'f', 6);
    QString navedailyincome = QString::number(avedailyincome, 'f', 6);
    QString nbalance = QString::number(balance, 'f', 8);
    QString ncredit = QString::number(credit, 'f', 8);
    QString nvotes = QString::number(votes, 'f', 8);
    QString addr =ui->sqlEdit->toPlainText();
	ui->trustrating->setText(ntrust);
    ui->income->setText(navedailyincome);
    ui->balancer->setText(nbalance);
    ui->address->setText(addr);
    ui->creditrating->setText(ncredit);
    ui->votess->setText(nvotes);
}

void p2pservices::setModel(WalletModel *model)
{
    this->model = model;
}

void p2pservices::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->sqlEdit->setText(dlg.getReturnValue());
    }
}

void p2pservices::on_pasteButton_clicked()
{
    // Paste text from clipboard into field
    ui->sqlEdit->setText(QApplication::clipboard()->text());
}
