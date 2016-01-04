#include "assetspage.h"
#include "ui_assetspage.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include "util.h"

AssetsPage::AssetsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AssetsPage)
{
    ui->setupUi(this);
    serverProc = new QProcess(this);

    if (!runColorCore()) return;
    getBalance();
    
    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(update()));
}

void AssetsPage::update()
{
    ui->tableWidget->clear();
	getBalance();
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
		QJsonArray assetArray = oaAddrMap["assets"].toJsonArray();
		for (int j = 0; j < assetArray.size(); j++) {		
		
		QVariant assetElement = assetArray.at(j).toVariant();
		QVariantMap assetMap = assetElement.toMap();
		assetID = assetMap["asset_id"].toString();
        quantity = assetMap["quantity"].toULongLong();
		}
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(address));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(oaAddress));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(assetID));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(quantity)));
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(balance)));
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

AssetsPage::~AssetsPage()
{
    serverProc->terminate();
    if (serverProc->NotRunning) delete serverProc;
    delete ui;
}
