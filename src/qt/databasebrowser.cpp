#include "databasebrowser.h"
#include "optionsdialog.h"
#include "util.h"
#include "addressbookpage.h"
#include <QtWidgets>
#include <QVariant>
#include <QtSql>

Browser::Browser(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    table->addAction(insertRowAction);
    table->addAction(deleteRowAction);
    table->addAction(fieldStrategyAction);
    table->addAction(rowStrategyAction);
    table->addAction(manualStrategyAction);
    table->addAction(submitAction);
    table->addAction(revertAction);
    table->addAction(selectAction);

    if (QSqlDatabase::drivers().isEmpty())
        QMessageBox::information(this, tr("No database drivers found"),
                                 tr("This demo requires at least one Qt database driver. "
                                    "Please check the documentation how to build the "
                                    "Qt SQL plugins."));

		QString defaultdb = (GetDataDir() /"ratings/rawdata.db").string().c_str();
		QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","Default");
		db.setDatabaseName(defaultdb);
        if (!db.open())
            QMessageBox::warning(this, tr("Unable to open database"), tr("An error occurred while "
                                                                         "opening the connection: ") + db.lastError().text());
        //QSqlQuery q("", db);
        //q.exec("select * from RAWDATA");
        connectionWidget->refresh();

    if (QSqlDatabase::connectionNames().isEmpty())
        QMetaObject::invokeMethod(this, "addConnection", Qt::QueuedConnection);
        
    emit statusMessage(tr("Ready."));
}

Browser::~Browser()
{
	
}

void Browser::exec()
{

    QSqlQueryModel *model = new QSqlQueryModel(table);
    //QString b = "INSERT INTO table VALUE "% sqlEdit->toPlainText();
    model->setQuery(QSqlQuery(sqlEdit->toPlainText(), connectionWidget->currentDatabase()));
    table->setModel(model);

    if (model->lastError().type() != QSqlError::NoError)
        emit statusMessage(model->lastError().text());
    else if (model->query().isSelect())
        emit statusMessage(tr("Query OK."));
    else
        emit statusMessage(tr("Query OK, number of affected rows: %1").arg(
                           model->query().numRowsAffected()));

    updateActions();

}

QSqlError Browser::addConnection(const QString &driver, const QString &dbName, const QString &host,
                            const QString &user, const QString &passwd, int port)
{
    static int cCount = 0;

    QSqlError err;
    QSqlDatabase db = QSqlDatabase::addDatabase(driver, QString("Browser%1").arg(++cCount));
    db.setDatabaseName(dbName);
    db.setHostName(host);
    db.setPort(port);
    if (!db.open(user, passwd)) {
        err = db.lastError();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QString("Browser%1").arg(cCount));
    }
    connectionWidget->refresh();

    return err;
}

void Browser::addConnection()
{
    OptionsDialog dialog(this, !GetBoolArg("-disablewallet", false));
    if (dialog.exec() != QDialog::Accepted)
        return;

        QSqlError err = addConnection(dialog.driverName(), dialog.databaseName(), dialog.hostName(),
                           dialog.userName(), dialog.password(), dialog.port());
        if (err.type() != QSqlError::NoError)
            QMessageBox::warning(this, tr("Unable to open database"), tr("An error occurred while "
                                       "opening the connection: ") + err.text());  
                                   
}

void Browser::showTable(const QString &t)
{
    QSqlTableModel *model = new CustomModel(table, connectionWidget->currentDatabase());
    model->setEditStrategy(QSqlTableModel::OnRowChange);
    model->setTable(connectionWidget->currentDatabase().driver()->escapeIdentifier(t, QSqlDriver::TableName));
    model->select();
    if (model->lastError().type() != QSqlError::NoError)
        emit statusMessage(model->lastError().text());
    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);

    connect(table->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged()));
    updateActions();
}

