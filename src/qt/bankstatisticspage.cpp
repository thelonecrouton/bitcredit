#include "bankstatisticspage.h"
#include "bidtracker.h"
#include "ui_bankstatisticspage.h"
#include "main.h"
#include "wallet.h"
#include "init.h"
#include "base58.h"
#include "bankmath.h"
#include "clientmodel.h"
#include "rpcserver.h"
#include <sstream>
#include <string>

#include <QWidget>

using namespace json_spirit;

BankStatisticsPage::BankStatisticsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BankStatisticsPage)
{
    ui->setupUi(this);
	//this->setStyleSheet("background-image:url(:/images/background);");
    //setFixedSize(768, 512);

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(updateStatistics()));
}

double mincreditscorePrevious = -1, avecreditscorePrevious = -1, mintrustPrevious = -1, btcassetsPrevious = -1, netinterestratePrevious = -1,
     trustPrevious = -1, inflationindexPrevious = -1, consensusindexPrevious = -1, minsafereserve = -1, 
    maxreserve = -1, reserverequirement = -1;

int64_t marketcapPrevious = -1, gblmoneysupplyPrevious = -1, grantstotalPrevious = -1, gblavailablecreditPrevious = -1,
    globaldebtPrevious = -1, bankreservePrevious = -1;

QString bankstatusPrevious = "Inactive";
QString bankstatusCritical = "Critical";
QString bankstatusLow = "Low";
QString bankstatusSafe = "Safe";
QString bankstatusGood = "Healthy";
QString bankstatusGreat = "Golden";
QString networkstatus = "Out of Sync";
QString reservestatusPrevious = "Inactive";
QString phase = "";


