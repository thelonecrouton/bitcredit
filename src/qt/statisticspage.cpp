#include "statisticspage.h"
#include "bidtracker.h"
#include "basenodeman.h"
#include "ui_statisticspage.h"
#include "main.h"
#include "wallet.h"
#include "init.h"
#include "base58.h"
#include "clientmodel.h"
#include "rpcserver.h"
#include <sstream>
#include <string>

#include <QWidget>

using namespace json_spirit;

StatisticsPage::StatisticsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatisticsPage)
{
    ui->setupUi(this);

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(updateStatistics()));
}

double bidsPrevious= -1, marketcapPrevious = -1, mincreditscorePrevious = -1, assetstotalPrevious = -1, avecreditscorePrevious = -1, mintrustPrevious = -1, btcassetsPrevious = -1, bcrassetsPrevious = -1, fiatassetsPrevious = -1, netinterestratePrevious = -1,
     trustPrevious = -1, inflationindexPrevious = -1, liquidityindexPrevious = -1, globaldebtPrevious = -1;

int64_t  gblmoneysupplyPrevious = -1,  gblavailablecreditPrevious = -1, bankreservePrevious = -1;

QString bankstatusPrevious = "Inactive" , bankstatusCritical = "Critical", bankstatusLow = "Low", bankstatusSafe = "Safe", bankstatusGood = "Healthy", bankstatusGreat = "Golden", networkstatus = "Out of Sync";
QString phase = "";

