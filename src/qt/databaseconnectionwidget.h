#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QSqlDatabase>

class ConnectionWidget: public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionWidget(QWidget *parent = 0);
    ~ConnectionWidget();

    QSqlDatabase currentDatabase() const;

signals:
    void tableActivated(const QString &table);
    void metaDataRequested(const QString &tableName);

public slots:
    void refresh();
    void showMetaData();
    void on_tree_itemActivated(QTreeWidgetItem *item, int column);
    void on_tree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    void setActive(QTreeWidgetItem *);

    QTreeWidget *tree;
    QAction *metaDataAction;
    QString activeDb;
};

#endif
