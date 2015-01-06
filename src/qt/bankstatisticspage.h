#ifndef BANKSTATISTICSPAGE_H
#define BANKSTATISTICSPAGE_H

#include "clientmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"

#include <QWidget>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include <QSlider>

class Stats;

namespace Ui {
class BankStatisticsPage;
}
class ClientModel;

class BankStatisticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit BankStatisticsPage(QWidget *parent = 0);
    ~BankStatisticsPage();
    
    void setModel(ClientModel *model);
    
    double mincreditscore;
    double avecreditscore;
    double glbcreditscore;
    double bestcreditscore, mintrust, avetrust, gbltrust, besttrust, grossinterestrate, netinterestrate, 
    gblinterestrate, grantindex, expectedchange, inflationindex, consensusindex;
    int64_t  grantsaverage, gblmoneysupply, grantstotal, bankreserve, gblavailablecredit,
    globaldebt;
    int volumePrevious;
    QString bankstatus;
    int64_t marketcap;
    
public slots:

    void updateStatistics();
    void updatePrevious(double,double,double,double,double,double,double,double,double,double,double,double,double,double,int,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,int64_t,QString );
					   

private slots:

private:
    Ui::BankStatisticsPage *ui;
    ClientModel *model;
    
};

#endif // BANKSTATISTICSPAGE_H
