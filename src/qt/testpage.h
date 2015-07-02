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
    //void setModel(ClientModel *model);


private:
    Ui::TestPage *ui;
    //ClientModel *model;

};

#endif // TESTPAGE_H
