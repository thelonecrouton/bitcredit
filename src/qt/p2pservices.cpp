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

}

/*void p2pservices::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        sqlEdit->setText(dlg.getReturnValue());
    }
}*/
