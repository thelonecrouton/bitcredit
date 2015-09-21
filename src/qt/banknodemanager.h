#ifndef BANKNODEMANAGER_H
#define BANKNODEMANAGER_H

#include "sync.h"

#include <QWidget>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QDir>

namespace Ui {
    class BanknodeManager;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class CBanknodeMan;
/** Banknode Manager page widget */
class BanknodeManager : public QWidget

{
    Q_OBJECT

public:
    explicit BanknodeManager(QWidget *parent = 0);
    ~BanknodeManager();
    
    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    int ours;

public slots:
    void updateNodeList();
    void updateAdrenalineNode(QString alias, QString addr, QString privkey, QString collateral);
    
    
signals:

private:
    QTimer *timer;
    Ui::BanknodeManager *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CCriticalSection cs_adrenaline;
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    QStringList myStringList;
    QString listitems;
    QFile *myTextFile;
    QString path;
    QString dataDir;
    std::string theme;
    QString themestring;
    QString ourcount;
    
private slots:
    void on_copyAddressButton_clicked();
    void on_createButton_clicked();
    void on_editButton_clicked();
    void on_getConfigButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_startAllButton_clicked();
    void on_stopAllButton_clicked();
    void on_removeButton_clicked();
    void on_tableWidget_2_itemSelectionChanged();
    
    QString getDefaultDataDirectory();
    
};

#endif // BANKNODEMANAGER_H
