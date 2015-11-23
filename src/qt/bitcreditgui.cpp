// Copyright (c) 2014-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcreditgui.h"

#include "bitcreditunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "networkstyle.h"
#include "notificator.h"
#include "openuridialog.h"
#include "optionsdialog.h"
#include "optionsmodel.h"
#include "sendmessagesdialog.h"
#include "messagemodel.h"
#include "messagepage.h"
#include "invoicepage.h"
#include "receiptpage.h"
#include "rpcconsole.h"
#include "utilitydialog.h"
#include "exchangebrowser.h"
#include "util.h"
#include "vanitygenpage.h"
#include "miningpage.h"
#include "blockexplorer.h"
#ifdef ENABLE_WALLET
#include "walletframe.h"
#include "walletmodel.h"
#include "stdio.h"
#include "bidpage.h"

#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include "init.h"
#include "ui_interface.h"
#include "util.h"
#include <iostream>
#include <QtGui>
#include <QAction>
#include <QApplication>
#include <QMainWindow>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QIcon>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif
#include <QStyle>
#include <QFont>
#include <QImage>
#include <QFontDatabase>

#include <iostream>

#include "databasebrowser.h"
#include "databaseconnectionwidget.h"


const QString BitcreditGUI::DEFAULT_WALLET = "~Default";

