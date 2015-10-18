#include "banknodemanager.h"
#include "ui_banknodemanager.h"
#include "addeditadrenalinenode.h"
#include "adrenalinenodeconfigdialog.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "activebanknode.h"
#include "banknodeconfig.h"
#include "banknodeman.h"
#include "walletdb.h"
#include "wallet.h"
#include "init.h"
#include "stdio.h"
#include "util.h"
#include "guiutil.h"
#include "intro.h"
#include "bitcreditgui.h"

#include <boost/filesystem.hpp>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QScroller>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>


BanknodeManager::BanknodeManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BanknodeManager),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->editButton->setEnabled(false);
    ui->getConfigButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->copyAddressButton->setEnabled(false);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    subscribeToCoreSignals();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    timer->start(30000);

    LOCK(cs_adrenaline);
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        updateAdrenalineNode(QString::fromStdString(adrenaline.second.sAlias), QString::fromStdString(adrenaline.second.sAddress), QString::fromStdString(adrenaline.second.sBanknodePrivKey), QString::fromStdString(adrenaline.second.sCollateralAddress));
    }

    updateNodeList();
}

BanknodeManager::~BanknodeManager()
{
    delete ui;
}

static void NotifyAdrenalineNodeUpdated(BanknodeManager *page, CAdrenalineNodeConfig nodeConfig)
{
    // alias, address, privkey, collateral address
    QString alias = QString::fromStdString(nodeConfig.sAlias);
    QString addr = QString::fromStdString(nodeConfig.sAddress);
    QString privkey = QString::fromStdString(nodeConfig.sBanknodePrivKey);
    QString collateral = QString::fromStdString(nodeConfig.sCollateralAddress);
    
    QMetaObject::invokeMethod(page, "updateAdrenalineNode", Qt::QueuedConnection,
                              Q_ARG(QString, alias),
                              Q_ARG(QString, addr),
                              Q_ARG(QString, privkey),
                              Q_ARG(QString, collateral)
                              );
}

void BanknodeManager::subscribeToCoreSignals()
{
    // Connect signals to core
    uiInterface.NotifyAdrenalineNodeChanged.connect(boost::bind(&NotifyAdrenalineNodeUpdated, this, _1));
}

void BanknodeManager::unsubscribeFromCoreSignals()
{
    // Disconnect signals from core
    uiInterface.NotifyAdrenalineNodeChanged.disconnect(boost::bind(&NotifyAdrenalineNodeUpdated, this, _1));
}

void BanknodeManager::on_tableWidget_2_itemSelectionChanged()
{
    if(ui->tableWidget_2->selectedItems().count() > 0)
    {
        ui->editButton->setEnabled(true);
        ui->getConfigButton->setEnabled(true);
        ui->startButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
	ui->copyAddressButton->setEnabled(true);
    }
}

