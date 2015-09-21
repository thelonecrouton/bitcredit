#include "testpage.h"
#include "ui_testpage.h"
#include "util.h"
#include "guiutil.h"
#include "clientmodel.h"
#include "chainparams.h"
#include "main.h"
#include "net.h"
#include "banknodeman.h"

#include <fstream>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

TestPage::TestPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestPage)
{
    ui->setupUi(this);

    ui->lineEditBid->setEnabled(false);  //  cannot calc until update clicked and data fetched
    
    connect(ui->pushButtonBTCExplorer, SIGNAL(clicked()), this, SLOT(SummonBTCExplorer()));
    connect(ui->pushButtonLTCExplorer, SIGNAL(clicked()), this, SLOT(SummonLTCExplorer()));
    connect(ui->pushButtonDASHExplorer, SIGNAL(clicked()), this, SLOT(SummonDASHExplorer()));
    connect(ui->pushButtonBTC, SIGNAL(clicked()), this, SLOT(SummonBTCWallet()));
    connect(ui->pushButtonLTC, SIGNAL(clicked()), this, SLOT(SummonLTCWallet()));
    connect(ui->pushButtonDASH, SIGNAL(clicked()), this, SLOT(SummonDASHWallet()));
    connect(ui->pushButtonRefresh, SIGNAL(clicked()), this, SLOT(GetBids()));    
    connect(ui->lineEditBid, SIGNAL(returnPressed()), this, SLOT(Estimate()));
    
    theme = GetArg("-theme", "");
    QString themestring = QString::fromUtf8(theme.c_str());
    if (themestring.contains("orange"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
    }
    else if (themestring.contains("dark"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
    }
    else if (themestring.contains("green"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #45f806");      
    }
    else if (themestring.contains("blue"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #088af8");         
    }
    else if (themestring.contains("pink"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #fb04db");        
    }
    else if (themestring.contains("purple"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #cb03d2");            
    }
    else if (themestring.contains("turq"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #0ab4dc");          
    } 
    //fallback on default
    else
    {    
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
    }
    
}
void TestPage::setClientModel(ClientModel *model)
{
    clientModel = model;
    if(model)
    {
        setNumBlocks(model->getNumBlocks());
        //connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));
    }
}

int TestPage::getNumBlocks()
{
    LOCK(cs_main);
    return chainActive.Height();
}

void TestPage::setNumBlocks(int count)
{
    ui->labelNumber->setText(QString::number(count));
    
}

void TestPage::Estimate()
{
    QString bidtotal = ui->labelTotal_2->text(); 
    float bidz = bidtotal.toFloat();
    float mybid = ui->lineEditBid->text().toFloat();
    float newtotal = bidz + mybid;
    float mybcr = (mybid / newtotal) * 30000;
    QString mybcrz = QString::number(mybcr);
    ui->labelBCR->setText("<b>" + mybcrz + "</b> BCR");   
}

void TestPage::GetBids()
{
    // get current blockheight and calc next superblock
    int base = 200700;
    int current = TestPage::getNumBlocks();
    int diff = current - base;

    int until = 900 - (diff % 900);
    QString blocks = QString::number(until);
    ui->labelNumber->setText(blocks);

    // get default datadir, tack on bidtracker
    QString dataDir = getDefaultDataDirectory();
    QString bidDir = "bidtracker";
    QString datPath = pathAppend(dataDir, bidDir);

    // get bids from /bidtracker/final.dat
    QString bidspath = QDir(datPath).filePath("final.dat");
    double btctot = 0;
    double ltctot = 0;
    double dashtot = 0;
    // for each line in file, get the float after the comma
    QFile bidsFile(bidspath);
    if (bidsFile.open(QIODevice::ReadOnly))
    {
       QTextStream btcin(&bidsFile);
       while (!btcin.atEnd())
       {
           QString line = btcin.readLine();
           if (line.startsWith("1"))  //  BTC
           {
               QString btcamount = line.remove(0, 35);
               btctot = btctot + btcamount.toDouble();
           }
           else if (line.startsWith("L"))  //  LTC
           {
               QString ltcamount = line.remove(0, 35);
               ltctot = ltctot + ltcamount.toDouble();
           }
           else if (line.startsWith("X"))  //  DASH
           {
               QString dashamount = line.remove(0, 35);
               dashtot = dashtot + dashamount.toDouble();
           }
	   else //  we should never get here
           {
               QMessageBox::information(0, QString("Oops!"), QString("There is a problem with the file, please try again later!"), QMessageBox::Ok);
           }
       }
       bidsFile.close();
    }

    // btctot, ltctot and dashtot are in satoshis, so divide by 10000000 to get right units
    // to do - add radiobuttons or dropdown to select sats or not?
    double btctotU = btctot / 100000000;
    double ltctotU = ltctot / 100000000;
    double dashtotU = dashtot / 100000000;
    QString btctotal = QString::number(btctotU, 'f', 8);
    QString ltctotal = QString::number(ltctotU, 'f', 8);
    QString dashtotal = QString::number(dashtotU, 'f', 8);
    ui->labelBTC_2->setText(btctotal);
    ui->labelLTC_2->setText(ltctotal);
    ui->labelDASH_2->setText(dashtotal);

    // add 'em up and display 'em
    double alltot = btctotU + ltctotU + dashtotU;
    QString alltotal = QString::number(alltot, 'f', 8);
    ui->labelTotal_2->setText(alltotal);

    // calc price per BCR based on total bids and display it
    double bcrprice = alltot / 30000;
    QString bcrPrice = QString::number(bcrprice, 'f', 8);
    ui->labelEstprice_2->setText(bcrPrice);     

    ui->lineEditBid->setEnabled(true);
}


QString TestPage::pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString TestPage::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

void TestPage::SummonBTCExplorer()
{
    QDesktopServices::openUrl(QUrl("https://btc.blockr.io/address/info/16f5dJd4EHRrQwGGRMczA69qbJYs4msBQ5", QUrl::TolerantMode));
}

void TestPage::SummonLTCExplorer()
{
    QDesktopServices::openUrl(QUrl("https://ltc.blockr.io/address/info/Lc7ebfQPz6VJ8qmXYaaFxBYLpDz2XsDu7c", QUrl::TolerantMode));
}

void TestPage::SummonDASHExplorer()
{
    QDesktopServices::openUrl(QUrl("http://explorer.dashpay.io/address/Xypcx2iE8rCtC3tjw5M8sxpRzn4JuoSaBH", QUrl::TolerantMode));
}

void TestPage::SummonBTCWallet()
{
    QProcess *proc = new QProcess(this);
    proc->startDetached("bitcoin-qt");
}

void TestPage::SummonLTCWallet()
{
    QProcess *proc = new QProcess(this);
    proc->startDetached("litecoin-qt");
}

void TestPage::SummonDASHWallet()
{
    QProcess *proc = new QProcess(this);
    proc->startDetached("dash-qt");
}




/*
void ChatWindow ::setModel(ClientModel *model)
{
    this->model = model;
}
*/

TestPage::~TestPage()
{
    delete ui;
}
