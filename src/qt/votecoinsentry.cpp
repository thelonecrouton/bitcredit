// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcredit developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team

#include "votecoinsentry.h"
#include "ui_votecoinsentry.h"

#include "guiutil.h"
#include "bitcreditunits.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"


#include <QApplication>
#include <QClipboard>

VoteCoinsEntry::VoteCoinsEntry(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::VoteCoinsEntry),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    ui->payToLayout->setSpacing(4);
#endif
#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->payTo->setPlaceholderText(tr("Enter a Bitcredit voting address (starts with 6BCR)"));
#endif
    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(ui->payTo);
    GUIUtil::setupAddressWidget(ui->payTo, this);
}

VoteCoinsEntry::~VoteCoinsEntry()
{
    delete ui;
}

void VoteCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void VoteCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
    }
}

void VoteCoinsEntry::on_payTo_textChanged(const QString &address)
{
    if(!model)
        return;
    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);

}

void VoteCoinsEntry::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    clear();
}

void VoteCoinsEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void VoteCoinsEntry::clear()
{
    ui->payTo->clear();
    ui->payTo->setFocus();
    updateDisplayUnit();
}

void VoteCoinsEntry::on_deleteButton_clicked()
{
    emit removeEntry(this);
}

bool VoteCoinsEntry::validate()
{
    // Check input validity
    bool retval = true;

    if(!ui->payTo->hasAcceptableInput() ||
       (model && !model->validateAddress(ui->payTo->text())))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if(!ui->payTo->text().startsWith("6BCR")){
        ui->payTo->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient VoteCoinsEntry::getValue()
{
    SendCoinsRecipient rv;
    rv.address = ui->payTo->text();
    rv.amount =  ui->payAmount->currentIndex()+1000;

    return rv;
}

void VoteCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    ui->payTo->setText(value.address);
}

void VoteCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);

}

bool VoteCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty();
}

void VoteCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void VoteCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {

    }
}