void BanknodeManager::updateAdrenalineNode(QString alias, QString addr, QString privkey, QString collateral)
{
    LOCK(cs_adrenaline);
    bool bFound = false;
    int nodeRow = 0;
    for(int i=0; i < ui->tableWidget_2->rowCount(); i++)
    {
        if(ui->tableWidget_2->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound)
        ui->tableWidget_2->insertRow(0);

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    QTableWidgetItem *statusItem = new QTableWidgetItem("");
    QTableWidgetItem *collateralItem = new QTableWidgetItem(collateral);

    ui->tableWidget_2->setItem(nodeRow, 0, aliasItem);
    ui->tableWidget_2->setItem(nodeRow, 1, addrItem);
    ui->tableWidget_2->setItem(nodeRow, 2, statusItem);
    ui->tableWidget_2->setItem(nodeRow, 3, collateralItem);
}

static QString seconds_to_DHMS(quint32 duration)
{
  QString res;
  int seconds = (int) (duration % 60);
  duration /= 60;
  int minutes = (int) (duration % 60);
  duration /= 60;
  int hours = (int) (duration % 24);
  int days = (int) (duration / 24);
  if((hours == 0)&&(days == 0))
      return res.sprintf("%02dm:%02ds", minutes, seconds);
  if (days == 0)
      return res.sprintf("%02dh:%02dm:%02ds", hours, minutes, seconds);
  return res.sprintf("%dd %02dh:%02dm:%02ds", days, hours, minutes, seconds);
}

void BanknodeManager::updateNodeList()
{
    TRY_LOCK(cs_banknodepayments, lockBanknodes);
    if(!lockBanknodes)
        return;

    ui->countLabel->setText("Updating...");
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    BOOST_FOREACH(CBanknode mn, mnodeman.vBanknodes) 
    {
        int mnRow = 0;
        ui->tableWidget->insertRow(0);

 	// populate list
	// Address, Rank, Active, Active Seconds, Last Seen, Pub Key
	QTableWidgetItem *activeItem = new QTableWidgetItem(QString::number(mn.IsEnabled()));
	QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
	QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(mnodeman.GetBanknodeRank(mn.vin, chainActive.Tip()->nHeight)));
	QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(seconds_to_DHMS((qint64)(mn.lastTimeSeen - mn.sigTime)));
	QTableWidgetItem *lastSeenItem = new QTableWidgetItem(seconds_to_DHMS((qint64)(GetTime()-mn.lastTimeSeen)));

	
	CScript pubkey;
        pubkey =GetScriptForDestination(mn.pubkey.GetID());
        CTxDestination address1;
        ExtractDestination(pubkey, address1);
        CBitcreditAddress address2(address1);
	QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(address2.ToString()));
	
	ui->tableWidget->setItem(mnRow, 0, addressItem);
	ui->tableWidget->setItem(mnRow, 1, rankItem);
	ui->tableWidget->setItem(mnRow, 2, activeItem);
	ui->tableWidget->setItem(mnRow, 3, activeSecondsItem);
	ui->tableWidget->setItem(mnRow, 4, lastSeenItem);
	ui->tableWidget->setItem(mnRow, 5, pubkeyItem);
    }

    
    // get default datadir and set path to mybanknodes.txt
    QString dataDir = getDefaultDataDirectory();
    QString path = QDir(dataDir).filePath("mybanknodes.txt");
        
    theme = GetArg("-theme", "");
    themestring = QString::fromUtf8(theme.c_str());  

    //check if file exists
    QFileInfo checkFile(path);
    if (checkFile.exists() && checkFile.isFile()) 
    {
        QFile myTextFile(path);
        QStringList myStringList;
        if (!myTextFile.open(QIODevice::ReadOnly))
            {
                QMessageBox::information(0, "Error opening file", myTextFile.errorString());
            }
        else
            {  
                while(!myTextFile.atEnd())
                {
                    myStringList.append(myTextFile.readLine());
                }
                myTextFile.close();
            }
        QString listitems = myStringList.join(""); 
        
        //search for pubkeys that match those in our list
        //ser our own count
        ours = 0;
        int rows = ui->tableWidget->rowCount();
        if (rows >1)
        for(int i = 0; i < rows; ++i)
        {
            QTableWidgetItem *temp = ui->tableWidget->item(i, 5);
            QString str1 = temp->text();
            //if showMineOnlychecked, hide everything else
            if ((!listitems.contains(str1)) && ui->showMineOnly->isChecked())
            {
            	ui->tableWidget->setRowHidden(i, true);
            	
            	
            }	
            //else just change background colour
            else if (listitems.contains(str1))
            {
                //highlight according to stylesheet
                if (themestring.contains("orange"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
                else if (themestring.contains("dark"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
                else if (themestring.contains("green"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#45f806");      
                }
                else if (themestring.contains("blue"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#088af8");         
                }
                else if (themestring.contains("pink"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#fb04db");        
                }
                else if (themestring.contains("purple"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#cb03d2");            
                }
                else if (themestring.contains("turq"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#0ab4dc");          
                } 
                //fallback on default
                else
                {    
                ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
            ours += 1;
            }
            
        }
        QString ourcount = QString::number(ours);
        ui->countLabel->setText(QString::number(ui->tableWidget->rowCount()) + "   (Mine: " + ourcount + ")");    
    }
    else
    {
         ui->countLabel->setText(QString::number(ui->tableWidget->rowCount()));
    }
}

QString BanknodeManager::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

void BanknodeManager::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
    }
}

void BanknodeManager::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
    }

}

void BanknodeManager::on_createButton_clicked()
{
    AddEditAdrenalineNode* aenode = new AddEditAdrenalineNode();
    aenode->exec();
}

void BanknodeManager::on_copyAddressButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sCollateralAddress = ui->tableWidget_2->item(r, 3)->text().toStdString();

    QApplication::clipboard()->setText(QString::fromStdString(sCollateralAddress));
}

void BanknodeManager::on_editButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    // get existing config entry
}

void BanknodeManager::on_getConfigButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];
    std::string sPrivKey = c.sBanknodePrivKey;
    AdrenalineNodeConfigDialog* d = new AdrenalineNodeConfigDialog(this, QString::fromStdString(sAddress), QString::fromStdString(sPrivKey));
    d->exec();
}

void BanknodeManager::on_removeButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this, "Delete Banknode?", "Are you sure you want to delete this Banknode configuration?", QMessageBox::Yes|QMessageBox::No);

    if(confirm == QMessageBox::Yes)
    {
        QModelIndex index = selected.at(0);
        int r = index.row();
        std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
        CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];
        CWalletDB walletdb(pwalletMain->strWalletFile);
        pwalletMain->mapMyAdrenalineNodes.erase(sAddress);
        walletdb.EraseAdrenalineNodeConfig(c.sAddress);
        ui->tableWidget_2->clearContents();
        ui->tableWidget_2->setRowCount(0);
        BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
        {
            updateAdrenalineNode(QString::fromStdString(adrenaline.second.sAlias), QString::fromStdString(adrenaline.second.sAddress), QString::fromStdString(adrenaline.second.sBanknodePrivKey), QString::fromStdString(adrenaline.second.sCollateralAddress));
        }
    }
}

