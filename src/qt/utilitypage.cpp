#include "utilitypage.h"
#include "ui_utilitypage.h"

utilitypage::utilitypage(QWidget *parent) :
    QTabWidget(parent),
    ui(new Ui::utilitypage)
{
    ui->setupUi(this);
}

utilitypage::~utilitypage()
{
    delete ui;
}
