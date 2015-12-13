#ifndef UTILITYPAGE_H
#define UTILITYPAGE_H

#include <QTabWidget>

namespace Ui {
class utilitypage;
}

class utilitypage : public QTabWidget
{
    Q_OBJECT

public:
    explicit utilitypage(QWidget *parent = 0);
    ~utilitypage();

private:
    Ui::utilitypage *ui;
};

#endif // UTILITYPAGE_H
