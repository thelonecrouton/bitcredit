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

    setFixedSize(768, 512);

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(updateStatistics()));
}

double mincreditscorePrevious = -1, avecreditscorePrevious = -1, glbcreditscorePrevious = -1, bestcreditscorePrevious = -1,
    mintrustPrevious = -1, avetrustPrevious = -1, gbltrustPrevious = -1, besttrustPrevious = -1, grossinterestratePrevious = -1, netinterestratePrevious = -1,
    gblinterestratePrevious = -1, grantindexPrevious = -1, expectedchangePrevious = -1, inflationindexPrevious = -1, consensusindexPrevious = -1, minsafereserve = -1, 
    maxreserve = -1, reserverequirement = -1, grantsaveragePrevious = -1;

int64_t marketcapPrevious = -1, gblmoneysupplyPrevious = -1, grantstotalPrevious = -1, bankreservePrevious = -1, gblavailablecreditPrevious = -1,
    globaldebtPrevious = -1;

QString bankstatusPrevious = "Inactive";
QString networkstatus = "Out of Sync";
QString reservestatusPrevious = "Inactive";
QString phase = "";

void BankStatisticsPage::updateStatistics()
{
	Stats st;
    double mincreditscore =  st.Getmincreditscore();
    double avecreditscore = st.Getavecreditscore();
    double glbcreditscore = st.Getglbcreditscore();
    double bestcreditscore = st.Getbestcreditscore();
    double mintrust = st.Getmintrust();
    double avetrust = st.Getavetrust();
    double gbltrust = st.Getgbltrust();
    double besttrust = st.Getbesttrust();
    double grossinterestrate = st.Getgrossinterestrate();
    double netinterestrate = st.Getnetinterestrate();
    double gblinterestrate = st.Getgblinterestrate();
    double grantindex = st.Getgrantindex();
    double expectedchange = st.Getexpectedchange();
    double nSubsidy = GetBlockValue((chainActive.Tip()->nHeight) ,0)/10000000;
    int nHeight = (chainActive.Tip()->nHeight);
    int64_t volume = st.Getvolume();
    int64_t marketcap = st.aveprice() * volume;
    double grantsaverage = st.Getgrantsaverage();
    double gblmoneysupply = st.Getgblmoneysupply();
    int64_t grantstotal = st.Getgrantstotal();
    int64_t bankreserve = st.Getbankreserve();
    int64_t gblavailablecredit = st.Getgblavailablecredit();
    int64_t globaldebt = st.Getglobaldebt();
    ui->networkstatus->setText(networkstatus);
    double minsafereserve = gblmoneysupply * 0.05; 
    double maxreserve = gblmoneysupply * 0.25;
    double reserverequirement = gblmoneysupply * 0.1;
    double inflationindex = st.Getinflationindex();
    double consensusindex = st.Getconsensusindex();
    ui->bankstatus->setText(bankstatusPrevious);
    QString height = QString::number(nHeight);

    if (bankreserve < minsafereserve)
    {
        phase = "<p align=\"center\">Critical</p>";
        ui->progressBar->setValue(bankreserve);
        ui->progressBar->setMaximum(maxreserve);
       
    }
    else if (bankreserve > minsafereserve && bankreserve <  reserverequirement)
    {
        phase = "<p align=\"center\">Warning</p>";
        ui->progressBar->setValue(bankreserve);;
        ui->progressBar->setMaximum(maxreserve);
    }
    else if (bankreserve > reserverequirement && bankreserve < maxreserve)
    {
        phase = "<p align=\"center\">Healthy</p>";

        ui->progressBar->setValue(bankreserve);;
        ui->progressBar->setMaximum(maxreserve);
    }
    else if (bankreserve == maxreserve)
    {
        phase = "<p align=\"center\">Perfect</p>";

        ui->progressBar->setValue(bankreserve);
        ui->progressBar->setMaximum(maxreserve);
    }
    else
    {
        ui->progressBar->hide();
        phase = "<p align=\"center\">No Data</p>";
    }
    ui->progressBar->setFormat(phase);
	ui->cBox->setText(phase);
    

    QString qVolume = QLocale(QLocale::English).toString((qlonglong)volume);
    QString nmincreditscore = QString::number(mincreditscore, 'f', 6);
    QString navecreditscore = QString::number(avecreditscore, 'f', 6);
    QString nglbcreditscore = QString::number(glbcreditscore, 'f', 6);
    QString nbestcreditscore = QString::number(bestcreditscore, 'f', 6);
    QString nmintrust = QString::number(mintrust, 'f', 6);

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

    QString navetrust = QString::number(avetrust, 'f', 6);
    QString ngbltrust = QString::number(gbltrust, 'f', 6);
    QString nbesttrust = QString::number(besttrust, 'f', 6);
    QString ngrossinterestrate = QString::number(grossinterestrate, 'f', 6);
    QString nnetinterestrate = QString::number(netinterestrate, 'f', 6);



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

    if(grossinterestrate > grossinterestratePrevious)
    {
        ui->grossinterestrate->setText("<font color=\"green\">" + ngrossinterestrate + "</font>");
    }
    else if (grossinterestrate < grossinterestratePrevious)
    {
        ui->grossinterestrate->setText("<font color=\"red\">" + ngrossinterestrate + "</font>");
    }
    else
    {
    ui->grossinterestrate->setText(ngrossinterestrate);
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

    QString ngblinterestrate = QString::number(gblinterestrate, 'f', 6);
    QString ngrantindex = QString::number(grantindex, 'f', 6);
    QString nexpectedchange = QString::number(expectedchange, 'f', 6);
    QString ninflationindex = QString::number(inflationindex, 'f', 6);
    QString nconsensusindex = QString::number(consensusindex, 'f', 6);

    if(gblinterestrate > gblinterestratePrevious)
    {
        ui->gblinterestrate->setText("<font color=\"green\">" + ngblinterestrate + "</font>");
    }
    else if (gblinterestrate < gblinterestratePrevious)
    {
        ui->gblinterestrate->setText("<font color=\"red\">" + ngblinterestrate + "</font>");
    }
    else
    {
    ui->gblinterestrate->setText(ngblinterestrate);
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

    if(expectedchange > expectedchangePrevious)
    {
        ui->expectedchange->setText("<font color=\"green\">" + nexpectedchange + "</font>");
    }
    else if (expectedchange < expectedchangePrevious)
    {
        ui->expectedchange->setText("<font color=\"red\">" + nexpectedchange + "</font>");
    }
    else
    {
    ui->expectedchange->setText(nexpectedchange);
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
    
    QString ngrantsaverage = QString::number(grantsaverage, 'f', 6);
    QString ngblmoneysupply = QString::number(gblmoneysupply, 'f', 6);
    QString ngrantstotal = QString::number(grantstotal, 'f', 6);
    QString ngblavailablecredit = QString::number(gblavailablecredit, 'f', 6);
    QString nglobaldebt = QString::number(globaldebt, 'f', 6);    

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

    if(grantsaverage > grantsaveragePrevious)
    {
        ui->grantsaverage->setText("<font color=\"green\">" + ngrantsaverage + "</font>");
    }
    else if (grantsaverage < grantsaveragePrevious)
    {
        ui->grantsaverage->setText("<font color=\"red\">" + ngrantsaverage + "</font>");
    }
    else
    {
    ui->grantsaverage->setText(ngrantsaverage);
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

    updatePrevious(mincreditscore , avecreditscore, glbcreditscore, bestcreditscore, mintrust, avetrust, gbltrust, besttrust, grossinterestrate, netinterestrate, gblinterestrate, grantindex, expectedchange, inflationindex, consensusindex, nHeight, volume , marketcap , grantsaverage , gblmoneysupply , grantstotal, bankreserve, gblavailablecredit, globaldebt, bankstatus);
}

void BankStatisticsPage::updatePrevious(double mincreditscore ,double  avecreditscore,double  glbcreditscore,double  bestcreditscore,double  mintrust,double  avetrust,double  gbltrust,double  besttrust,double  grossinterestrate,double  netinterestrate,double  gblinterestrate,double  grantindex,double  expectedchange,double  inflationindex,double consensusindex,int  nHeight,int64_t  volume ,int64_t  marketcap ,int64_t  grantsaverage ,int64_t  gblmoneysupply ,int64_t  grantstotal,int64_t  bankreserve,int64_t  gblavailablecredit,int64_t  globaldebt, QString bankstatus)
{
    mincreditscorePrevious = mincreditscore;
    avecreditscorePrevious = avecreditscore;
    glbcreditscorePrevious = glbcreditscore;
    bestcreditscorePrevious = bestcreditscore;
    mintrustPrevious = mintrust;
    avetrustPrevious = avetrust;
    gbltrustPrevious = gbltrust;
    besttrustPrevious = besttrust;
    grossinterestratePrevious = grossinterestrate;
    netinterestratePrevious = netinterestrate;
    gblinterestratePrevious = gblinterestrate;
    marketcapPrevious = marketcap;
    grantindexPrevious = grantindex;
    expectedchangePrevious = expectedchange;
    inflationindexPrevious = inflationindex;
    consensusindexPrevious = consensusindex;
    grantsaveragePrevious = grantsaverage;
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
