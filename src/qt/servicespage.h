#ifndef SERVICESPAGE_H
#define SERVICESPAGE_H

#include <QWidget>
#include <string>

#include "walletmodel.h"

class WalletModel;

namespace Ui {
class ServicesPage;
}

class ServicesPage : public QWidget
{
    Q_OBJECT

public:
    explicit ServicesPage(QWidget *parent = 0);
    ~ServicesPage();
    void setModel(WalletModel *model);

private slots:
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
private:
    Ui::ServicesPage *ui;
    void gettrust();
    WalletModel *model;
};

#endif // P2PSERVICES_H
