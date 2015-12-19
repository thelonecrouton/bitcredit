#include "miningpage.h"
#include "ui_miningpage.h"
#include "main.h"
#include "util.h"
#include "rpcserver.h"
#include "walletmodel.h"
#include "bignum.h"
#include <boost/thread.hpp>
#include <stdio.h>

extern json_spirit::Value GetNetworkHashPS(int lookup, int height);

MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage)
{
    ui->setupUi(this);

    int nThreads = boost::thread::hardware_concurrency();

    int nUseThreads = GetArg("-genproclimit", -1);
    if (nUseThreads < 0)
         nUseThreads = nThreads;

    ui->sliderCores->setMinimum(0);
    ui->sliderCores->setMaximum(nThreads);
    ui->sliderCores->setValue(nUseThreads);
    ui->labelNCores->setText(QString("%1").arg(nUseThreads));

    // setup Plot
    // create graph
    ui->diffplot_difficulty->addGraph();

    // Use usual background
    ui->diffplot_difficulty->setBackground(QBrush(QColor(255,255,255,255)));//QWidget::palette().color(this->backgroundRole())));

    // give the axes some labels:
    ui->diffplot_difficulty->xAxis->setLabel(tr("Blocks"));
    ui->diffplot_difficulty->yAxis->setLabel(tr("Difficulty"));

    // set the pens
    //a13469
    ui->diffplot_difficulty->graph(0)->setPen(QPen(QColor(161, 52, 105), 3, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));//QPen(QColor(76, 76, 229)));
    ui->diffplot_difficulty->graph(0)->setLineStyle(QCPGraph::lsLine);

    // set axes label fonts:
    QFont label = font();
    ui->diffplot_difficulty->xAxis->setLabelFont(label);
    ui->diffplot_difficulty->yAxis->setLabelFont(label);

    // setup Plot
    // create graph
    ui->diffplot_hashrate->addGraph();

    // Use usual background
    ui->diffplot_hashrate->setBackground(QBrush(QColor(255,255,255,255)));//QWidget::palette().color(this->backgroundRole())));

    // give the axes some labels:
    ui->diffplot_hashrate->xAxis->setLabel(tr("Blocks"));
    ui->diffplot_hashrate->yAxis->setLabel(tr("Hashrate H/s"));

    // set the pens
    //a13469, 6c3d94
    ui->diffplot_hashrate->graph(0)->setPen(QPen(QColor(108, 61, 148), 3, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));//QPen(QColor(76, 76, 229)));
    ui->diffplot_hashrate->graph(0)->setLineStyle(QCPGraph::lsLine);

    // set axes label fonts:
    QFont label2 = font();
    ui->diffplot_hashrate->xAxis->setLabelFont(label2);
    ui->diffplot_hashrate->yAxis->setLabelFont(label2);

    connect(ui->sliderCores, SIGNAL(valueChanged(int)), this, SLOT(changeNumberOfCores(int)));
    connect(ui->pushSwitchMining, SIGNAL(clicked()), this, SLOT(switchMining()));

    updateUI();
    startTimer(1500);
}

MiningPage::~MiningPage()
{
    delete ui;
}

