#include "bidpage.h"
#include "ui_bidpage.h"
#include "util.h"
#include "guiutil.h"
#include "clientmodel.h"
#include "chainparams.h"
#include "main.h"
#include "net.h"
#include "basenodeman.h"

#include <fstream>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

BidPage::BidPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::BidPage)
{
    ui->setupUi(this);

    ui->lineEditBid->setEnabled(false);  //  cannot calc until update clicked and data fetched

    connect(ui->pushButtonBTCExplorer, SIGNAL(clicked()), this, SLOT(SummonBTCExplorer()));
    connect(ui->pushButtonBTC, SIGNAL(clicked()), this, SLOT(SummonBTCWallet()));
    connect(ui->pushButtonRefresh, SIGNAL(clicked()), this, SLOT(GetBids()));
    connect(ui->lineEditBid, SIGNAL(returnPressed()), this, SLOT(Estimate()));

    theme = GetArg("-theme", "");
    QString themestring = QString::fromUtf8(theme.c_str());
    if (themestring.contains("orange"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
        ui->frame->setStyleSheet("border: 2px solid #ffa405");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("dark"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
        ui->frame->setStyleSheet("border: 2px solid #ffa405");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("green"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #45f806");
        ui->frame->setStyleSheet("border: 2px solid #45f806");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("blue"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #031cd7");
        ui->frame->setStyleSheet("border: 2px solid #031cd7");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("pink"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ff03a3");
        ui->frame->setStyleSheet("border: 2px solid #ff03a3");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("purple"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #a106a7");
        ui->frame->setStyleSheet("border: 2px solid #a106a7");
        ui->label_heading->setStyleSheet("border: none");
    }
    else if (themestring.contains("turq"))
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #0ab4dc");
        ui->frame->setStyleSheet("border: 2px solid #0ab4dc");
        ui->label_heading->setStyleSheet("border: none");
    }
    //fallback on default
    else
    {
        ui->pushButtonRefresh->setStyleSheet("border: 2px solid #ffa405");
        ui->frame->setStyleSheet("border: 1px solid #ffa405");
        ui->label_heading->setStyleSheet("border: none");
    }

}
void BidPage::setClientModel(ClientModel *model)
{
    clientModel = model;
    if(model)
    {
        setNumBlocks(model->getNumBlocks());
        //connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));
    }
}

int BidPage::getNumBlocks()
{
    LOCK(cs_main);
    return chainActive.Height();
}

void BidPage::setNumBlocks(int count)
{
    ui->labelNumber->setText(QString::number(count));

}

void BidPage::Estimate()
{
    QString bidtotal = ui->labelTotal_2->text();
    float bidz = bidtotal.toFloat();
    float mybid = ui->lineEditBid->text().toFloat();
    float newtotal = bidz + mybid;
    float mybcr = (mybid / newtotal) * 18000;
    QString mybcrz = QString::number(mybcr);
    float cost = mybid / mybcr;
    QString coststr = QString::number(cost, 'f', 8);
    ui->labelBCR->setText("<b>" + mybcrz + "</b> BCR @ " + "<b>" + coststr + "</b>");
}

void BidPage::GetBids()
{
    // calc time until next 00:00 GMT
    long int startdate = 1450396800; // 18 December 2015 00:00
    long int current = GetTime();
    long int diff = current - startdate;

    int until = 86400 - (diff % 86400);
    ui->labelNumber->setText(GUIUtil::formatDurationStr(until));

    // get default datadir, tack on bidtracker
    QString dataDir = getDefaultDataDirectory();
    QString bidDir = "bidtracker";
    QString datPath = pathAppend(dataDir, bidDir);

    // get bids from /bidtracker/final.dat
    QString bidspath = QDir(datPath).filePath("prefinal.dat");
    double btctot = 0;
    // for each line in file, get the float after the comma
    QFile bidsFile(bidspath);
    if (bidsFile.open(QIODevice::ReadOnly))
    {
       QTextStream btcin(&bidsFile);
       while (!btcin.atEnd())
       {
           QString line = btcin.readLine();
           if (line.isEmpty()){ continue; }
           else if (line.startsWith("1"))  //  BTC
           {
               QString btcamount = line.remove(0, 35);
               btctot = btctot + btcamount.toDouble();
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
    QString btctotal = QString::number(btctotU, 'f', 8);
    //ui->labelBTC_2->setText(btctotal);

    // add 'em up and display 'em
    double alltot = btctotU;
    QString alltotal = QString::number(alltot, 'f', 8);
    ui->labelTotal_2->setText(alltotal);

    // calc price per BCR based on total bids and display it
    double bcrprice = alltot / 18000;
    QString bcrPrice = QString::number(bcrprice, 'f', 8);
    ui->labelEstprice_2->setText(bcrPrice);

    ui->lineEditBid->setEnabled(true);
}


QString BidPage::pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString BidPage::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

void BidPage::SummonBTCExplorer()
{
    QDesktopServices::openUrl(QUrl("https://btc.blockr.io/address/info/1BCRbid2i3wbgqrKtgLGem6ZchcfYbnhNu", QUrl::TolerantMode));
}

void BidPage::SummonBTCWallet()
{
    QProcess *proc = new QProcess(this);
    #ifdef linux
        proc->startDetached("bitcoin-qt");
    #elif _WIN32
        proc->startDetached("bitcoin-qt.exe");
    #endif
}

BidPage::~BidPage()
{
    delete ui;
}