void BankStatisticsPage::updateStatistics()
{
	Bankmath st;
	Rawdata my;
	Bidtracker r;
    double mincreditscore =  st.Getmincreditscore();
    double avecreditscore = st.Getavecreditscore();
    double mintrust = st.Getmintrust();
    double btcassets = my.getbankreserve()/100000000;
    double netinterestrate = st.Getnetinterestrate();
	double trustr = st.Gettrust();
    double trust = st.Gettrust();
    double nSubsidy = GetBlockValue((chainActive.Tip()->nHeight) ,0)/10000000;
    int nHeight = (chainActive.Tip()->nHeight);
    int64_t totalnumtx = my.totalnumtx();
    int64_t marketcap = nSubsidy * totalnumtx;
    double gblmoneysupply = my.Getgblmoneysupply();
    int64_t grantstotal = my.Getgrantstotal();
    int64_t bankreserve = my.getbankreserve();
    int64_t gblavailablecredit = st.Getgblavailablecredit();
    int64_t globaldebt = st.Getglobaldebt();
    double minsafereserve = gblmoneysupply * 0.05; 
    double maxreserve = gblmoneysupply * 0.25;
    double reserverequirement = gblmoneysupply * 0.1;
    double inflationindex = gblmoneysupply * 0.25;
    
    
    if(btcassets > 0 && btcassets< 1000)
    {
        ui->bankstatus->setText("<font color=\"red\">" + bankstatusCritical + "</font>");
    }
    else if (btcassets > 999 && btcassets< 5000)
    {
        ui->bankstatus->setText("<font color=\"orange\">" + bankstatusLow + "</font>");
    }
    else if (btcassets > 4999 && btcassets< 10000)
    {
        ui->bankstatus->setText("<font color=\"green\">" + bankstatusSafe + "</font>");
    }
    else if (btcassets > 9999 && btcassets< 15000)
    {
        ui->bankstatus->setText("<font color=\"blue\">" + bankstatusGood + "</font>");
    }
    else if (btcassets > 9999 && btcassets< 15000)
    {
        ui->bankstatus->setText("<font color=\"black\">" + bankstatusGreat + "</font>");
    }
    else
    {
    ui->bankstatus->setText(bankstatusPrevious);
    }
    QString height = QString::number(nHeight);

    QString qVolume = QLocale(QLocale::English).toString((qlonglong)totalnumtx);
    QString nmincreditscore = QString::number(mincreditscore, 'f', 6);
    QString navecreditscore = QString::number(avecreditscore, 'f', 6);
    QString nmintrust = QString::number(mintrust, 'f', 6);
    QString nbtcassets = QString::number(btcassets, 'f', 6);
    QString nnetinterestrate = QString::number(netinterestrate, 'f', 6);
    QString ntrust = QString::number(trust, 'f', 6);
    QString ninflationindex = QString::number(inflationindex, 'f', 6);
    QString nconsensusindex = QString::number(consensusindex, 'f', 6);
    QString ngblmoneysupply = QString::number(gblmoneysupply, 'f', 6);
    QString ngrantstotal = QString::number(grantstotal, 'f', 6);
    QString ngblavailablecredit = QString::number(gblavailablecredit, 'f', 6);
    QString nglobaldebt = QString::number(globaldebt, 'f', 6);  

    if(mincreditscore > mincreditscorePrevious)
    {
        ui->mincreditscore->setText("<font color=\"green\">" + nmincreditscore + "</font>");
    }
    else if (mincreditscore < mincreditscorePrevious)
    {
        ui->mincreditscore->setText("<font color=\"red\">" + nmincreditscore + "</font>");
    }
    else
    {
    ui->mincreditscore->setText(nmincreditscore);
    }

    if(avecreditscore > avecreditscorePrevious)
    {
        ui->avecreditscore->setText("<font color=\"green\">" + navecreditscore + "</font>");
    }
    else if (avecreditscore < avecreditscorePrevious)
    {
        ui->avecreditscore->setText("<font color=\"red\">" + navecreditscore + "</font>");
    }
    else
    {
    ui->avecreditscore->setText(navecreditscore);
    }

    if(mintrust > mintrustPrevious)
    {
        ui->mintrust->setText("<font color=\"green\">" + nmintrust + "</font>");
    }
    else if (mintrust < mintrustPrevious)
    {
        ui->mintrust->setText("<font color=\"red\">" + nmintrust + "</font>");
    }
    else
    {
    ui->mintrust->setText(nmintrust);
    }

    if(btcassets > btcassetsPrevious)
    {
        ui->btcassets->setText("<font color=\"green\">" + nbtcassets + "</font>");
    }
    else if (btcassets < btcassetsPrevious)
    {
        ui->btcassets->setText("<font color=\"red\">" + nbtcassets + "</font>");
    }
    else
    {
    ui->btcassets->setText(nbtcassets);
    }


    if(netinterestrate > netinterestratePrevious)
    {
        ui->netinterestrate->setText("<font color=\"green\">" + nnetinterestrate + "</font>");
    }
    else if (netinterestrate < netinterestratePrevious)
    {
        ui->netinterestrate->setText("<font color=\"red\">" + nnetinterestrate + "</font>");
    }
    else
    {
    ui->netinterestrate->setText(nnetinterestrate);
    }

    if(trust > trustPrevious)
    {
        ui->trustscoreLabel->setText("<font color=\"green\">" + ntrust + "</font>");
    }
    else if (trust < trustPrevious)
    {
        ui->trustscoreLabel->setText("<font color=\"red\">" + ntrust + "</font>");
    }
    else
    {
    ui->trustscoreLabel->setText(ntrust);
    }

    if(inflationindex > inflationindexPrevious)
    {
        ui->inflationindex->setText("<font color=\"green\">" + ninflationindex + "</font>");
    }
    else if (inflationindex < inflationindexPrevious)
    {
        ui->inflationindex->setText("<font color=\"red\">" + ninflationindex + "</font>");
    }
    else
    {
    ui->inflationindex->setText(ninflationindex);
    }  

	if(marketcap > marketcapPrevious)
    {
        ui->marketcap->setText("<font color=\"green\">" + QString::number(marketcap) + " $</font>");
    }
    else if(marketcap < marketcapPrevious)
    {
        ui->marketcap->setText("<font color=\"red\">" + QString::number(marketcap) + " $</font>");
    } 
    else 
    {
        ui->marketcap->setText(QString::number(marketcap) + " $");
    }

    if(gblmoneysupply > gblmoneysupplyPrevious)
    {
        ui->gblmoneysupply->setText("<font color=\"green\">" + ngblmoneysupply + "</font>");
    }
    else if (gblmoneysupply < gblmoneysupplyPrevious)
    {
        ui->gblmoneysupply->setText("<font color=\"red\">" + ngblmoneysupply + "</font>");
    }
    else
    {
    ui->gblmoneysupply->setText(ngblmoneysupply);
    }

    if(grantstotal > grantstotalPrevious)
    {
        ui->grantstotal->setText("<font color=\"green\">" + ngrantstotal + "</font>");
    }
    else if (grantstotal < grantstotalPrevious)
    {
        ui->grantstotal->setText("<font color=\"red\">" + ngrantstotal + "</font>");
    }
    else
    {
    ui->grantstotal->setText(ngrantstotal);
    }

    if(gblavailablecredit > gblavailablecreditPrevious)
    {
        ui->gblavailablecredit->setText("<font color=\"green\">" + ngblavailablecredit + "</font>");
    }
    else if (gblavailablecredit < gblavailablecreditPrevious)
    {
        ui->gblavailablecredit->setText("<font color=\"red\">" + ngblavailablecredit + "</font>");
    }
    else
    {
    ui->gblavailablecredit->setText(ngblavailablecredit);
    }

    if(globaldebt > globaldebtPrevious)
    {
        ui->globaldebt->setText("<font color=\"green\">" + nglobaldebt + "</font>");
    }
    else if (globaldebt < globaldebtPrevious)
    {
        ui->globaldebt->setText("<font color=\"red\">" + nglobaldebt + "</font>");
    }
    else
    {
    ui->globaldebt->setText(nglobaldebt);
    }

    updatePrevious(mincreditscore , avecreditscore, mintrust, btcassets, netinterestrate, trust, inflationindex, consensusindex, nHeight, totalnumtx , marketcap ,  gblmoneysupply , grantstotal, bankreserve, gblavailablecredit, globaldebt, bankstatus);
}

void BankStatisticsPage::updatePrevious(double mincreditscore , double  avecreditscore, double  mintrust, double  btcassets,double  netinterestrate,double  trust,double  inflationindex,double consensusindex,int  nHeight,int64_t  totalnumtx ,int64_t  marketcap ,int64_t  gblmoneysupply ,int64_t  grantstotal,int64_t  bankreserve,int64_t  gblavailablecredit,int64_t  globaldebt, QString bankstatus)
{
    mincreditscorePrevious = mincreditscore;
    avecreditscorePrevious = avecreditscore;
    mintrustPrevious = mintrust;
    btcassetsPrevious = btcassets;
    netinterestratePrevious = netinterestrate;
    marketcapPrevious = marketcap;
    trustPrevious = trust;
    inflationindexPrevious = inflationindex;
    consensusindexPrevious = consensusindex;
    gblmoneysupplyPrevious = gblmoneysupply;
    totalnumtxPrevious = totalnumtx;
    grantstotalPrevious = grantstotal;
    bankreservePrevious = bankreserve;
    gblavailablecreditPrevious = gblavailablecredit;
    globaldebtPrevious = globaldebt;
}

void BankStatisticsPage::setModel(ClientModel *model)
{
    updateStatistics();
    this->model = model;
}


BankStatisticsPage::~BankStatisticsPage()
{
    delete ui;
}
