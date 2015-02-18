// Copyright (c) 2014-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bankcoinsdialog.h"
#include "ui_bankcoinsdialog.h"

#include "addresstablemodel.h"
#include "bitcreditunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "bankcoinsentry.h"
#include "walletmodel.h"
#include "bankstatisticspage.h"
#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"
#include "wallet.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>

BankCoinsDialog::BankCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BankCoinsDialog),
    clientModel(0),
    model(0),
    fNewRecipientAllowed(true),
    fFeeMinimized(true)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif



    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

   
    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1); // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0); // recommended
    if (!settings.contains("nCustomFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nCustomFeeRadio", 1); // total at least
    if (!settings.contains("nCustomFeeRadio"))
        settings.setValue("nCustomFeeRadio", 0); // per kilobyte
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    if (!settings.contains("fSendFreeTransactions"))
        settings.setValue("fSendFreeTransactions", false);

	this->setStyleSheet("background-image:url(:/images/background);");

}

void BankCoinsDialog::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(updateSmartFeeLabel()));
    }
}

void BankCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            BankCoinsEntry *entry = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        
        coinControlUpdateLabels();
    }
}

BankCoinsDialog::~BankCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    delete ui;
}

void BankCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        BankCoinsEntry *entry = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
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
    if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
    else
        prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

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
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void BankCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void BankCoinsDialog::reject()
{
    clear();
}

void BankCoinsDialog::accept()
{
    clear();
}

BankCoinsEntry *BankCoinsDialog::addEntry()
{
    BankCoinsEntry *entry = new BankCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(BankCoinsEntry*)), this, SLOT(removeEntry(BankCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateTabsAndLabels();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}


void BankCoinsDialog::updatebankstats()
{
	BankStatisticsPage rm;
	double trst = rm.trustr;
	QString ntrustrating = QString::number(trst, 'f', 6);
    ui->trustrating->setText("<font color=\"green\">" + ntrustrating + "</font>");
       
}
void BankCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
    
}

void BankCoinsDialog::removeEntry(BankCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *BankCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        BankCoinsEntry *entry = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void BankCoinsDialog::setAddress(const QString &address)
{
    BankCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        BankCoinsEntry *first = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void BankCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    BankCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        BankCoinsEntry *first = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool BankCoinsDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void BankCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void BankCoinsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0);
    
    
    updateSmartFeeLabel();
}

void BankCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to BankCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid, please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::InsaneFee:
        msgParams.first = tr("A fee higher than %1 is considered an insanely high fee.").arg(BitcreditUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), 10000000));
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    emit message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void BankCoinsDialog::minimizeFeeSection(bool fMinimize)
{
    fFeeMinimized = fMinimize;
}

void BankCoinsDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void BankCoinsDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void BankCoinsDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;
}


void BankCoinsDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    updateFeeMinimizedLabel();
}

// Coin Control: settings menu - coin control enabled/disabled by user
void BankCoinsDialog::coinControlFeatureChanged(bool checked)
{
    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    if (checked)
        coinControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void BankCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void BankCoinsDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        
    }
       
}

// Coin Control: custom change address changed
void BankCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        

        CBitcreditAddress addr = CBitcreditAddress(text.toStdString());

        if (text.isEmpty()) // Nothing entered
         {
            CPubKey pubkey;
            CKeyID keyid;
            addr.GetKeyID(keyid);
            if (!model->getPubKey(keyid, pubkey)) // Unknown change address
            {
                
            }
            else // Known change address
            {
                

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                
                CoinControlDialog::coinControl->destChange = addr.Get();
            }
        }
    }
}

// Coin Control: update labels
void BankCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        BankCoinsEntry *entry = qobject_cast<BankCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats

    }
    else
    {
        // hide coin control stats

    }
}
