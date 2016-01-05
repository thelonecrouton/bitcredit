#ifndef P2PSERVICES_H
#define P2PSERVICES_H

#include <QWidget>
#include <string>

#include "walletmodel.h"

class WalletModel;

namespace Ui {
class p2pservices;
}

class p2pservices : public QWidget
{
    Q_OBJECT

public:
    explicit p2pservices(QWidget *parent = 0);
    ~p2pservices();
    void setModel(WalletModel *model);

private slots:
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
private:
    Ui::p2pservices *ui;
    void gettrust();
    WalletModel *model;
};

#endif // P2PSERVICES_H
