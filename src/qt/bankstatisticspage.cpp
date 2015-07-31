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

double mincreditscorePrevious = -1, avecreditscorePrevious = -1, mintrustPrevious = -1, btcassetsPrevious = -1, ltcassetsPrevious = -1, bcrassetsPrevious = -1, fiatassetsPrevious = -1, netinterestratePrevious = -1,
     trustPrevious = -1, inflationindexPrevious = -1, consensusindexPrevious = -1, minsafereserve = -1, 
    maxreserve = -1, reserverequirement = -1;

int64_t marketcapPrevious = -1, gblmoneysupplyPrevious = -1, grantstotalPrevious = -1, gblavailablecreditPrevious = -1,
    globaldebtPrevious = -1, bankreservePrevious = -1;

QString bankstatusPrevious = "Inactive" , bankstatusCritical = "Critical", bankstatusLow = "Low", bankstatusSafe = "Safe", bankstatusGood = "Healthy", bankstatusGreat = "Golden", networkstatus = "Out of Sync";
QString phase = "";

void BankStatisticsPage::updateStatistics()
{
	Bankmath st;
	Rawdata my;
	Bidtracker r;
    double mincreditscore =  st.Getmincreditscore();
    double avecreditscore = st.Getavecreditscore();
    double mintrust = st.Getmintrust();
    double btcassets = r.btcgetbalance();
    double ltcassets = r.ltcgetbalance();
    double bcrassets = r.bcrgetbalance();
    double fiatassets = r.fiatgetbalance();
    double netinterestrate = st.Getnetinterestrate();
	double trustr = st.Gettrust();
    double trust = st.Gettrust();
    double nSubsidy = GetBlockValue((chainActive.Tip()->nHeight) ,0)/10000000;
    int nHeight = (chainActive.Tip()->nHeight);
    int64_t totalnumtx = my.totalnumtx();
    double gblmoneysupply = my.Getgblmoneysupply();
    int64_t marketcap = btcassets/ gblmoneysupply;
    int64_t btcstash = btcassets/ 100000000;
    int64_t grantstotal = my.Getgrantstotal();
    int64_t bankreserve = btcassets;
    int64_t gblavailablecredit = st.Getgblavailablecredit();
    int64_t globaldebt = st.Getglobaldebt();
    double minsafereserve = gblmoneysupply * 0.05; 
    double maxreserve = gblmoneysupply * 0.25;
    double reserverequirement = gblmoneysupply * 0.1;
    double inflationindex = gblmoneysupply * 0.25;
    string huha = r.btcgetunspent();
    
    if(btcstash > 0 && btcstash< 1000)
    {
        ui->bankstatus->setText("<font color=\"red\">" + bankstatusCritical + "</font>");
    }
    else if (btcstash > 999 && btcstash< 10000)
    {
        ui->bankstatus->setText("<font color=\"orange\">" + bankstatusLow + "</font>");
    }
    else if (btcstash > 9999 && btcstash< 100000)
    {
        ui->bankstatus->setText("<font color=\"green\">" + bankstatusSafe + "</font>");
    }
    else if (btcstash > 99999 && btcstash< 200000)
    {
        ui->bankstatus->setText("<font color=\"blue\">" + bankstatusGood + "</font>");
    }
    else if (btcstash > 199999)
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
    QString nbtcassets = QString::number(btcassets/100000000, 'f', 8);
    QString nltcassets = QString::number(ltcassets/100000000, 'f', 8);
    QString nbcrassets = QString::number(bcrassets/100000000, 'f', 8);
    QString nfiatassets = QString::number(fiatassets, 'f', 8);
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

    if(ltcassets > ltcassetsPrevious)
    {
        ui->ltcassets->setText("<font color=\"green\">" + nltcassets + "</font>");
    }
    else if (ltcassets < ltcassetsPrevious)
    {
        ui->ltcassets->setText("<font color=\"red\">" + nltcassets + "</font>");
    }
    else
    {
    ui->ltcassets->setText(nltcassets);
    }

    if(bcrassets > bcrassetsPrevious)
    {
        ui->bcrassets->setText("<font color=\"green\">" + nbcrassets + "</font>");
    }
    else if (bcrassets < bcrassetsPrevious)
    {
        ui->bcrassets->setText("<font color=\"red\">" + nbcrassets + "</font>");
    }
    else
    {
    ui->bcrassets->setText(nbcrassets);
    }

    if(fiatassets > fiatassetsPrevious)
    {
        ui->fiatassets->setText("<font color=\"green\">" + nfiatassets + "</font>");
    }
    else if (fiatassets < fiatassetsPrevious)
    {
        ui->fiatassets->setText("<font color=\"red\">" + nfiatassets + "</font>");
    }
    else
    {
    ui->fiatassets->setText(nfiatassets);
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
        ui->assetstotal->setText("<font color=\"green\">" + ngrantstotal + "</font>");
    }
    else if (grantstotal < grantstotalPrevious)
    {
        ui->assetstotal->setText("<font color=\"red\">" + ngrantstotal + "</font>");
    }
    else
    {
    ui->assetstotal->setText(ngrantstotal);
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

    updatePrevious(mincreditscore , avecreditscore, mintrust, btcassets, ltcassets, bcrassets, fiatassets, netinterestrate, trust, inflationindex, consensusindex, nHeight, totalnumtx , marketcap ,  gblmoneysupply , grantstotal, bankreserve, gblavailablecredit, globaldebt, bankstatus);
}

void BankStatisticsPage::updatePrevious(double mincreditscore , double  avecreditscore, double  mintrust, double  btcassets, double  ltcassets, double  bcrassets, double  fiatassets,double  netinterestrate,double  trust,double  inflationindex,double consensusindex,int  nHeight,int64_t  totalnumtx ,int64_t  marketcap ,int64_t  gblmoneysupply ,int64_t  grantstotal,int64_t  bankreserve,int64_t  gblavailablecredit,int64_t  globaldebt, QString bankstatus)
{
    mincreditscorePrevious = mincreditscore;
    avecreditscorePrevious = avecreditscore;
    mintrustPrevious = mintrust;
    btcassetsPrevious = btcassets;
    ltcassetsPrevious = ltcassets;
    bcrassetsPrevious = bcrassets;
    fiatassetsPrevious = fiatassets;
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
