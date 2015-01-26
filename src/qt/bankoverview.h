<<<<<<< HEAD
=======
// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_BANKOVERVIEWPAGE_H
#define BITCREDIT_QT_BANKOVERVIEWPAGE_H

#include "amount.h"

#include <QWidget>

#include <QDialog>
#include <QString>
#include "walletmodel.h"

class ClientModel;
class WalletModel;
class OptionsModel;
class SendCoinsEntry;
class SendCoinsRecipient;

namespace Ui {
    class BankOverview;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
class QUrl;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class BankOverview : public QWidget
{
    Q_OBJECT
public:
    explicit BankOverview(QWidget *parent = 0);
    ~BankOverview();

    void setClientModel(ClientModel *clientModel);
    void showOutOfSyncWarning(bool fShow);
	int getdest();
	bool isClear();

	void setModel(WalletModel *model);
    bool validate();
    SendCoinsRecipient getValue();


    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handleURI(const QString &uri);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);
	void setFocus();
	
public slots:
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);
    void clear();	
    void reject();
    void accept();
    
          
signals:
    void payAmountChanged();
    void message(const QString &title, const QString &message, unsigned int style);
    
private:
    Ui::BankOverview *ui;
    ClientModel *clientModel;
    WalletModel *model;
    bool fNewRecipientAllowed;
    SendCoinsRecipient recipient;	
    CAmount walletbalance;
    CAmount walletbalancebtc;
    CAmount walletbalancefiat;
    CAmount bankcredit;
    CAmount bankcreditbtc;
    CAmount bankcreditfiat;
	CAmount grantbalance;
	CAmount grantbalancebtc;
    CAmount grantbalancefiat;
    CAmount totalbalance;
	CAmount totalbalancebtc;
	CAmount totalbalancefiat;
	double trustrating;
	double creditscore;
	bool updateLabel(const QString &address);
	void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
	bool fFeeMinimized;


private slots:

    void updateDisplayUnit();
    void on_sendButton_clicked();
   
};

#endif // BITCREDIT_QT_OVERVIEWPAGE_H
>>>>>>> origin/master2

