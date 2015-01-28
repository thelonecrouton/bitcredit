#include "bankstatisticspage.h"
#include "ui_bankstatisticspage.h"
#include "main.h"
#include "wallet.h"
#include "init.h"
#include "base58.h"
#include "stats.h"
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
	this->setStyleSheet("background-image:url(:/images/background);");
    //setFixedSize(768, 512);

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(updateStatistics()));
}

double mincreditscorePrevious = -1, avecreditscorePrevious = -1, mintrustPrevious = -1, avetrustPrevious = -1, netinterestratePrevious = -1,
     grantindexPrevious = -1, inflationindexPrevious = -1, consensusindexPrevious = -1, minsafereserve = -1, 
    maxreserve = -1, reserverequirement = -1;

int64_t marketcapPrevious = -1, gblmoneysupplyPrevious = -1, grantstotalPrevious = -1, gblavailablecreditPrevious = -1,
    globaldebtPrevious = -1, bankreservePrevious = -1;

QString bankstatusPrevious = "Inactive";
QString networkstatus = "Out of Sync";
QString reservestatusPrevious = "Inactive";
QString phase = "";

void BankStatisticsPage::updateStatistics()
{
	Stats st;
    double mincreditscore =  st.Getmincreditscore();
    double avecreditscore = st.Getavecreditscore();
<<<<<<< HEAD
<<<<<<< HEAD
    double glbcreditscore = st.Getglbcreditscore();
    double bestcreditscore = st.Getbestcreditscore();
    double mintrust = st.Getmintrust();
    double avetrust = st.Getavetrust();
    double gbltrust = st.Getgbltrust();
    double besttrust = st.Getbesttrust();
    double grossinterestrate = st.Getgrossinterestrate();
    double netinterestrate = st.Getnetinterestrate();
    double gblinterestrate = st.Getgblinterestrate();
=======
    
    
=======
>>>>>>> origin/master2
    double mintrust = st.Getmintrust();
    double avetrust = st.Getavetrust();
    double netinterestrate = st.Getnetinterestrate();
<<<<<<< HEAD

>>>>>>> origin/master2
=======
	double trustr = st.Gettrust();
>>>>>>> origin/master2
    double grantindex = st.Getgrantindex();
    double nSubsidy = GetBlockValue((chainActive.Tip()->nHeight) ,0)/10000000;
    int nHeight = (chainActive.Tip()->nHeight);
    int64_t volume = st.Getvolume();
    int64_t marketcap = st.aveprice() * volume;
    double gblmoneysupply = st.Getgblmoneysupply();
    int64_t grantstotal = st.Getgrantstotal();
    int64_t bankreserve = st.Getbankreserve();
    int64_t gblavailablecredit = st.Getgblavailablecredit();
    int64_t globaldebt = st.Getglobaldebt();
    double minsafereserve = gblmoneysupply * 0.05; 
    double maxreserve = gblmoneysupply * 0.25;
    double reserverequirement = gblmoneysupply * 0.1;
    double inflationindex = st.Getinflationindex();
<<<<<<< HEAD
    double consensusindex = st.Getconsensusindex();
=======
    
>>>>>>> origin/master2
    ui->bankstatus->setText(bankstatusPrevious);
    QString height = QString::number(nHeight);

    QString qVolume = QLocale(QLocale::English).toString((qlonglong)volume);
    QString nmincreditscore = QString::number(mincreditscore, 'f', 6);
    QString navecreditscore = QString::number(avecreditscore, 'f', 6);
    QString nmintrust = QString::number(mintrust, 'f', 6);
    QString navetrust = QString::number(avetrust, 'f', 6);
    QString nnetinterestrate = QString::number(netinterestrate, 'f', 6);
    QString ngrantindex = QString::number(grantindex, 'f', 6);
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

<<<<<<< HEAD
    if(glbcreditscore > glbcreditscorePrevious)
    {
        ui->glbcreditscore->setText("<font color=\"green\">" + nglbcreditscore + "</font>");
    }
    else if (glbcreditscore < glbcreditscorePrevious)
    {
        ui->glbcreditscore->setText("<font color=\"red\">" + nglbcreditscore + "</font>");
    }
    else
    {
    ui->glbcreditscore->setText(nglbcreditscore);
    }

    if(bestcreditscore > bestcreditscorePrevious)
    {
        ui->bestcreditscore->setText("<font color=\"green\">" + nbestcreditscore + "</font>");
    }
    else if (bestcreditscore < bestcreditscorePrevious)
    {
        ui->bestcreditscore->setText("<font color=\"red\">" + nbestcreditscore + "</font>");
    }
    else
    {
    ui->bestcreditscore->setText(nbestcreditscore);
    }

=======
>>>>>>> origin/master2
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

    if(avetrust > avetrustPrevious)
    {
        ui->avetrust->setText("<font color=\"green\">" + navetrust + "</font>");
    }
    else if (avetrust < avetrustPrevious)
    {
        ui->avetrust->setText("<font color=\"red\">" + navetrust + "</font>");
    }
    else
    {
    ui->avetrust->setText(navetrust);
    }

<<<<<<< HEAD
    if(gbltrust > gbltrustPrevious)
    {
        ui->gbltrust->setText("<font color=\"green\">" + ngbltrust + "</font>");
    }
    else if (gbltrust < gbltrustPrevious)
    {
        ui->gbltrust->setText("<font color=\"red\">" + ngbltrust + "</font>");
    }
    else
    {
    ui->gbltrust->setText(ngbltrust);
    }

    if(besttrust > besttrustPrevious)
    {
        ui->besttrust->setText("<font color=\"green\">" + nbesttrust + "</font>");
    }
    else if (besttrust < besttrustPrevious)
    {
        ui->besttrust->setText("<font color=\"red\">" + nbesttrust + "</font>");
    }
    else
    {
    ui->besttrust->setText(nbesttrust);
    }
=======
>>>>>>> origin/master2

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

    if(grantindex > grantindexPrevious)
    {
        ui->grantindex->setText("<font color=\"green\">" + ngrantindex + "</font>");
    }
    else if (grantindex < grantindexPrevious)
    {
        ui->grantindex->setText("<font color=\"red\">" + ngrantindex + "</font>");
    }
    else
    {
    ui->grantindex->setText(ngrantindex);
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
<<<<<<< HEAD
    }

<<<<<<< HEAD
    if(consensusindex > consensusindexPrevious)
    {
        ui->consensusindex->setText("<font color=\"green\">" + nconsensusindex + "</font>");
    }
    else if (consensusindex < consensusindexPrevious)
    {
        ui->consensusindex->setText("<font color=\"red\">" + nconsensusindex + "</font>");
    }
    else
    {
    ui->consensusindex->setText(nconsensusindex);
    }
=======
>>>>>>> origin/master2
    
    QString ngblmoneysupply = QString::number(gblmoneysupply, 'f', 6);
    QString ngrantstotal = QString::number(grantstotal, 'f', 6);
    QString ngblavailablecredit = QString::number(gblavailablecredit, 'f', 6);
    QString nglobaldebt = QString::number(globaldebt, 'f', 6);    
=======
    }  
>>>>>>> origin/master2

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

    updatePrevious(mincreditscore , avecreditscore, mintrust, avetrust, netinterestrate, grantindex, inflationindex, consensusindex, nHeight, volume , marketcap ,  gblmoneysupply , grantstotal, bankreserve, gblavailablecredit, globaldebt, bankstatus);
}

void BankStatisticsPage::updatePrevious(double mincreditscore , double  avecreditscore, double  mintrust, double  avetrust,double  netinterestrate,double  grantindex,double  inflationindex,double consensusindex,int  nHeight,int64_t  volume ,int64_t  marketcap ,int64_t  gblmoneysupply ,int64_t  grantstotal,int64_t  bankreserve,int64_t  gblavailablecredit,int64_t  globaldebt, QString bankstatus)
{
    mincreditscorePrevious = mincreditscore;
    avecreditscorePrevious = avecreditscore;
    mintrustPrevious = mintrust;
    avetrustPrevious = avetrust;
    netinterestratePrevious = netinterestrate;
    marketcapPrevious = marketcap;
    grantindexPrevious = grantindex;
    inflationindexPrevious = inflationindex;
    consensusindexPrevious = consensusindex;
    gblmoneysupplyPrevious = gblmoneysupply;
    volumePrevious = volume;
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
