#ifndef BIDPAGE_H
#define BIDPAGE_H

#include "clientmodel.h"

#include <QWidget>

namespace Ui
{
    class BidPage;
}

class BidPage: public QWidget
{
    Q_OBJECT

public:
    BidPage(QWidget *parent = 0);
    ~BidPage();

    QString str;
    QString btctotal;
    double btctot;
    QString ltctotal;
    double ltctot;
    QString dashtotal;
    double dashtot;
    std::string theme;

    void setClientModel(ClientModel *model);

private slots:
    void SummonBTCWallet(); 
    void SummonLTCWallet();
    void SummonDASHWallet();   
    void SummonBTCExplorer(); 
    void SummonLTCExplorer();
    void SummonDASHExplorer();
    void GetBids();
    void setNumBlocks(int count);
    int getNumBlocks();
    void Estimate();

    QString getDefaultDataDirectory();
    QString pathAppend(const QString& path1, const QString& path2);

private:
    Ui::BidPage *ui;
    ClientModel *clientModel;

};

#endif // BIDPAGE_H
