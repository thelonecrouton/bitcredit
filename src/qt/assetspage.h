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
private slots:
    void readPyOut();
public slots:
    void update();

private:
    QStringListModel *model;
    Ui::AssetsPage *ui;

    QProcess *serverProc;
};

#endif // MAINWINDOW_H
