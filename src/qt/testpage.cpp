#include "testpage.h"
#include "ui_testpage.h"

TestPage::TestPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestPage)
{
    ui->setupUi(this);

}

/*
void ChatWindow ::setModel(ClientModel *model)
{
    this->model = model;
}
*/

TestPage::~TestPage()
{
    delete ui;

}
