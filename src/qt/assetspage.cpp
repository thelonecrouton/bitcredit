#include "mainwindow.h"
#include "ui_assetspage.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>


Assets::Assets(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Assets)
{
    ui->setupUi(this);
    serverProc = new QProcess(this);

    if (!runColorCore()) return;
    getBalance();
}

void Assets::getBalance()
{


    QString response;
    if (!sendRequest("getbalance", response)) {
        //TODO print error
        return;
    }

    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &err);

    QJsonArray jsonArray = jsonDoc.array();
    for (int i = 0; i < jsonArray.size(); i++) {
        QVariant oaAddrElement = jsonArray.at(i).toVariant();
        QVariantMap oaAddrMap = oaAddrElement.toMap();
        QString oaAddress = oaAddrMap["oa_address"].toString();
        QLabel* const labelAddr = new QLabel(QString("OA Address"));
        //labelAddr->setFixedSize(200, 50);
        ui->verticalLayout->addWidget(labelAddr);
        QLabel* oaAddrValue = new QLabel();
        //oaAddrValue->setFixedSize(200, 50);
        oaAddrValue->setText(oaAddress);
        ui->verticalLayout->addWidget(oaAddrValue);
    }
}

bool Assets::runColorCore()
{
    QString startCmd = QCoreApplication::applicationDirPath() + "/colorcore/colorcore.py server";
    QObject::connect(serverProc, SIGNAL(readyRead()), this, SLOT(readPyOut()));
    serverProc->start("python3 " + startCmd);
    if (!serverProc->waitForStarted()){
        qDebug() << "Error: Could not start!" << endl;
        return false;
    }
    return true;
}

void Assets::readPyOut() {
    QByteArray pyOut  = serverProc->readAllStandardOutput();
    if (pyOut.length() < 1) {
        qDebug() << serverProc->errorString();
        return;
    }
    qDebug() << pyOut;
}

bool Assets::sendRequest(QString cmd, QString& result)
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

Assets::~Assets()
{
    serverProc->terminate();
    if (serverProc->NotRunning) delete serverProc;
    delete ui;
}
