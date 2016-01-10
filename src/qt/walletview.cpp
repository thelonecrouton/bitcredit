// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"
#include "exchangebrowser.h"
#include "adrenalinenodeconfigdialog.h"
#include "addeditadrenalinenode.h"
#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "bitcreditgui.h"
#include "clientmodel.h"
#include "statisticspage.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "messagemodel.h"
#include "overviewpage.h"
#include "receiptpage.h"
#include "servicespage.h"
#include "sendmessagesdialog.h"
#include "messagepage.h"
#include "invoiceviewpage.h"
#include "invoicepage.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "votecoinsdialog.h"
#include "walletmodel.h"
#include "utilitydialog.h"
#include "ui_interface.h"
#include "bidpage.h"
#include "blockexplorer.h"
#include "databasebrowser.h"
#include "databaseconnectionwidget.h"
#include "assetspage.h"
#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

WalletView::WalletView(QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0)
{
    // Create tabs
    overviewPage = new OverviewPage();
	exchangeBrowser = new ExchangeBrowser(this);
    statisticsPage = new StatisticsPage(this);
	bidPage = new BidPage(this);
	blockexplorer = new BlockExplorer(this);
    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);
    receiveCoinsPage = new ReceiveCoinsDialog();
    sendCoinsPage = new SendCoinsDialog();
	voteCoinsPage = new VoteCoinsDialog();
    sendMessagesPage = new SendMessagesDialog();
    messagePage = new MessagePage();
    invoicePage = new InvoicePage();
    receiptPage = new ReceiptPage();
    databasePage = new Browser();
    assetsPage = new AssetsPage(this);
    servicesPage = new ServicesPage();

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    addWidget(statisticsPage);
    addWidget(voteCoinsPage);
	addWidget(exchangeBrowser);
	addWidget(sendMessagesPage);
    addWidget(messagePage);
    addWidget(invoicePage);
    addWidget(receiptPage);
    addWidget(bidPage);
    addWidget(blockexplorer);
    addWidget(databasePage);
    addWidget(assetsPage);
    addWidget(servicesPage);
    
    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Clicking on "Export" allows to export the transaction list
    //connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
}

WalletView::~WalletView()
{
}

void WalletView::setBitcreditGUI(BitcreditGUI *gui)
{
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString)));

        connect(this, SIGNAL(incomingMessage(QString, QString, QString, QString, int)), gui, SLOT(incomingMessage(QString, QString, QString, QString, int)));

    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
    sendCoinsPage->setClientModel(clientModel);

}

void WalletView::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;

    // Put transaction list in tabs
    transactionView->setModel(walletModel);
    overviewPage->setWalletModel(walletModel);
    receiveCoinsPage->setModel(walletModel);
    sendCoinsPage->setModel(walletModel);
    voteCoinsPage->setModel(walletModel);
    sendMessagesPage->setWalletModel(walletModel);
    assetsPage->setModel(walletModel);
	servicesPage->setModel(walletModel);
	
    if (walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

        // Show progress dialog
        connect(walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }
}

void WalletView::setMessageModel(MessageModel *messageModel)
{
    this->messageModel = messageModel;
    if(messageModel)
    {
        // Report errors from wallet thread
        connect(messageModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        messagePage->setModel(messageModel);
        invoicePage->setModel(messageModel);
        receiptPage->setModel(messageModel);
        sendMessagesPage->setModel(messageModel);

        // Balloon pop-up for new message
        connect(messageModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewMessage(QModelIndex,int,int)));

    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();

    emit incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address);
}

void WalletView::processNewMessage(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if(!messageModel)
        return;

    MessageModel *mm = messageModel;

    QString sent_datetime = mm->index(start, MessageModel::ReceivedDateTime, parent).data().toString();
    QString from_address  = mm->index(start, MessageModel::FromAddress,      parent).data().toString();
    QString to_address    = mm->index(start, MessageModel::ToAddress,        parent).data().toString();
    QString message       = mm->index(start, MessageModel::Message,          parent).data().toString();
    int type 	          = mm->index(start, MessageModel::TypeInt,          parent).data().toInt();

    emit incomingMessage(sent_datetime, from_address, to_address, message, type);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoBlockExplorerPage()
{
    setCurrentWidget(blockexplorer);
}

void WalletView::gotoExchangeBrowserPage()
{
    setCurrentWidget(exchangeBrowser);
}

void WalletView::gotoDatabasePage()
{
    setCurrentWidget(databasePage);
}

void WalletView::gotoBidPage()
{
    setCurrentWidget(bidPage);
}

void WalletView::gotoServicesPage()
{
    setCurrentWidget(servicesPage);
}

void WalletView::gotoSendMessagesPage()
{
	setCurrentWidget(sendMessagesPage);
}

void WalletView::gotoMessagesPage()
{
    setCurrentWidget(messagePage);
}

void WalletView::gotoInvoicesPage()
{
    setCurrentWidget(invoicePage);
}

void WalletView::gotoReceiptPage()
{
    setCurrentWidget(receiptPage);
}

void WalletView::gotoStatisticsPage()
{
    setCurrentWidget(statisticsPage);
}

void WalletView::gotoVoteCoinsPage(QString addr)
{
    setCurrentWidget(voteCoinsPage);

    if (!addr.isEmpty())
        voteCoinsPage->setAddress(addr);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoAssetsPage()
{
    setCurrentWidget(assetsPage);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(sendCoinsPage);

    if (!addr.isEmpty())
        sendCoinsPage->setAddress(addr);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    emit encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!walletModel->backupWallet(filename)) {
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model

    if (walletModel->getEncryptionStatus() == WalletModel::Locked || walletModel->getEncryptionStatus() == WalletModel::UnlockedForAnonymizationOnly)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::UnlockAnonymize, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    dlg->show();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    dlg->show();
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void WalletView::printPaperWallets()
{
    if(!walletModel)
        return;

    PaperWalletDialog dlg(this);
    dlg.setModel(walletModel);
    dlg.exec();
}
