#ifndef TESTPAGE_H
#define TESTPAGE_H

#include "clientmodel.h"

#include <QWidget>

namespace Ui
{
    class TestPage;
}

class TestPage: public QWidget
{
    Q_OBJECT

public:
    TestPage(QWidget *parent = 0);
    ~TestPage();

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
    Ui::TestPage *ui;
    ClientModel *clientModel;

};

#endif // TESTPAGE_H
