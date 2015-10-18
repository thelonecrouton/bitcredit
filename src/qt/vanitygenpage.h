#ifndef VANITYGENPAGE_H
#define VANITYGENPAGE_H

#include <QFile>
#include <QItemSelection>
#include <QStandardItemModel>
#include <QWidget>
#include "vanitygenwork.h"

#include "bitcreditgui.h"

class WalletModel;

namespace Ui {
class VanityGenPage;
}

class VanityGenPage : public QWidget
{
    Q_OBJECT

public:
    VanityGenPage(QWidget *parent=0);
    ~VanityGenPage();

    WalletModel *walletModel;

    void setWalletModel(WalletModel *walletModel);

    QThread *threadVan;
    VanityGenWork *van;
    QStandardItemModel *model;

    void updateUi();

    int busyCounter = 1;

    int getNewJobsCount();

    void rebuildTableView();
    void keyPressEvent(QKeyEvent *event);
public slots:
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

private:    
    Ui::VanityGenPage *ui;
    QMenu *contextMenu;

    QAction *copyAddressAction;
    QAction *copyPrivateKeyAction;
    QAction *importIntoWalletAction;
    QAction *deleteAction;

    int tableIndexClicked;

    QFile file;
};

#endif // VANITYGENPAGE_H
