#include "assetspage.h"
#include "ui_assetspage.h"
#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QClipboard>
#include <QStringList>
#include "util.h"
#include "addresstablemodel.h"
#include "addressbookpage.h"
#include "guiutil.h"
#include "optionsmodel.h"

AssetsPage::AssetsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AssetsPage),
    model(0)
{
    ui->setupUi(this);
    serverProc = new QProcess(this);

    if (!runColorCore()) return;
    getBalance();
    // normal bitcredit address field
    GUIUtil::setupAddressWidget(ui->chainID, this);
            
    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(update()));
    connect(ui->transferButton, SIGNAL(pressed()), this, SLOT(sendassets()));
    connect(ui->issueButton, SIGNAL(pressed()), this, SLOT(issueassets()));
    connect(ui->tableWidget, SIGNAL(cellDoubleClicked (int, int) ), this, SLOT(cellSelected( int, int )));
}

void AssetsPage::update()
{
    ui->tableWidget->clearContents();
	getBalance();
}

void AssetsPage::on_pasteButton_clicked()
{
    ui->chainID->setText(QApplication::clipboard()->text());
}

void AssetsPage::setModel(WalletModel *model)
{
    this->model = model;
}

void AssetsPage::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->chainID->setText(dlg.getReturnValue());
        ui->amount->setFocus();
    }
}

void AssetsPage::clear()
{
    // clear UI elements 
    ui->chainID->clear();
    ui->amount->clear();
    ui->metadata->clear();
}

void AssetsPage::listunspent()
{
    QString response;
    if (!sendRequest("listunspent", response)) {
        //TODO print error
        return;
    }

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &err);

    QJsonArray jsonArray = jsonDoc.array();
    for (int i = 0; i < jsonArray.size(); i++) {
		ui->tableWidget->insertRow(0);
        QVariant oaAddrElement = jsonArray.at(i).toVariant();
        QVariantMap oaAddrMap = oaAddrElement.toMap();
        QString oaAddress = oaAddrMap["oa_address"].toString();
        QString address = oaAddrMap["address"].toString();
        QString assetID = oaAddrMap["asset_id"].toString();
        double amount = oaAddrMap["amount"].toDouble();
        unsigned long int quantity = oaAddrMap["asset_quantity"].toULongLong();
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(address));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(oaAddress));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(assetID));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(quantity)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(amount)));
    }
}

void AssetsPage::getBalance()
{
    QString response;
    if (!sendRequest("getbalance", response)) {
        //TODO print error
        return;
    }

	QString assetID;
	QComboBox *assetids;
	QComboBox *qtys;
    unsigned long int quantity=0;

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &err);

    QJsonArray jsonArray = jsonDoc.array();
    for (int i = 0; i < jsonArray.size(); i++) {
		ui->tableWidget->setRowCount(jsonArray.size());
        QVariant oaAddrElement = jsonArray.at(i).toVariant();
        QVariantMap oaAddrMap = oaAddrElement.toMap();
        QString oaAddress = oaAddrMap["oa_address"].toString();
        QString address = oaAddrMap["address"].toString();
        double balance = oaAddrMap["value"].toDouble();
		QVariantList data = oaAddrMap["assets"].toList();
		assetids = new QComboBox;
		qtys = new QComboBox;
		assetids->setEditable(true);
		qtys->setEditable(true);
		foreach(QVariant v, data) {
		assetids->addItem(v.toMap().value("asset_id").toString());
		qtys->addItem(v.toMap().value("quantity").toString());	
		//assetID = v.toMap().value("asset_id").toString();
		//quantity = v.toMap().value("quantity").toULongLong();
		}		
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(address));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(oaAddress));
		ui->tableWidget->setCellWidget(i, 2, assetids );
		ui->tableWidget->setCellWidget(i, 3, qtys );
        //ui->tableWidget->setItem(i, 2, new QTableWidgetItem(assetID));
        //ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(quantity)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(balance)));
    }
}

