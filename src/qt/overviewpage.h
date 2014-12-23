// Copyright (c) 2011-2013 The Bitcredits developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include "amount.h"

#include <QWidget>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QtNetwork>
#include <QDebug>
#include <QList>
#include <QNetworkAccessManager>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class WalletModel;
class IRCModel;

namespace Ui {
    class OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
	void setIRCModel(IRCModel *model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);
	void ircAppendMessage(QString message);
	      
signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
	IRCModel *ircmodel;
    CAmount currentBalance;
    CAmount currentUnconfirmedBalance;
    CAmount currentImmatureBalance;
    CAmount currentWatchOnlyBalance;
    CAmount currentWatchUnconfBalance;
    CAmount currentWatchImmatureBalance;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

    QString currentTag;
    QString linkString;
    QString titleString;
    QString dateString;
            
private slots:
    void updateDisplayUnit();
    void enableTrollbox();
    void updateTrollName();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
	void sendIRCMessage();
};

#endif // OVERVIEWPAGE_H
