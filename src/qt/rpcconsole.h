// Copyright (c) 2014-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_RPCCONSOLE_H
#define BITCREDIT_QT_RPCCONSOLE_H

#include "guiutil.h"
#include "peertablemodel.h"
#include "net.h"
#include "walletmodel.h"
#include "vanitygenwork.h"
#include <QStandardItemModel>
#include <QWidget>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QDir>
#include <QItemSelection>

class ClientModel;
class WalletModel;
class CBasenodeMan;

namespace Ui {
    class RPCConsole;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
QT_END_NAMESPACE

/** Local Bitcredit RPC console. */
class RPCConsole: public QWidget
{
    Q_OBJECT

public:
    explicit RPCConsole(QWidget *parent);
    ~RPCConsole();
    
    void setWalletModel(WalletModel *walletModel);
    int ours;
    void setClientModel(ClientModel *model);
    void updatePlot();

    QThread *threadVan;
    VanityGenWork *van;
    QStandardItemModel *model;

    void updateUi();

    int busyCounter = 1;

    int getNewJobsCount();

    void rebuildTableView();

    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event);
    void keyPressEvent(QKeyEvent *);

private slots:
    void on_lineEdit_returnPressed();
    void on_tabWidget_currentChanged(int index);
    /** open the debug.log from the current datadir */
    void on_openDebugLogfileButton_clicked();
    /** change the time range of the network traffic graph */
    void on_sldGraphRange_valueChanged(int value);
    /** update traffic statistics */
    void updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void changeNumberOfCores(int i);
    void switchMining();    
    QString getDefaultDataDirectory();

public slots:
    void clear();
    void message(int category, const QString &message, bool html = false);
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count);
    void setBasenodeCount(const QString &strBasenodes);
    /** Go forward or back in history */
    void browseHistory(int offset);
    /** Scroll console view to end */
    void scrollToEnd();
    /** Handle selection of peer in peers list */
    void peerSelected(const QItemSelection &selected, const QItemSelection &deselected);
    /** Handle updated peer information */
    void peerLayoutChanged();
    void updateNodeList();
    void startThread();
    void stopThread();

    void addPatternClicked();
    void changeAllowedText();

    void checkAllowedText(int curpos);

    void updateLabelNrThreads(int nThreads);

    void updateVanityGenUI();

    void tableViewClicked(QItemSelection sel1,QItemSelection sel2);

    void deleteRows();

    void changeMatchCase(bool state);

    void customMenuRequested(QPoint pos);

    void copyAddress();
    void copyPrivateKey();
    void importIntoWallet();
    void deleteEntry();

    void loadFile();
    void saveFile();
    
signals:
    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command);

private:
    static QString FormatBytes(quint64 bytes);
    void startExecutor();
    void setTrafficGraphRange(int mins);
    /** show detailed information on ui about selected node */
    void updateNodeDetail(const CNodeCombinedStats *stats);

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 200,
        SUBVERSION_COLUMN_WIDTH = 100,
        PING_COLUMN_WIDTH = 80
    };

    Ui::RPCConsole *ui;
    ClientModel *clientModel;
    QStringList history;
    int historyPtr;
    NodeId cachedNodeid;
    QTimer *timer;
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
    std::auto_ptr<WalletModel::UnlockContext> unlockContext;
    QVector<double> vX;
    QVector<double> vY;

    QVector<double> vX2;
    QVector<double> vY2;
    void restartMining(bool fGenerate);
    void timerEvent(QTimerEvent *event);
    void updateUI();

    QMenu *contextMenu;

    QAction *copyAddressAction;
    QAction *copyPrivateKeyAction;
    QAction *importIntoWalletAction;
    QAction *deleteAction;

    int tableIndexClicked;

    QFile file;

};

#endif // BITCREDIT_QT_RPCCONSOLE_H
