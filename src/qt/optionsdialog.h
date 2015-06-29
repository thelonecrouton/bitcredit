// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_OPTIONSDIALOG_H
#define BITCREDIT_QT_OPTIONSDIALOG_H

#include <QDialog>
#include <QTreeView>
#include <QPushButton>
#include <QFileSystemModel>
#include <QModelIndex>
#include <QLabel>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QRadioButton>


class OptionsModel;
class QValidatedLineEdit;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

namespace Ui {
class OptionsDialog;
}

/** Preferences dialog. */
class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent, bool enableWallet);
    ~OptionsDialog();

    void setModel(OptionsModel *model);
    void setMapper();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    /* enable OK button */
    void enableOkButton();
    /* disable OK button */
    void disableOkButton();
    /* set OK button state (enabled / disabled) */
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void setTheme();
    //void getData(const QModelIndex &index);
    void showRestartWarning(bool fPersistent = false);
    void clearStatusLabel();
    void doProxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort);
	/* query the networks, for which the default proxy is used */
    void updateDefaultProxyNets();
    
    QString getDefaultDataDirectory();
    
     
signals:
    void proxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort);

private:
    Ui::OptionsDialog *ui;
    OptionsModel *model;
    QDataWidgetMapper *mapper;
    bool fProxyIpsValid;
    QTreeView *tree;
    QPushButton *pushButton_apply_theme;
    QFileSystemModel *model2;
    QModelIndex *idx;
    QModelIndex *index;
    QLabel *test;
    QString selected;
    QString *homedir;
    QFile *qss;
    QString confFile;
    QString themename;
    QStringList *filters;
    QRadioButton *checkOrange;
    QRadioButton *checkDark;
    QRadioButton *checkGreen;
    QRadioButton *checkBlue;
    QRadioButton *checkPink;
    QRadioButton *checkPurple;
    QRadioButton *checkTurq;
    QString path;
    QString dataDir;
    QStringList confList;
    QStringList themepath;
    QString data;
    QString tempfile;
    QString tempstring;
    
    
};

#endif // BITCREDIT_QT_OPTIONSDIALOG_H
