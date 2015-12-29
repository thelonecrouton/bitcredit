#ifndef BROWSER_H
#define BROWSER_H

#include <QWidget>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include <QSlider>
#include "ui_databasebrowserwidget.h"

#include "walletmodel.h"

class WalletModel;
class ConnectionWidget;

class Browser: public QWidget, private Ui::Browser
{
    Q_OBJECT
public:
    Browser(QWidget *parent = 0);
    virtual ~Browser();

    QSqlError addConnection(const QString &driver, const QString &dbName, const QString &host,
                  const QString &user, const QString &passwd, int port = -1);

    void insertRow();
    void deleteRow();
    void updateActions();
    void setModel(WalletModel *model);

public slots:
    void exec();
    void showTable(const QString &table);
    void showMetaData(const QString &table);
    void addConnection();
    void currentChanged();


    void on_insertRowAction_triggered();
    void on_deleteRowAction_triggered();
    void on_fieldStrategyAction_triggered();
    void on_rowStrategyAction_triggered();
    void on_manualStrategyAction_triggered();
    void on_submitAction_triggered();
    void on_revertAction_triggered();
    void on_selectAction_triggered();
    void on_connectionWidget_tableActivated(const QString &table);
    void on_connectionWidget_metaDataRequested(const QString &table);
    void on_submitButton_clicked();
    void on_clearButton_clicked();
    void on_addressBookButton_clicked();

signals:
    void statusMessage(const QString &message);

private:

	WalletModel *model;

};

class CustomModel: public QSqlTableModel
{
    Q_OBJECT
public:
    explicit CustomModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase()):QSqlTableModel(parent, db) {}
    QVariant data(const QModelIndex &idx, int role) const Q_DECL_OVERRIDE
    {
        if (role == Qt::BackgroundRole && isDirty(idx))
            return QBrush(QColor(Qt::yellow));
        return QSqlTableModel::data(idx, role);
    }
};

#endif