BitcreditGUI::BitcreditGUI(const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    clientModel(0),
    messageModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelEncryptionIcon(0),
    labelConnectionsIcon(0),
    labelBlocksIcon(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    appMenuBar(0),
    overviewAction(0),
    historyAction(0),
    quitAction(0),
    sendCoinsAction(0),
    usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    signMessageAction(0),
    verifyMessageAction(0),
    aboutAction(0),
    receiveCoinsAction(0),
    optionsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    openRPCConsoleAction(0),
    openAction(0),
    showHelpMessageAction(0),
    sendMessagesAction(0),
    trayIcon(0),
    trayIconMenu(0),
    notificator(0),
    rpcConsole(0),
    prevBlocks(0),
    spinnerFrame(0)
{
    setFixedSize(1000, 600);
    setWindowFlags(Qt::FramelessWindowHint);
    QFontDatabase::addApplicationFont(":/fonts/preg");
    QFontDatabase::addApplicationFont(":/fonts/pxbold");
    QFontDatabase::addApplicationFont(":/fonts/mohave");
    QString windowTitle = tr("Bitcredit ") + " - ";
#ifdef ENABLE_WALLET
    /* if compiled with wallet support, -disablewallet can still disable the wallet */
    enableWallet = !GetBoolArg("-disablewallet", false);
#else
    enableWallet = false;
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }
    windowTitle += " " + networkStyle->getTitleAddText();

    //if theme= line exists in bitcredit.conf, use it
    theme = GetArg("-theme", "");
    themestr = QString::fromUtf8(theme.c_str());
    if (mapArgs.count("-theme"))
    {
        QFile qss;
        //QMessageBox::information(0, QString("Warning!"), QString("You are about to load a custom theme:<br>" + str + " !<br><br>The Bitcredit developers accept no responsibility for<br>any resultant loss of client utility!"), QMessageBox::Ok);
        if (themestr.contains("orange"))
        {
            QFile qss(":/css/orange");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("dark"))
        {
            QFile qss(":/css/dark");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("green"))
        {
            QFile qss(":/css/green");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("blue"))
        {
            QFile qss(":/css/blue");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("pink"))
        {
            QFile qss(":/css/pink");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("purple"))
        {
            QFile qss(":/css/purple");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
        else if (themestr.contains("turq"))
        {
            QFile qss(":/css/turq");
            qss.open(QFile::ReadOnly);
            qApp->setStyleSheet(qss.readAll());
            qss.close();
        }
    }
    //if not, load the default theme
    else
    {
        QFile qss(":/css/orange");
        qss.open(QFile::ReadOnly);
        qApp->setStyleSheet(qss.readAll());
        qss.close();
    }

#ifndef Q_OS_MAC
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
#else
    MacDockIconHandler::instance()->setIcon(networkStyle->getAppIcon());
#endif
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(0);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(this);
        setCentralWidget(walletFrame);
       
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions(networkStyle);

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

     // Status bar notification icons
    unitDisplayControl = new UnitDisplayStatusBarControl();
    labelEncryptionIcon = new QLabel();
    labelEncryptionIcon->setObjectName("labelEncryptionIcon");
    labelConnectionsIcon = new QLabel();
    labelConnectionsIcon->setPixmap(QIcon(":/icons/connect_0").pixmap(STATUSBAR_ICONSIZE, 44));
    labelConnectionsIcon->setObjectName("labelConnectionsIcon");
    labelBlocksIcon = new QLabel();
    labelBlocksIcon->setPixmap(QIcon(":/icons/connecting").pixmap(STATUSBAR_ICONSIZE, 44)); //Initialize with 'searching' icon so people with slow connections see something
    labelBlocksIcon->setToolTip("Looking for more network connections");
    labelBlocksIcon->setObjectName("labelBlocksIcon");

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    addToolBarBreak(Qt::LeftToolBarArea);
    QToolBar *toolbar2 = addToolBar(tr("Toolbar"));
    addToolBar(Qt::BottomToolBarArea, toolbar2);
    toolbar2->setOrientation(Qt::Horizontal);
    toolbar2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar2->setMovable(false);
    toolbar2->setObjectName("toolbar2");
    toolbar2->setFixedHeight(40);
    toolbar2->setFixedWidth(1000);
    toolbar2->setIconSize(QSize(18, 18));

    QWidget* spacer2 = new QWidget();
    spacer2->setFixedWidth(90);
    toolbar2->addWidget(spacer2);
    spacer2->setObjectName("spacer2");
    toolbar2->addWidget(labelConnectionsIcon);

    QWidget* spacer3 = new QWidget();
    spacer3->setFixedWidth(30);
    toolbar2->addWidget(spacer3);
    spacer3->setObjectName("spacer3");
    toolbar2->addWidget(labelBlocksIcon);

    QWidget* spacer6 = new QWidget();
    spacer6->setFixedWidth(30);
    toolbar2->addWidget(spacer6);
    spacer6->setObjectName("spacer6");
    toolbar2->addWidget(labelEncryptionIcon);

    QWidget* spacer4 = new QWidget();
    spacer4->setFixedWidth(160);
    toolbar2->addWidget(spacer4);
    spacer4->setObjectName("spacer4");

    toolbar2->addAction(openAction);
    toolbar2->addAction(usedSendingAddressesAction);
    toolbar2->addAction(verifyMessageAction);
    toolbar2->addAction(encryptWalletAction);
    toolbar2->addAction(backupWalletAction);
    toolbar2->addAction(changePassphraseAction);
    toolbar2->addAction(paperWalletAction);
    toolbar2->addAction(openRPCConsoleAction);
    toolbar2->addAction(aboutAction);
    toolbar2->addAction(toggleHideAction);
    toolbar2->addAction(quitAction);

    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();
}

BitcreditGUI::~BitcreditGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::instance()->setMainWindow(NULL);
#endif

    delete rpcConsole;
}

void BitcreditGUI::createActions(const NetworkStyle *networkStyle)
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/null"), tr("&Overview"), this);
    overviewAction->setCheckable(true);

    dataAction = new QAction(QIcon(":/icons/null"), tr("&P2P Finance"), this);
    dataAction->setCheckable(true);

    historyAction = new QAction(QIcon(":/icons/null"), tr("&Overview"), this);
    historyAction->setCheckable(true);
    tabGroup->addAction(historyAction);

    QToolBar *toolbarsend = addToolBar(tr("Send"));
    QToolBar *toolbarsend2 = addToolBar(tr("Receive"));

    toolbarsend->setIconSize(QSize(200, 35));
    toolbarsend->setObjectName("toolbar3");
    toolbarsend2->setIconSize(QSize(200, 35));
    toolbarsend2->setObjectName("toolbar4");
    toolbarsend->setFixedSize(200, 35);
    toolbarsend2->setFixedSize(200, 35);
    QHBoxLayout *vbox4 = new QHBoxLayout();
    vbox4->setContentsMargins(0, 0, 0, 0);
    vbox4->setSpacing(0);
    vbox4->addWidget(toolbarsend2);
    vbox4->addWidget(toolbarsend);
    wId2 = new QWidget(this);
    wId2->setContentsMargins(0, 0, 0, 0);
    wId2->setFixedSize(400, 35);
    wId2->move(400, 9);
    wId2->setLayout(vbox4);
    wId2->setFocus();
    wId2->hide();

    QToolBar *toolbarmess = addToolBar(tr("Send Message"));
    QToolBar *toolbarmess2 = addToolBar(tr("Messages"));

    toolbarmess->setIconSize(QSize(200, 35));
    toolbarmess2->setIconSize(QSize(200, 35));
    toolbarmess->setObjectName("toolbar5");
    toolbarmess2->setObjectName("toolbar6");
    toolbarmess->setFixedSize(200, 35);
    toolbarmess2->setFixedSize(200, 35);
    QHBoxLayout *vbox3 = new QHBoxLayout();
    vbox3->setContentsMargins(0, 0, 0, 0);
    vbox3->setSpacing(0);
    vbox3->addWidget(toolbarmess2);
    vbox3->addWidget(toolbarmess);
    wId = new QWidget(this);
    wId->setContentsMargins(0, 0, 0, 0);
    wId->setFixedSize(400, 35);
    wId->move(400, 9);
    wId->setLayout(vbox3);
    wId->setFocus();
    wId->hide();

    QToolBar *toolbarrecinv = addToolBar(tr("Receipts"));
    QToolBar *toolbarrecinv2 = addToolBar(tr("Invoices"));

    toolbarrecinv->setIconSize(QSize(200, 35));
    toolbarrecinv2->setIconSize(QSize(200, 35));
    toolbarrecinv->setObjectName("toolbar7");
    toolbarrecinv2->setObjectName("toolbar8");
    toolbarrecinv->setFixedSize(200, 35);
    toolbarrecinv2->setFixedSize(200, 35);
    QHBoxLayout *vbox2 = new QHBoxLayout();
    vbox2->setContentsMargins(0, 0, 0, 0);
    vbox2->setSpacing(0);
    vbox2->addWidget(toolbarrecinv2);
    vbox2->addWidget(toolbarrecinv);
    wId3 = new QWidget(this);
    wId3->setContentsMargins(0, 0, 0, 0);
    wId3->setFixedSize(400, 35);
    wId3->move(400, 9);
    wId3->setLayout(vbox2);
    wId3->setFocus();
    wId3->hide();

    QToolBar *toolbarstats = addToolBar(tr("Block Browser"));
    QToolBar *toolbarstats2 = addToolBar(tr("Market"));
    QToolBar *toolbarstats3 = addToolBar(tr("Bank Statistics"));
    QToolBar *toolbarutils = addToolBar(tr("Mining"));
    QToolBar *toolbarutils2 = addToolBar(tr("Vanity"));
    QToolBar *toolbarutils3 = addToolBar(tr("Vote"));

    toolbarutils->setIconSize(QSize(100, 35));
    toolbarutils2->setIconSize(QSize(100, 35));
    toolbarutils3->setIconSize(QSize(100, 35));
    toolbarutils->setObjectName("toolbar12");
    toolbarutils2->setObjectName("toolbar13");
    toolbarutils2->setObjectName("toolbar14");
    toolbarutils->setFixedSize(100, 35);
    toolbarutils2->setFixedSize(100, 35);
    toolbarutils3->setFixedSize(100, 35);
    toolbarstats->setIconSize(QSize(100, 35));
    toolbarstats2->setIconSize(QSize(100, 35));
    toolbarstats3->setIconSize(QSize(100, 35));
    toolbarstats->setObjectName("toolbar9");
    toolbarstats2->setObjectName("toolbar10");
    toolbarstats3->setObjectName("toolbar11");
    toolbarstats->setFixedSize(100, 35);
    toolbarstats2->setFixedSize(100, 35);
    toolbarstats3->setFixedSize(100, 35);
    QHBoxLayout *vbox5 = new QHBoxLayout();
    vbox5->setContentsMargins(0, 0, 0, 0);
    vbox5->setSpacing(0);
    vbox5->addWidget(toolbarstats2);
    vbox5->addWidget(toolbarstats);
    vbox5->addWidget(toolbarstats3);
    vbox5->addWidget(toolbarutils2);
    vbox5->addWidget(toolbarutils);
    vbox5->addWidget(toolbarutils3);
    wId4 = new QWidget(this);
    wId4->setContentsMargins(0, 0, 0, 0);
    wId4->setFixedSize(600, 35);
    wId4->move(300, 9);
    wId4->setLayout(vbox5);
    wId4->setFocus();
    wId4->hide();

    vanityAction = new QAction(QIcon(":/icons/null"), tr("&Vanity"), this);
    vanityAction->setCheckable(true);
    toolbarutils2->addAction(vanityAction);
    tabGroup->addAction(vanityAction);

    miningAction = new QAction(QIcon(":/icons/null"), tr("&Mining"), this);
    miningAction->setCheckable(true);
    toolbarutils->addAction(miningAction);
    tabGroup->addAction(miningAction);

    blockAction = new QAction(QIcon(":/icons/null"), tr("&Block Crawler"), this);
    blockAction->setCheckable(true);
    toolbarstats2->addAction(blockAction);
    tabGroup->addAction(blockAction);

    exchangeAction = new QAction(QIcon(":/icons/null"), tr("&Market Data"), this);
    exchangeAction->setCheckable(true);
    toolbarstats->addAction(exchangeAction);
    tabGroup->addAction(exchangeAction); 
	
    bankstatsAction = new QAction(QIcon(":/icons/null"), tr("&Bank Statistics"), this);
    bankstatsAction->setCheckable(true);
    toolbarstats3->addAction(bankstatsAction);
    tabGroup->addAction(bankstatsAction);	

    sendCoinsAction = new QAction(QIcon(":/icons/null"), tr("&Send"), this);
    sendCoinsAction->setCheckable(true);
    toolbarsend2->addAction(sendCoinsAction);
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/null"), tr("&Receive"), this);
    receiveCoinsAction->setCheckable(true);
    toolbarsend->addAction(receiveCoinsAction);
    tabGroup->addAction(receiveCoinsAction);

    actionSendReceive = new QAction(QIcon(":/icons/null"), tr("&Send / Receive"), this);
    actionSendReceive->setCheckable(true);

    sendMessagesAction = new QAction(QIcon(":/icons/null"), tr("Send &Message"), this);
    sendMessagesAction->setCheckable(true);
    toolbarmess2->addAction(sendMessagesAction);
    tabGroup->addAction(sendMessagesAction);

    messageAction = new QAction(QIcon(":/icons/null"), tr("Mess&ages"), this);
    messageAction->setCheckable(true);
    toolbarmess->addAction(messageAction);    
    tabGroup->addAction(messageAction);
    
    actionSendReceiveMess = new QAction(QIcon(":/icons/null"), tr("&Messages"), this);
    actionSendReceiveMess->setCheckable(true); 

    invoiceAction = new QAction(QIcon(":/icons/null"), tr("&Invoices"), this);
    invoiceAction->setCheckable(true);
    toolbarrecinv->addAction(invoiceAction);
    tabGroup->addAction(invoiceAction);

    receiptAction = new QAction(QIcon(":/icons/null"), tr("&Receipts"), this);
    receiptAction->setCheckable(true);
    toolbarrecinv2->addAction(receiptAction);
    tabGroup->addAction(receiptAction);

    actionSendReceiveinv = new QAction(QIcon(":/icons/null"), tr("&Receipts / Invoices"), this);
    actionSendReceiveinv->setCheckable(true);

    actionSendReceivestats = new QAction(QIcon(":/icons/null"), tr("&Utilities"), this);
    actionSendReceivestats->setCheckable(true);

    bidAction = new QAction(QIcon(":/icons/null"), ("&Block Auction"), this);
    bidAction->setCheckable(true);
    tabGroup->addAction(bidAction);

    voteCoinsAction = new QAction(QIcon(":/icons/null"), tr("&Vote"), this);
    voteCoinsAction->setCheckable(true);
    toolbarutils3->addAction(voteCoinsAction);
    tabGroup->addAction(voteCoinsAction);

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(actionSendReceive, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(actionSendReceive, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(actionSendReceiveMess, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(actionSendReceiveMess, SIGNAL(triggered()), this, SLOT(gotoSendMessagesPage()));
    connect(actionSendReceiveinv, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(actionSendReceiveinv, SIGNAL(triggered()), this, SLOT(gotoInvoicesPage()));
    connect(actionSendReceivestats, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(actionSendReceivestats, SIGNAL(triggered()), this, SLOT(gotoExchangeBrowserPage()));

    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(dataAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(dataAction, SIGNAL(triggered()), this, SLOT(gotoDatabasePage()));
    connect(exchangeAction, SIGNAL(triggered()), this, SLOT(gotoExchangeBrowserPage()));
    connect(blockAction, SIGNAL(triggered()), this, SLOT(gotoBlockExplorerPage()));
    connect(blockAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(bankstatsAction, SIGNAL(triggered()), this, SLOT(gotoStatisticsPage()));
    connect(voteCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(voteCoinsAction, SIGNAL(triggered()), this, SLOT(gotoVoteCoinsPage()));
    connect(sendMessagesAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendMessagesAction, SIGNAL(triggered()), this, SLOT(gotoSendMessagesPage()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(gotoMessagesPage()));
    connect(invoiceAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(invoiceAction, SIGNAL(triggered()), this, SLOT(gotoInvoicesPage()));
    connect(receiptAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiptAction, SIGNAL(triggered()), this, SLOT(gotoReceiptPage()));
    connect(bidAction, SIGNAL(triggered()), this, SLOT(gotoBidPage()));
    connect(bidAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(vanityAction, SIGNAL(triggered()), this, SLOT(gotoVanityGenPage()));
    connect(vanityAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(miningAction, SIGNAL(triggered()), this, SLOT(gotoMiningPage()));
    connect(miningAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));

#endif // ENABLE_WALLET

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);

    //colour code according to theme
    if (themestr.contains("orange"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-orange"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("dark"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-orange"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("green"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-green"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("blue"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-blue"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("pink"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-pink"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("purple"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-purple"), tr("&About Bitcredit Core"), this);
    }
    else if (themestr.contains("turq"))
    {
        aboutAction = new QAction(QIcon(":/icons/about-turq"), tr("&About Bitcredit Core"), this);
    }
    else
    {
        aboutAction = new QAction(QIcon(":/icons/about"), tr("&About Bitcredit Core"), this);
    }
    aboutAction->setObjectName("aboutAction");
    aboutAction->setStatusTip(tr("Show information about Bitcredit Core"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/null"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for Bitcredit Core"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/mini"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Bitcredit addresses to prove you own them"));
    verifyMessageAction = new QAction(QIcon(":/icons/verify"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Bitcredit addresses"));
    paperWalletAction = new QAction(QIcon(":/icons/print"), tr("&Print paper wallets"), this);
    paperWalletAction->setStatusTip(tr("Print paper wallets"));

    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));

    usedSendingAddressesAction = new QAction(QIcon(":/icons/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(QIcon(":/icons/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(QIcon(":/icons/open"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a bitcredit: URI or payment request"));

    showHelpMessageAction = new QAction(QIcon(":/icons/info"), tr("&Command-line options"), this);
    showHelpMessageAction->setStatusTip(tr("Show the Bitcredit Core help message to get a list with possible Bitcredit command-line options"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
        connect(paperWalletAction, SIGNAL(triggered()), walletFrame, SLOT(printPaperWallets()));
    }
#endif
}

void BitcreditGUI::createToolBars()
{
    QLabel *labelLogo = new QLabel(this);
    //colour code according to theme
    //labelLogo->setPixmap(QPixmap(":images/head"));
    if (themestr.contains("orange"))
    {
        labelLogo->setPixmap(QPixmap(":images/head"));
    }
    else if (themestr.contains("dark"))
    {
        labelLogo->setPixmap(QPixmap(":images/head"));
    }
    else if (themestr.contains("green"))
    {
        labelLogo->setPixmap(QPixmap(":images/head-green"));
    }
    else if (themestr.contains("blue"))
    {
        labelLogo->setPixmap(QPixmap(":images/head-blue"));
    }
    else if (themestr.contains("pink"))
    {
        labelLogo->setPixmap(QPixmap(":images/head-pink"));
    }
    else if (themestr.contains("purple"))
    {
        labelLogo->setPixmap(QPixmap(":images/head-purple"));
    }
    else if (themestr.contains("turq"))
    {
        labelLogo->setPixmap(QPixmap(":images/head-turq"));
    }
    else
    {
        labelLogo->setPixmap(QPixmap(":images/head"));
    }
    labelLogo->show();
    QToolBar *toolbar = addToolBar(tr("Menu"));
    toolbar->setObjectName("toolbar");
    addToolBar(Qt::LeftToolBarArea, toolbar);
    toolbar->addWidget(labelLogo);
    toolbar->setOrientation(Qt::Vertical);
    toolbar->setFixedWidth(220);
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(50,20));


    if(walletFrame)
    {
        /*QWidget* spacer5 = new QWidget();
        spacer5->setFixedHeight(15);
        toolbar->addWidget(spacer5);
        spacer5->setObjectName("spacer5");*/
		toolbar->addAction(historyAction);
    	toolbar->addAction(actionSendReceive);
        toolbar->addAction(bidAction);
        toolbar->addAction(dataAction);
        toolbar->addAction(actionSendReceiveMess);
   		toolbar->addAction(actionSendReceiveinv);
		toolbar->addAction(actionSendReceivestats);
        overviewAction->setChecked(true);
        historyAction->setChecked(true);
    }

    toolbar->addAction(optionsAction);
}

void BitcreditGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(clientModel->getOptionsModel());
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
    }
}

#ifdef ENABLE_WALLET
bool BitcreditGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    setWalletActionsEnabled(true);
    return walletFrame->addWallet(name, walletModel);
}

bool BitcreditGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void BitcreditGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void BitcreditGUI::setWalletActionsEnabled(bool enabled)
{
    sendCoinsAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
    paperWalletAction->setEnabled(enabled);
    exchangeAction->setEnabled(enabled);
    sendMessagesAction->setEnabled(enabled);
	voteCoinsAction->setEnabled(enabled);
    messageAction->setEnabled(enabled);
    invoiceAction->setEnabled(enabled);
    receiptAction->setEnabled(enabled);
    bidAction->setEnabled(enabled);
    vanityAction->setEnabled(enabled);
    miningAction->setEnabled(enabled);
    dataAction->setEnabled(enabled);
}

void BitcreditGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("Bitcredit Core client") + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->show();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void BitcreditGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(voteCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void BitcreditGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#endif

void BitcreditGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcreditGUI::aboutClicked()
{
    if(!clientModel)
        return;

    QDialog* dlg = new HelpMessageDialog(this, true);
    //HelpMessageDialog dlg(this, true);
    dlg->setModal(false);
    dlg->show();
}

void BitcreditGUI::showHelpMessageClicked()
{
    HelpMessageDialog *help = new HelpMessageDialog(this, false);
    help->setAttribute(Qt::WA_DeleteOnClose);
    help->show();
}

#ifdef ENABLE_WALLET
void BitcreditGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        emit receivedURI(dlg.getURI());
    }
}

void BitcreditGUI::gotoMiningPage()
{
    miningAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoMiningPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoBlockExplorerPage()
{
    blockAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoBlockExplorerPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoDatabasePage()
{
    blockAction->setChecked(false);
    dataAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    if (walletFrame) walletFrame->gotoDatabasePage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoVanityGenPage()
{
    vanityAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoVanityGenPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoBidPage()
{
    bidAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoBidPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoSendMessagesPage()
{
    sendMessagesAction->setChecked(true);
    messageAction->setChecked(false);
    actionSendReceive->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoSendMessagesPage();

    wId2->hide();
    wId->show();
    wId->raise();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoMessagesPage()
{
    messageAction->setChecked(true);
    sendMessagesAction->setChecked(false);
    actionSendReceive->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoMessagesPage();

    wId2->hide();
    wId->show();
    wId->raise();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoInvoicesPage()
{
    invoiceAction->setChecked(true);
    receiptAction->setChecked(false);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoInvoicesPage();

    wId2->hide();
    wId->hide();
    wId3->show();
    wId3->raise();
    wId4->hide();
}

void BitcreditGUI::gotoReceiptPage()
{
    receiptAction->setChecked(true);
    invoiceAction->setChecked(false);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoReceiptPage();

    wId2->hide();
    wId->hide();
    wId3->show();
    wId3->raise();
    wId4->hide();
}

void BitcreditGUI::gotoExchangeBrowserPage()
{
    exchangeAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoExchangeBrowserPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->show();
    wId4->raise();
}

void BitcreditGUI::gotoStatisticsPage()
{
    bankstatsAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoStatisticsPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->show();
    wId4->raise();
}

void BitcreditGUI::gotoOverviewPage()
{
    //overviewAction->setChecked(true);
    historyAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoHistoryPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoHistoryPage();

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoReceiveCoinsPage()
{
    sendCoinsAction->setChecked(false);
    receiveCoinsAction->setChecked(true);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();

    wId2->show();
    wId2->raise();
    wId->hide();
    wId3->hide();
    wId4->hide();

}

void BitcreditGUI::gotoSendCoinsPage(QString addr)
{
    receiveCoinsAction->setChecked(false);
    sendCoinsAction->setChecked(true);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);

    wId2->show();
    wId2->raise();
    wId->hide();
    wId3->hide();
    wId4->hide();

}

void BitcreditGUI::gotoVoteCoinsPage(QString addr)
{
    actionSendReceive->setChecked(false);
    actionSendReceiveMess->setChecked(false);
    actionSendReceiveinv->setChecked(false);
    actionSendReceivestats->setChecked(false);
    dataAction->setChecked(false);
	if (walletFrame) walletFrame->gotoVoteCoinsPage(addr);

    wId2->hide();
    wId->hide();
    wId3->hide();
    wId4->hide();
}

void BitcreditGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcreditGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void BitcreditGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to Bitcredit network", "", count));
}

void BitcreditGUI::setNumBlocks(int count)
{
    if(!clientModel)
        return;

    //Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            progressBarLabel->setText(tr("Synchronizing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
            progressBarLabel->setText(tr("Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            progressBarLabel->setText(tr("No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = lastBlockDate.secsTo(currentDate);

    tooltip = tr("Processed %n blocks of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60*60;
        const int DAY_IN_SECONDS = 24*60*60;
        const int WEEK_IN_SECONDS = 7*24*60*60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if(secs < 2*DAY_IN_SECONDS)
        {
            timeBehindText = tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
        }
        else if(secs < 2*WEEK_IN_SECONDS)
        {
            timeBehindText = tr("%n day(s)","",secs/DAY_IN_SECONDS);
        }
        else if(secs < YEAR_IN_SECONDS)
        {
            timeBehindText = tr("%n week(s)","",secs/WEEK_IN_SECONDS);
        }
        else
        {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        progressBarLabel->setVisible(false);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
        progressBar->setVisible(false);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(QIcon(QString(
                ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(true);
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcreditGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("Bitcredit"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "Bitcredit - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        int r = mBox.exec();
        if (ret != NULL)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void BitcreditGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcreditGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            QApplication::quit();
        }
    }
#endif
    QMainWindow::closeEvent(event);
}

#ifdef ENABLE_WALLET
void BitcreditGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address)
{
    // On new transaction, make an info balloon
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             tr("Date: %1\n"
                "Amount: %2\n"
                "Type: %3\n"
                "Address: %4\n")
                  .arg(date)
                  .arg(BitcreditUnits::formatWithUnit(unit, amount, true))
                  .arg(type)
                  .arg(address), CClientUIInterface::MSG_INFORMATION);
}
#endif // ENABLE_WALLET

void BitcreditGUI::incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type)
{
    // Prevent balloon-spam when initial block download is in progress

    if (type == MessageTableEntry::Received)
    {
        notificator->notify(Notificator::Information,
                            tr("Incoming Message"),
                            tr("Date: %1\n"
                               "From Address: %2\n"
                               "To Address: %3\n"
                               "Message: %4\n")
                              .arg(sent_datetime)
                              .arg(from_address)
                              .arg(to_address)
                              .arg(message));
    };
}

void BitcreditGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcreditGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        foreach(const QUrl &uri, event->mimeData()->urls())
        {
            emit receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool BitcreditGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
        {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
        }
    {
        return QMainWindow::eventFilter(object, event);
    }
}

#ifdef ENABLE_WALLET
bool BitcreditGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void BitcreditGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}
#endif // ENABLE_WALLET

void BitcreditGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitcreditGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcreditGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void BitcreditGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

static bool ThreadSafeMessageBox(BitcreditGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

void BitcreditGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

void BitcreditGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl() :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu();
    foreach(BitcreditUnits::Unit u, BitcreditUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(BitcreditUnits::name(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu,SIGNAL(triggered(QAction*)),this,SLOT(onMenuSelection(QAction*)));
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *optionsModel)
{
    if (optionsModel)
    {
        this->optionsModel = optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setPixmap(QIcon(":/icons/unit_" + BitcreditUnits::id(newUnits)).pixmap(31,STATUSBAR_ICONSIZE));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}

void BitcreditGUI::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void BitcreditGUI::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX() - m_nMouseClick_X_Coordinate, event->globalY() - m_nMouseClick_Y_Coordinate);
}
