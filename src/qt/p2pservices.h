#ifndef P2PSERVICES_H
#define P2PSERVICES_H

#include <QWidget>
#include <string>
namespace Ui {
class p2pservices;
}

class p2pservices : public QWidget
{
    Q_OBJECT

public:
    explicit p2pservices(QWidget *parent = 0);
    ~p2pservices();

public slots:
    //void on_addressBookButton_clicked();
private:
    Ui::p2pservices *ui;
    void gettrust();
};

#endif // P2PSERVICES_H
