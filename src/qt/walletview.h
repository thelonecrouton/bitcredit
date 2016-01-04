// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_WALLETVIEW_H
#define BITCREDIT_QT_WALLETVIEW_H

#include "amount.h"

#include <QStackedWidget>

class BitcreditGUI;
class ClientModel;
class VoteCoinsDialog;
class OverviewPage;
class ReceiveCoinsDialog;
class SendCoinsDialog;
class SendCoinsRecipient;
class TransactionView;
class WalletModel;
class ExchangeBrowser;
class StatisticsPage;
class MessagePage;
class InvoicePage;
class ReceiptPage;
class MessageModel;
class SendMessagesDialog;
class AddEditAdrenalineNode;
class BidPage;
class BlockExplorer;
class ConnectionWidget;
class Browser;
class AssetsPage;

QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
class WalletView : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WalletView(QWidget *parent);
    ~WalletView();

    void setBitcreditGUI(BitcreditGUI *gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a bitcredit wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);

	void setMessageModel(MessageModel *messageModel);

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

private:
    ClientModel *clientModel;
    MessageModel *messageModel;
    WalletModel *walletModel;
	ExchangeBrowser *exchangeBrowser;
    OverviewPage *overviewPage;
    QWidget *transactionsPage;
    ReceiveCoinsDialog *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
	StatisticsPage *statisticsPage;
    TransactionView *transactionView;
    SendMessagesDialog *sendMessagesPage;
    MessagePage *messagePage;
    InvoicePage *invoicePage;
    ReceiptPage *receiptPage;
    BidPage *bidPage;
    VoteCoinsDialog *voteCoinsPage;
    BlockExplorer *blockexplorer;
    Browser *databasePage;
    AssetsPage *assetsPage;
    QProgressDialog *progressDialog;

public slots:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to exchange browser page */
    void gotoExchangeBrowserPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
	void gotoStatisticsPage();
	void gotoSendMessagesPage();
    void gotoBlockExplorerPage();
    void gotoDatabasePage();
    /** Switch to view messages page */
    void gotoMessagesPage();
    /** Switch to invoices page */
    void gotoInvoicesPage();
    /** Switch to receipt page */
    void gotoReceiptPage();
    /** Switch to send coins page */
    void gotoBidPage();
    void gotoAssetsPage();
    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");
    void gotoVoteCoinsPage(QString addr = "");
    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    void processNewMessage(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();
	/** Open the print paper wallets dialog **/
    void printPaperWallets();
    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);

signals:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged(int status);
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address);

    void incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type);
};

#endif // BITCREDIT_QT_WALLETVIEW_H
