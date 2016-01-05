#ifndef ASSETSPAGE_H
#define ASSETSPAGE_H

#include <QWidget>
#include <QStringListModel>
#include <QFile>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTableWidget>

#include "walletmodel.h"

class WalletModel;

namespace Ui {
class AssetsPage;
}

class AssetsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AssetsPage(QWidget *parent = 0);
    ~AssetsPage();
    //bool colorQuery(QString cmd);
    bool runColorCore();
    bool sendRequest(QString cmd, QString &result);
	void listunspent();
    void getBalance();
    void setModel(WalletModel *model);
    
private slots:
    void readPyOut();
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void on_issueButton_clicked();

public slots:
    void update();

private:
    Ui::AssetsPage *ui;
    WalletModel *model;
    QProcess *serverProc;
};

#endif // MAINWINDOW_H