void StatisticsPage::updateStatistics()
{
	Bidtracker r;
    CCoinsStats stats;
    double mincreditscore =  0.0;
    double avecreditscore = 0.0;
    double mintrust = 0.0;
    double btcassets = r.getbalance("https://blockchain.info/q/addressbalance/16bi8R4FoDHfjNJ1RhpvcAEn4Cz78FbtZB")/COIN;
    double bcrassets = r.getbalance("http://chainz.cryptoid.info/bcr/api.dws?q=getbalance&a=5qH4yHaaaRuX1qKCZdUHXNJdesssNQcUct");
    double bcrprice = r.bcrbtc();
    double btcprice = r.usdbtc();
    double netinterestrate = 0.0;
    double assetstotal = btcassets + bcrassets*bcrprice;
    double fiatassets = assetstotal*btcprice;
    int nHeight = (chainActive.Tip()->nHeight);
    int64_t totalnumtx = 0;
    double gblmoneysupply = stats.nTotalAmount/COIN ;
    double marketcap =  assetstotal - ((bcrprice*0) *btcprice);
    double gblavailablecredit = (gblmoneysupply - bcrassets )- mnodeman.size()*50000;
    double grossmarketcap =  (gblmoneysupply * bcrprice) * btcprice;
    double inflationindex = (45000/gblmoneysupply) *100;
    double liquidityindex = ((gblmoneysupply * bcrprice)*btcprice)/ assetstotal;
	double globaldebt =  grossmarketcap - marketcap;

	
    if(btcassets > 0 && btcassets< 100)
    {
        ui->bankstatus->setText("<font color=\"red\">" + bankstatusCritical + "</font>");
    }
    else if (btcassets > 99 && btcassets< 1000)
    {
        ui->bankstatus->setText("<font color=\"orange\">" + bankstatusLow + "</font>");
    }
    else if (btcassets > 999 && btcassets< 10000)
    {
        ui->bankstatus->setText("<font color=\"green\">" + bankstatusSafe + "</font>");
    }
    else if (btcassets > 9999 && btcassets< 20000)
    {
        ui->bankstatus->setText("<font color=\"blue\">" + bankstatusGood + "</font>");
    }
    else if (btcassets > 19999)
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
    QString nbtcassets = QString::number(btcassets/COIN, 'f', 8);
    QString nbcrassets = QString::number(bcrassets, 'f', 8);
    QString nfiatassets = QString::number(fiatassets, 'f', 8);
    QString nassetstotal = QString::number(assetstotal, 'f', 6);
    QString nnetinterestrate = QString::number(netinterestrate, 'f', 6);
    QString ntrust = QString::number(trust, 'f', 6);
    QString ninflationindex = QString::number(inflationindex, 'f', 6);
    QString nliquidityindex = QString::number(liquidityindex, 'f', 6);
    QString ngblmoneysupply = QString::number(gblmoneysupply, 'f', 6);
    QString ngblavailablecredit = QString::number(gblavailablecredit, 'f', 6);
    QString nglobaldebt = QString::number(globaldebt, 'f', 6);  

    if(liquidityindex > liquidityindexPrevious)
    {
        ui->liquidityindex->setText("<font color=\"green\">" + nliquidityindex + "</font>");
    }
    else if (liquidityindex < liquidityindexPrevious)
    {
        ui->liquidityindex->setText("<font color=\"red\">" + nliquidityindex + "</font>");
    }
    else
    {
    ui->liquidityindex->setText(nliquidityindex);
    }

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

	if(marketcap > marketcapPrevious)
    {
        ui->marketcap->setText("<font color=\"green\">$" + QString::number(marketcap) + " </font>");
    }
    else if(marketcap < marketcapPrevious)
    {
        ui->marketcap->setText("<font color=\"red\">$" + QString::number(marketcap) + " </font>");
    } 
    else 
    {
        ui->marketcap->setText(" $" +QString::number(marketcap) );
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

    if(assetstotal > assetstotalPrevious)
    {
        ui->assetstotal->setText("<font color=\"green\">$" + nassetstotal + "</font>");
    }
    else if (assetstotal < assetstotalPrevious)
    {
        ui->assetstotal->setText("<font color=\"red\">$" + nassetstotal + "</font>");
    }
    else
    {
    ui->assetstotal->setText("$"+nassetstotal);
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
    ui->gblavailablecredit->setText(""+ ngblavailablecredit);
    }

    if(globaldebt > globaldebtPrevious)
    {
        ui->globaldebt->setText("<font color=\"green\">$" + nglobaldebt + "</font>");
    }
    else if (globaldebt < globaldebtPrevious)
    {
        ui->globaldebt->setText("<font color=\"red\">$" + nglobaldebt + "</font>");
    }
    else
    {
    ui->globaldebt->setText("$" + nglobaldebt);
    }

    updatePrevious(mincreditscore , avecreditscore, mintrust, btcassets, bcrassets, fiatassets, netinterestrate, trust, inflationindex, consensusindex, nHeight, totalnumtx , marketcap ,  gblmoneysupply , assetstotal, gblavailablecredit, globaldebt, bankstatus, bidsPrevious);
}

void StatisticsPage::updatePrevious(double mincreditscore , double  avecreditscore, double  mintrust, double  btcassets, double  bcrassets, double  fiatassets,double  netinterestrate,double  trust,double  inflationindex,double liquidityindex,int  nHeight,int64_t  totalnumtx ,double marketcap ,int64_t  gblmoneysupply ,double  assetstotal,int64_t  gblavailablecredit,double  globaldebt, QString bankstatus, double bids)
{
    mincreditscorePrevious = mincreditscore;
    avecreditscorePrevious = avecreditscore;
    mintrustPrevious = mintrust;
    btcassetsPrevious = btcassets;
    bcrassetsPrevious = bcrassets;
    fiatassetsPrevious = fiatassets;
    netinterestratePrevious = netinterestrate;
    marketcapPrevious = marketcap;
    trustPrevious = trust;
    inflationindexPrevious = inflationindex;
    liquidityindexPrevious = liquidityindex;
    gblmoneysupplyPrevious = gblmoneysupply;
    totalnumtxPrevious = totalnumtx;
    assetstotalPrevious = assetstotal;
    gblavailablecreditPrevious = gblavailablecredit;
    globaldebtPrevious = globaldebt;
    bidsPrevious= bids;
}

void StatisticsPage::setModel(ClientModel *model)
{
    updateStatistics();
    this->model = model;
}


StatisticsPage::~StatisticsPage()
{
    delete ui;
}