void AssetsPage::sendassets()
{
	QProcess p;
	QString sendCmd,data;
	data=ui->chainID->text()+ " "+ ui->asset->text() + " " +ui->amount->text() +" " + ui->sendTo->text(); 
	QMessageBox msgBox;
	msgBox.setWindowTitle("Transfer Asset");
	msgBox.setText("Please confirm you wish to send " + ui->amount->text() + "units of " + ui->asset->text() + "to "+ ui->sendTo->text());
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	if(msgBox.exec() == QMessageBox::Yes){
#ifdef WIN32
		p.setWorkingDirectory(QCoreApplication::applicationDirPath());
		sendCmd ="cmd.exe /c python.exe assets/colorcore.py sendasset" + data;
#else
		sendCmd ="python3.4 "+ QCoreApplication::applicationDirPath() +"/assets/colorcore.py sendasset " + data;
#endif
		p.start(sendCmd);
		if (!p.waitForStarted()){
			LogPrintf("Error: Could not send! \n");		
		}
		p.waitForFinished();	
	}	
		ui->chainID->clear();
		ui->asset->clear();
		ui->amount->clear();
		ui->sendTo->clear();	
}

void AssetsPage::issueassets()
{
	QProcess d,m;
	QString sendCmd, data;
	data =ui->chainID->text()+" "+ ui->amount->text();	
#ifdef WIN32
	d.setWorkingDirectory(QCoreApplication::applicationDirPath());
	sendCmd = "cmd.exe /c python.exe assets/colorcore.py issueasset "+data ;
#else
	sendCmd = "python3.4 "+ QCoreApplication::applicationDirPath() +"/assets/colorcore.py issueasset " + data;
#endif
	d.start(sendCmd);

	if (!d.waitForStarted()){
		LogPrintf("Error: Could not issue! \n");
		
	}
	d.waitForFinished();

	if (ui->distribute->isChecked()){
		data =ui->chainID->text()+" "+ ui->sendTo->text()+" "+ ui->price->text();
#ifdef WIN32
	m.setWorkingDirectory(QCoreApplication::applicationDirPath());
	sendCmd ="cmd.exe /c python.exe assets/colorcore.py distribute "+ data;
#else
	sendCmd ="python3.4 "+ QCoreApplication::applicationDirPath() +"/assets/colorcore.py distribute " + data;
#endif
	m.start(sendCmd);

	if (!m.waitForStarted()){
		LogPrintf("Error: Could not distribute! \n");		
	}
	m.waitForFinished();
	}

	ui->chainID->clear();
	ui->amount->clear();
	ui->sendTo->clear();
	ui->price->clear();
}

bool AssetsPage::runColorCore()
{
    QString startCmd;
    QObject::connect(serverProc, SIGNAL(readyRead()), this, SLOT(readPyOut()));
#ifdef WIN32
	serverProc->setWorkingDirectory(QCoreApplication::applicationDirPath());
	startCmd = "cmd.exe /c python.exe assets/colorcore.py server";
#else
	startCmd = "python3.4 " + QCoreApplication::applicationDirPath() + "/assets/colorcore.py server";
#endif
	serverProc->start(startCmd);
    if (!serverProc->waitForStarted()){
        LogPrintf("Error: Could not start! \n");
        return false;
    }
    return true;
}

void AssetsPage::readPyOut() {
    QByteArray pyOut  = serverProc->readAllStandardOutput();
    if (pyOut.length() < 1) {
        qDebug() << serverProc->errorString();
        return;
    }
    qDebug() << pyOut;
}

bool AssetsPage::sendRequest(QString cmd, QString& result)
{
    QNetworkAccessManager mgr;
    QEventLoop eventLoop;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    QNetworkRequest req(QUrl( QString("http://localhost:8080/"+cmd)));
    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        result = (QString)reply->readAll();
        delete reply;
        return true;
    }
    result = (QString)reply->errorString();
    delete reply;
    return false;
}

void AssetsPage::cellSelected(int nRow, int nCol)
{
	QAbstractItemModel* tmodel = ui->tableWidget->model();
	QModelIndex index = tmodel->index(nRow, nCol);

	if(nCol==0) ui->chainID->setText(ui->tableWidget->model()->data(index).toString());
	if(nCol==1) ui->sendTo->setText(ui->tableWidget->model()->data(index).toString());
	if(nCol==2) {
		QComboBox *myCB = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(nRow,nCol));
		ui->asset->setText(myCB->currentText());		
	}
	if(nCol==3){
		QComboBox *myCB = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(nRow,nCol));
		ui->amount->setText(myCB->currentText());		
	} 
	
}

AssetsPage::~AssetsPage()
{
    serverProc->terminate();
    if (serverProc->NotRunning) delete serverProc;
    delete ui;
}
