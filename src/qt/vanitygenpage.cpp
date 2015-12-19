#include "vanitygenpage.h"
#include "ui_vanitygenpage.h"
#include "vanitygenwork.h"

#include "bitcreditgui.h"
#include "util.h"

#include "walletmodel.h"
#include "askpassphrasedialog.h"

#include <boost/thread.hpp>

#include <QClipboard>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTimer>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>

#include <QDebug>

VanityGenPage::VanityGenPage(QWidget *parent):
    QWidget(parent),
    walletModel(0),
    ui(new Ui::VanityGenPage)
{
    ui->setupUi(this);

    model = new QStandardItemModel(0,3,this);

    QStringList headerLabels;
    headerLabels << "Pattern" << "Privkey" << "Chance";
    model->setHorizontalHeaderLabels(headerLabels);

    ui->tableView->setModel(model);

    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->resizeSection(0,250);
    ui->tableView->horizontalHeader()->resizeSection(2,150);

    ui->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);//MultiSelection);

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(tableViewClicked(QItemSelection,QItemSelection)));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequested(QPoint)));

    ui->tableView->setFocusPolicy(Qt::StrongFocus);
    ui->tableView->installEventFilter(this);

    VanityGenKeysChecked = 0;
    VanityGenHashrate = 0;//"0.0";
    VanityGenNThreads = 0;
    VanityGenMatchCase = 0;

    //Input field:

    ui->lineEdit->setValidator(new QRegExpValidator(QRegExp("(6BCR){1,1}[1-9A-HJ-NP-Za-km-z]{3,3}"), NULL));
    ui->lineEdit->setMaxLength(16);

    connect(ui->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(changeAllowedText()));
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(addPatternClicked()));

    checkAllowedText(0);


    //"Add Pattern" - Buttton:

    connect(ui->buttonPattern, SIGNAL(clicked()), this, SLOT(addPatternClicked()));


    int nThreads = boost::thread::hardware_concurrency();
    int nUseThreads = GetArg("-genproclimit", -1);
    if (nUseThreads < 0)
        nUseThreads = nThreads;
    ui->horizontalSlider->setMaximum(nUseThreads);

    ui->checkBoxAutoImport->setEnabled(false);
    ui->buttonImport->setEnabled(false);
    ui->buttonDelete->setEnabled(false);

    connect(ui->checkBoxMatchCase, SIGNAL(clicked(bool)), this, SLOT(changeMatchCase(bool)));

    connect(ui->buttonDelete, SIGNAL(clicked(bool)),this, SLOT(deleteRows()));

    connect(ui->buttonImport, SIGNAL(clicked(bool)), this, SLOT(importIntoWallet()));

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(updateLabelNrThreads(int)));
    connect(ui->horizontalSlider, SIGNAL(sliderReleased()), this, SLOT(saveFile()));

    connect(ui->checkBoxAutoImport, SIGNAL(released()), this, SLOT(saveFile()));
    connect(ui->checkBoxShowPrivKeys, SIGNAL(released()), this, SLOT(saveFile()));


    connect(ui->buttonStart,SIGNAL(clicked()), this, SLOT(startThread()));

    copyAddressAction = new QAction("Copy Address", this);
    copyPrivateKeyAction = new QAction("Copy PrivateKey", this);
    importIntoWalletAction = new QAction("Import into Wallet", this);
    deleteAction = new QAction("Delete", this);

    contextMenu = new QMenu();
    contextMenu->addAction(importIntoWalletAction);
    contextMenu->addSeparator();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyPrivateKeyAction);
    contextMenu->addSeparator();
    contextMenu->addAction(deleteAction);

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyPrivateKeyAction, SIGNAL(triggered()), this, SLOT(copyPrivateKey()));
    connect(importIntoWalletAction, SIGNAL(triggered()), this, SLOT(importIntoWallet()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteEntry()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateVanityGenUI()));
    timer->start(250);

    updateUi();

    loadFile();

}



void VanityGenPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (walletModel->getEncryptionStatus() != WalletModel::Locked)
    updateUi();
}