void BanknodeManager::on_startButton_clicked()
{
    // start the node
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];

    std::string errorMessage;
    bool result = activeBanknode.RegisterByPubKey(c.sAddress, c.sBanknodePrivKey, c.sCollateralAddress, errorMessage);

    QMessageBox msg;
    if(result)
        msg.setText("Banknode at " + QString::fromStdString(c.sAddress) + " started.");
    else
        msg.setText("Error: " + QString::fromStdString(errorMessage));

    msg.exec();
}

void BanknodeManager::on_stopButton_clicked()
{
    // start the node
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];

    std::string errorMessage;
    bool result = activeBanknode.StopBankNode(c.sAddress, c.sBanknodePrivKey, errorMessage);
    QMessageBox msg;
    if(result)
    {
        msg.setText("Banknode at " + QString::fromStdString(c.sAddress) + " stopped.");
    }
    else
    {
        msg.setText("Error: " + QString::fromStdString(errorMessage));
    }
    msg.exec();
}

void BanknodeManager::on_startAllButton_clicked()
{
    std::string results;
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        CAdrenalineNodeConfig c = adrenaline.second;
	std::string errorMessage;
        bool result = activeBanknode.RegisterByPubKey(c.sAddress, c.sBanknodePrivKey, c.sCollateralAddress, errorMessage);
	if(result)
	{
   	    results += c.sAddress + ": STARTED\n";
	}	
	else
	{
	    results += c.sAddress + ": ERROR: " + errorMessage + "\n";
	}
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(results));
    msg.exec();
}

void BanknodeManager::on_stopAllButton_clicked()
{
    std::string results;
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        CAdrenalineNodeConfig c = adrenaline.second;
	std::string errorMessage;
        bool result = activeBanknode.StopBankNode(c.sAddress, c.sBanknodePrivKey, errorMessage);
	if(result)
	{
   	    results += c.sAddress + ": STOPPED\n";
	}	
	else
	{
	    results += c.sAddress + ": ERROR: " + errorMessage + "\n";
	}
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(results));
    msg.exec();
}
