#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QFile>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace Ui {
class Assets;
}

class Assets : public QWidget
{
    Q_OBJECT

public:
    explicit Assets(QWidget *parent = 0);
    ~Assets();
    //bool colorQuery(QString cmd);
    bool runColorCore();
    bool sendRequest(QString cmd, QString &result);

    void getBalance();
private slots:
    void readPyOut();
private:
    QStringListModel *model;
    Ui::MainWindow *ui;

    QProcess *serverProc;
};

#endif // MAINWINDOW_H
