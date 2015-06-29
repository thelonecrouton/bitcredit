// Copyright (c) 2014-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_UTILITYDIALOG_H
#define BITCREDIT_QT_UTILITYDIALOG_H

#include <QDialog>
#include <QObject>
#include "walletmodel.h"
#include "util.h"

class BitcreditGUI;
class ClientModel;

namespace Ui {
    class HelpMessageDialog;
    class PaperWalletDialog;
}

/** "Paper Wallet" dialog box */
class PaperWalletDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaperWalletDialog(QWidget *parent);
    ~PaperWalletDialog();

    void setModel(WalletModel *model);

private:
    Ui::PaperWalletDialog *ui;
    WalletModel *model;
    static const int PAPER_WALLET_READJUST_LIMIT = 20;
    static const int PAPER_WALLET_PAGE_MARGIN = 50;

private slots:
    void on_getNewAddress_clicked();
    void on_printButton_clicked();
};


/** "Help message" dialog box */
class HelpMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpMessageDialog(QWidget *parent, bool about);
    ~HelpMessageDialog();

    void printToConsole();
    void showOrPrint();

private:
    Ui::HelpMessageDialog *ui;
    QString text;
        
private slots:
    void on_okButton_accepted();
    void showIntro();
    void showBasic();
    void showBanknodes();
    void showTech();
    void showOther1();
    void showChainz();
    void showWiki();
    void showOther2();
};


/** "Shutdown" window */
class ShutdownWindow : public QWidget
{
    Q_OBJECT

public:
    ShutdownWindow(QWidget *parent=0, Qt::WindowFlags f=0);
    static void showShutdownWindow(BitcreditGUI *window);

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // BITCREDIT_QT_UTILITYDIALOG_H