void Browser::showMetaData(const QString &t)
{
    QSqlRecord rec = connectionWidget->currentDatabase().record(t);
    QStandardItemModel *model = new QStandardItemModel(table);

    model->insertRows(0, rec.count());
    model->insertColumns(0, 7);

    model->setHeaderData(0, Qt::Horizontal, "Fieldname");
    model->setHeaderData(1, Qt::Horizontal, "Type");
    model->setHeaderData(2, Qt::Horizontal, "Length");
    model->setHeaderData(3, Qt::Horizontal, "Precision");
    model->setHeaderData(4, Qt::Horizontal, "Required");
    model->setHeaderData(5, Qt::Horizontal, "AutoValue");
    model->setHeaderData(6, Qt::Horizontal, "DefaultValue");

    for (int i = 0; i < rec.count(); ++i) {
        QSqlField fld = rec.field(i);
        model->setData(model->index(i, 0), fld.name());
        model->setData(model->index(i, 1), fld.typeID() == -1
                ? QString(QMetaType::typeName(fld.type()))
                : QString("%1 (%2)").arg(QMetaType::typeName(fld.type())).arg(fld.typeID()));
        model->setData(model->index(i, 2), fld.length());
        model->setData(model->index(i, 3), fld.precision());
        model->setData(model->index(i, 4), fld.requiredStatus() == -1 ? QVariant("?")
                : QVariant(bool(fld.requiredStatus())));
        model->setData(model->index(i, 5), fld.isAutoValue());
        model->setData(model->index(i, 6), fld.defaultValue());
    }

    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    updateActions();
}

void Browser::insertRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(table->model());
    if (!model)
        return;

    QModelIndex insertIndex = table->currentIndex();
    int row = insertIndex.row() == -1 ? 0 : insertIndex.row();
    model->insertRow(row);
    insertIndex = model->index(row, 0);
    table->setCurrentIndex(insertIndex);
    table->edit(insertIndex);
}

void Browser::deleteRow()
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel *>(table->model());
    if (!model)
        return;

    QModelIndexList currentSelection = table->selectionModel()->selectedIndexes();
    for (int i = 0; i < currentSelection.count(); ++i) {
        if (currentSelection.at(i).column() != 0)
            continue;
        model->removeRow(currentSelection.at(i).row());
    }

    updateActions();
}

void Browser::updateActions()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    bool enableIns = tm;
    bool enableDel = enableIns && table->currentIndex().isValid();

    insertRowAction->setEnabled(enableIns);
    deleteRowAction->setEnabled(enableDel);

    fieldStrategyAction->setEnabled(tm);
    rowStrategyAction->setEnabled(tm);
    manualStrategyAction->setEnabled(tm);
    submitAction->setEnabled(tm);
    revertAction->setEnabled(tm);
    selectAction->setEnabled(tm);

    if (tm) {
        QSqlTableModel::EditStrategy es = tm->editStrategy();
        fieldStrategyAction->setChecked(es == QSqlTableModel::OnFieldChange);
        rowStrategyAction->setChecked(es == QSqlTableModel::OnRowChange);
        manualStrategyAction->setChecked(es == QSqlTableModel::OnManualSubmit);
    }
}

void Browser::on_fieldStrategyAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnFieldChange);
}

void Browser::on_rowStrategyAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnRowChange);
}

void Browser::on_manualStrategyAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->setEditStrategy(QSqlTableModel::OnManualSubmit);
}

void Browser::on_submitAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->submitAll();
}

void Browser::on_revertAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->revertAll();
}

void Browser::on_selectAction_triggered()
{
    QSqlTableModel * tm = qobject_cast<QSqlTableModel *>(table->model());
    if (tm)
        tm->select();
}

void Browser::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        sqlEdit->setText(dlg.getReturnValue());
    }
}

void Browser::on_submitButton_clicked()
{
   exec();
   sqlEdit->setFocus();
}

void Browser::on_clearButton_clicked()
{
    sqlEdit->clear();
    sqlEdit->setFocus();
}

void Browser::on_connectionWidget_metaDataRequested(const QString &table)
{
	showMetaData(table);
}

void Browser::on_connectionWidget_tableActivated(const QString &table)
{
	 showTable(table);
}

void Browser::on_insertRowAction_triggered()
{
	 insertRow();
}
    
void Browser::on_deleteRowAction_triggered()
{
	 deleteRow();
}
    
void Browser::currentChanged() 
{
	updateActions(); 
}

void Browser::setModel(WalletModel *model)
{
    this->model = model;
}
