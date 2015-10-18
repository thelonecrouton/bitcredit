// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_OVERVIEWPAGE_H
#define BITCREDIT_QT_OVERVIEWPAGE_H

#include "amount.h"

#include <QWidget>
#include <QtGui>
#include <QStyleOption>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QtNetwork>
#include <QDebug>
#include <QList>


class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class WalletModel;

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

protected:
    void paintEvent(QPaintEvent *)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);

        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);
    void updateDarksendProgress();

public slots:
    void darkSendStatus();
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance);
	
	      
signals:
    void transactionClicked(const QModelIndex &index);

private:
    QTimer *timer;
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CAmount currentBalance;
    CAmount currentUnconfirmedBalance;
    CAmount currentImmatureBalance;
    qint64 currentAnonymizedBalance;
    qint64 cachedTxLocks;
    qint64 lastNewBlock;
    CAmount currentWatchOnlyBalance;
    CAmount currentWatchUnconfBalance;
    CAmount currentWatchImmatureBalance;

    int showingDarkSendMessage;
    int darksendActionCheck;
    int cachedNumBlocks;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void toggleDarksend();
    void darksendAuto();
    void darksendReset();
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
	
};

#endif // BITCREDIT_QT_OVERVIEWPAGE_H
