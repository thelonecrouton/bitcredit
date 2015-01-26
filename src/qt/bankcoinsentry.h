// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_BANKCOINSENTRY_H
#define BITCREDIT_QT_BANKCOINSENTRY_H

#include "walletmodel.h"

#include <QStackedWidget>

class WalletModel;

namespace Ui {
    class BankCoinsEntry;
}

/**
 * A single entry in the dialog for sending bitcredits.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class BankCoinsEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit BankCoinsEntry(QWidget *parent = 0);
    ~BankCoinsEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendCoinsRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public slots:
    void clear();

signals:
    void removeEntry(BankCoinsEntry *entry);
    void payAmountChanged();

private slots:
    void deleteClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    SendCoinsRecipient recipient;
    Ui::BankCoinsEntry *ui;
    WalletModel *model;

    bool updateLabel(const QString &address);
};

#endif // BITCREDIT_QT_BANKCOINSENTRY_H
