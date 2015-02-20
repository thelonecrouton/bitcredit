// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bankoverview.h"
#include "ui_bankoverview.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include "bitcreditunits.h"
#include "clientmodel.h"
#include "bankcoinsentry.h"
#include "base58.h"
#include "ui_interface.h"
#include "wallet.h"
#include "bankstatisticspage.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>

#include <QApplication>
#include <QClipboard>

#include "qvalidatedlineedit.h"
#include "qvaluecombobox.h"

BankOverview::BankOverview(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BankOverview),
    clientModel(0),
    model(0),
    walletbalance(-1),
    walletbalancebtc(-1),
    walletbalancefiat(-1),
    bankcredit(-1),
    bankcreditbtc(-1),
    bankcreditfiat(-1),
	grantbalance(-1),
	grantbalancebtc(-1),
    grantbalancefiat(-1),
    totalbalance(-1),
	totalbalancebtc(-1),
	totalbalancefiat(-1),
	trustrating(-1),
	creditscore(-1)
{
    ui->setupUi(this);
#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif
	
    
	setFocusProxy(ui->payAmount);
    
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    fNewRecipientAllowed = true;
    
    this->setStyleSheet("background-image:url(:/images/background);");

    connect(ui->payAmount, SIGNAL(valueChanged()), this, SIGNAL(payAmountChanged()));
}


void BankOverview::clear()
{
    ui->payAmount->clear();

    updateDisplayUnit();
}

bool BankOverview::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    return retval;
}



SendCoinsRecipient BankOverview::getValue()
{
    
    SendCoinsRecipient rv;

	if (ui->payTo->currentIndex()+1 ==0)
	{
		rv.address = "6DEVCRjNBJzgoALxYAZ87a4Td25D53WnHR";
		rv.label = "Development Bank";  
		ui->payAmount->setValue(100000000000); 
		rv.amount = 10000000000000;
	}

	else if (ui->payTo->currentText() == "Grant Reserve")
	{
		rv.address = "6AtYqDFdDNN6WDKa9cRcqxj9rvMJ6B4mZn";
		rv.label = "Grant Reserve"; 
		ui->payAmount->setValue(100000000000);
		rv.amount = 10000000000000;
	}	
    
    return rv;
}

void BankOverview::setValue(const SendCoinsRecipient &value)
{
    ui->payAmount->setValue(value.amount);
}

void BankOverview::setFocus()
{
    ui->payAmount->setFocus();
}

void BankOverview::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        BankOverview *entry = qobject_cast<BankOverview*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->setModel(model);
        }
    }
}


BankOverview::~BankOverview()
{
    delete ui;
}

void BankOverview::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        BankOverview *entry = qobject_cast<BankOverview*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
				int index;
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    // Format confirmation message
    QStringList formatted;
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        // generate bold amount string
        QString amount = "<b>" + BitcreditUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // secure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // insecure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);
    }

    fNewRecipientAllowed = false;


    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
   // processSendCoinsReturn(prepareStatus,
       // BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();
    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcreditUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    foreach(BitcreditUnits::Unit u, BitcreditUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcreditUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1 (= %2)")
        .arg(BitcreditUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user

        accept();

    fNewRecipientAllowed = true;
}

void BankOverview::reject()
{
    clear();
}

void BankOverview::accept()
{
    clear();
}


void BankOverview::setAddress(const QString &address)
{
    BankOverview *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        BankOverview *first = qobject_cast<BankOverview*>(ui->entries->itemAt(0)->widget());
        
    }

    entry->setAddress(address);
}

void BankOverview::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if(model && model->getOptionsModel())
    {
        ui->labelwallettotal->setText(BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void BankOverview::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update labelBalance with the current balance and the current unit
        ui->labelwallettotal->setText(BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance()));
    }
}



