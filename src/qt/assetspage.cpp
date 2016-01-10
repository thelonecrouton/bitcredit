#include "assetspage.h"
#include "ui_assetspage.h"
#include <QApplication>
#include <QDebug>
#include <QJsonArray>
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
    connect(ui->tableWidget, SIGNAL(cellDoubleClicked (int, int) ), this, SLOT(cellSelected( int, int )));
}

void AssetsPage::update()
{
    ui->tableWidget->clearContents();
	getBalance();
}

void AssetsPage::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->chainID->setText(QApplication::clipboard()->text());
}

void AssetsPage::on_pasteButton2_clicked()
{
    ui->sendTo->setText(QApplication::clipboard()->text());
}

void AssetsPage::setModel(WalletModel *model)
{
    this->model = model;

    //clear();
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

void AssetsPage::on_addressBookButton2_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->sendTo->setText(dlg.getReturnValue());
        ui->transferButton->setFocus();
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
    unsigned long int quantity=0;

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &err);

    QJsonArray jsonArray = jsonDoc.array();
    for (int i = 0; i < jsonArray.size(); i++) {
		ui->tableWidget->insertRow(0);
        QVariant oaAddrElement = jsonArray.at(i).toVariant();
        QVariantMap oaAddrMap = oaAddrElement.toMap();
        QString oaAddress = oaAddrMap["oa_address"].toString();
        QString address = oaAddrMap["address"].toString();
        double balance = oaAddrMap["value"].toDouble();
		QVariantList data = oaAddrMap["assets"].toList();
		foreach(QVariant v, data) {
		assetID = v.toMap().value("asset_id").toString();
		quantity = v.toMap().value("quantity").toULongLong();
		}		
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(address));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(oaAddress));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(assetID));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(quantity)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(balance)));
    }
}

void AssetsPage::on_issueButton_clicked()
{
    QString response;
	QString request= "issueasset " + ui->chainID->text()+" "+ ui->amount->text();

    if (!sendRequest(request, response)) {
        QMessageBox::information(this, tr("Issue Asset"), tr("Server response: ") + response);
        return;
    }
}

void AssetsPage::on_transferButton_clicked()
{
    QString response, request;
	//QStringList strListItems= QStringList() <<"sendasset"<<ui->chainID->text()<<ui->asset->text()<<ui->amount->text()<< ui->sendTo->text();
	//request = strListItems.join(" ");
    request = " sendasset "+ ui->chainID->text()+ " "+ ui->asset->text() + " " +ui->amount->text() +" " + ui->sendTo->text();

	QMessageBox msgBox;
	msgBox.setWindowTitle("title");
	msgBox.setText("Question");
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
	if(msgBox.exec() == QMessageBox::Yes){
	// do something
	}else {
	// do something else
	}   
    
    if (!sendRequest(request, response)) {
        QMessageBox::information(this, tr("Transfer Asset"), tr("Server response: ") + response);
        return;
    }
}

bool AssetsPage::runColorCore()
{
    QString startCmd = QCoreApplication::applicationDirPath() + "/assets/colorcore.py server";
    QObject::connect(serverProc, SIGNAL(readyRead()), this, SLOT(readPyOut()));
    serverProc->start("python3.4 " + startCmd);
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
	std::cout << cmd.toStdString() << std::endl;
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
	if(nCol==2) ui->asset->setText(ui->tableWidget->model()->data(index).toString());
	if(nCol==3) ui->amount->setText(ui->tableWidget->model()->data(index).toString());
	
}

AssetsPage::~AssetsPage()
{
    serverProc->terminate();
    if (serverProc->NotRunning) delete serverProc;
    delete ui;
}
