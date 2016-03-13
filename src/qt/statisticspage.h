#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

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
class StatisticsPage;
}
class ClientModel;

class StatisticsPage : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsPage(QWidget *parent = 0);
    ~StatisticsPage();
    
    void setModel(ClientModel *model);
    double trustr;
    double mincreditscore;
    double avecreditscore;
    double bestcreditscore, mintrust, btcassets, gbltrust, besttrust, netinterestrate, 
     trust, inflationindex, consensusindex, globaldebt;
    int64_t  grantsaverage, gblmoneysupply, grantstotal, gblavailablecredit;
    int totalnumtxPrevious;
    QString bankstatus;
    int64_t marketcap;
    std::string theme;
    
public slots:

    void updateStatistics();
    void updatePrevious(double,double,double,double,double,double,double,double,double,double,int,int64_t,double,int64_t,double,int64_t,double,QString,double );

private slots:

private:
    Ui::StatisticsPage *ui;
    ClientModel *model;
    
};

#endif // STATISTICSPAGE_H
