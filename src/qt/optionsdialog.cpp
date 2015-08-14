// Copyright (c) 2011-2013 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcredit-config.h"
#endif

#include "optionsdialog.h"
#include "ui_optionsdialog.h"

#include "bitcreditunits.h"
#include "optionsmodel.h"
#include "darksend.h"

#include "main.h" // for MAX_SCRIPTCHECK_THREADS
#include "netbase.h"
#include "txdb.h" // for -dbcache defaults

#ifdef ENABLE_WALLET
#include "wallet.h" // for CWallet::minTxFee
#endif

#include "guiutil.h"
#include "util.h"
#include "intro.h"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QStringList>
#include <QModelIndex>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QStyle>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QDir>


OptionsDialog::OptionsDialog(QWidget *parent, bool enableWallet) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    model(0),
    mapper(0),
    fProxyIpsValid(true)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nOptionsDialogWindow", this->size(), this);

    /* Main elements init */
    ui->databaseCache->setMinimum(nMinDbCache);
    ui->databaseCache->setMaximum(nMaxDbCache);
    ui->threadsScriptVerif->setMinimum(-(int)boost::thread::hardware_concurrency());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->proxyIpTor->setEnabled(false);
    ui->proxyPortTor->setEnabled(false);
    ui->proxyPortTor->setValidator(new QIntValidator(1, 65535, this));
    
    //hide Darksend stuff
    ui->label->hide();
    ui->label_2->hide();
    ui->darksendRounds->hide();
    ui->anonymizeBitcredit->hide();
    
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyIp, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyPort, SLOT(setEnabled(bool)));
    
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyIpTor, SLOT(setEnabled(bool)));
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyPortTor, SLOT(setEnabled(bool)));
    
    connect(ui->checkOrange, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkDark, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkGreen, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkBlue, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkPink, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkPurple, SIGNAL(clicked()), this, SLOT(setTheme()));
    connect(ui->checkTurq, SIGNAL(clicked()), this, SLOT(setTheme()));
    
    ui->proxyIp->installEventFilter(this);
   
    /* Window elements init */
#ifdef Q_OS_MAC
    /* remove Window tab on Mac */
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWindow));
#endif

    /* remove Wallet tab in case of -disablewallet */
    if (!enableWallet) {
        ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWallet));
    }

    /* Display elements init */
    QDir translations(":translations");
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    foreach(const QString &langStr, translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language - country (locale name)", e.g. "German - Germany (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" - ") + QLocale::countryToString(locale.country()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
        else
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language (locale name)", e.g. "German (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
    }
#if QT_VERSION >= 0x040700
    ui->thirdPartyTxUrls->setPlaceholderText("https://example.com/tx/%s");
#endif

    ui->unit->setModel(new BitcreditUnits(this));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    /* setup/change UI elements when proxy IPs are invalid/valid */
    connect(this, SIGNAL(proxyIpChecks(QValidatedLineEdit *, int)), this, SLOT(doProxyIpChecks(QValidatedLineEdit *, int)));
}

OptionsDialog::~OptionsDialog()
{
    GUIUtil::saveWindowGeometry("nOptionsDialogWindow", this);
    delete ui;
}

QString OptionsDialog::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

void OptionsDialog::setTheme()
{
    if (this->ui->checkOrange->isChecked())
    {
        selected = "orange.qss";
    }
    else if (this->ui->checkDark->isChecked())
    {
        selected = "dark.qss";          
    }
    else if (this->ui->checkGreen->isChecked())
    {
        selected = "green.qss";        
    }
    else if (this->ui->checkBlue->isChecked())
    {
        selected = "blue.qss";           
    }
    else if (this->ui->checkPink->isChecked())
    {
        selected = "pink.qss";          
    }
    else if (this->ui->checkPurple->isChecked())
    {
        selected = "purple.qss";           
    }
    else if (this->ui->checkTurq->isChecked())
    {
        selected = "turq.qss";          
    } 
    else
    {
       // if we end up here something has gone wrong...
       QMessageBox::information(0, "Problem Encountered!", "Don't Panic!");
    }
    
    // get default datadir and set path to bitcredit.conf
    QString dataDir = getDefaultDataDirectory();
    QString path = QDir(dataDir).filePath("bitcredit.conf");
    //read bitcredit.conf into a stringlist
    QFile f(path);
    QStringList confList;
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(0, "Error opening file", f.errorString());
    }
    else
    {  
        while(!f.atEnd())
        {
            confList.append(f.readLine());
        }
        f.close();
    }
    // if theme line not present, append one
    QStringList theme = confList.filter(QRegExp("theme"));
    QString data = theme.join("");
    if (data == "")
    {
        //append 'theme=blah' line to file
        //QString themepath = QDir(dataDir).filePath(selected);
        QFile f(path);
        if (f.open(QIODevice::Append)) 
        {    
            QTextStream stream(&f);
            //stream << "theme=" + themepath << endl;
            stream << "theme=" + selected << endl;
        }
    }
    else
    // theme line already present, replace it
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite))
        {
            QMessageBox::information(0, "File problem!", file.errorString());
        }
        else
        //load file contents but skip the existing theme line
        {
            QString s;
            QTextStream t(&file);
            while(!t.atEnd())
            {
                QString line = t.readLine();
                if(!line.contains("theme"))
                    s.append(line + "\n");
            }
            //add chosen theme line
            QString themepath = QDir(dataDir).filePath(selected);
            //s.append("theme=" + themepath);
            s.append("theme=" + selected);
            file.resize(0);
            //rewrite file
            t << s;
            file.close();
        }
    }
    QMessageBox::information(0, "Configuration file changed!", "Restart wallet to apply changes.");
}    

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if(model)
    {
        /* check if client restart is needed and show persistent message */
        if (model->isRestartRequired())
            showRestartWarning(true);

        QString strLabel = model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui->overriddenByCommandLineLabel->setText(strLabel);

        mapper->setModel(model);
        setMapper();
        mapper->toFirst();
        
        updateDefaultProxyNets();
    }

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->databaseCache, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    connect(ui->threadsScriptVerif, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    /* Wallet */
    connect(ui->spendZeroConfChange, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Network */
    connect(ui->allowIncoming, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocksTor, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Display */
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning()));
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->bitcreditAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);
    mapper->addMapping(ui->advertisedBalance, OptionsModel::AdvertisedBalance);

    /* Wallet */
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);

    /* Network */
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(ui->allowIncoming, OptionsModel::Listen);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);
    
    mapper->addMapping(ui->connectSocksTor, OptionsModel::ProxyUseTor);
    mapper->addMapping(ui->proxyIpTor, OptionsModel::ProxyIPTor);
    mapper->addMapping(ui->proxyPortTor, OptionsModel::ProxyPortTor);

    /* Window */
#ifndef Q_OS_MAC
    mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);


    /* Darksend Rounds */
    mapper->addMapping(ui->darksendRounds, OptionsModel::DarksendRounds);
    mapper->addMapping(ui->anonymizeBitcredit, OptionsModel::anonymizeBitcreditAmount);

}

void OptionsDialog::enableOkButton()
{
    /* prevent enabling of the OK button when data modified, if there is an invalid proxy address present */
    if(fProxyIpsValid)
        setOkButtonState(true);
}

void OptionsDialog::disableOkButton()
{
    setOkButtonState(false);
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_resetButton_clicked()
{
    if(model)
    {
        // confirmation dialog
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm options reset"),
            tr("Client restart required to activate changes.") + "<br><br>" + tr("Client will be shutdown, do you want to proceed?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Cancel)
            return;

        /* reset all options and close GUI */
        model->Reset();
        QApplication::quit();
    }
}

void OptionsDialog::on_okButton_clicked()
{
    mapper->submit();
    darkSendPool.cachedNumBlocks = 0;
    accept();
    updateDefaultProxyNets();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::showRestartWarning(bool fPersistent)
{
    ui->statusLabel->setStyleSheet("QLabel { color: red; }");

    if(fPersistent)
    {
        ui->statusLabel->setText(tr("Client restart required to activate changes."));
    }
    else
    {
        ui->statusLabel->setText(tr("This change would require a client restart."));
        // clear non-persistent status label after 10 seconds
        // Todo: should perhaps be a class attribute, if we extend the use of statusLabel
        QTimer::singleShot(10000, this, SLOT(clearStatusLabel()));
    }
}

void OptionsDialog::clearStatusLabel()
{
    ui->statusLabel->clear();
}

void OptionsDialog::doProxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort)
{
    Q_UNUSED(nProxyPort);

    const std::string strAddrProxy = pUiProxyIp->text().toStdString();
    CService addrProxy;

    /* Check for a valid IPv4 / IPv6 address */
    if (!(fProxyIpsValid = LookupNumeric(pUiProxyIp->text().toStdString().c_str(), addrProxy)))
    {
        disableOkButton();
        pUiProxyIp->setValid(false);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
    else
    {
        enableOkButton();
        ui->statusLabel->clear();
    }
}

void OptionsDialog::updateDefaultProxyNets()
{
    proxyType proxy;
    std::string strProxy;
    QString strDefaultProxyGUI;

    GetProxy(NET_IPV4, proxy);
    strProxy = proxy.ToStringIP() + ":" + proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv4->setChecked(true) : ui->proxyReachIPv4->setChecked(false);

    GetProxy(NET_IPV6, proxy);
    strProxy = proxy.ToStringIP() + ":" + proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv6->setChecked(true) : ui->proxyReachIPv6->setChecked(false);

    GetProxy(NET_TOR, proxy);
    strProxy = proxy.ToStringIP() + ":" + proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachTor->setChecked(true) : ui->proxyReachTor->setChecked(false);
}

bool OptionsDialog::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::FocusOut)
    {
        if(object == ui->proxyIp)
        {
            emit proxyIpChecks(ui->proxyIp, ui->proxyPort->text().toInt());
        }
        else if(object == ui->proxyIpTor)
        {
            emit proxyIpChecks(ui->proxyIpTor, ui->proxyPortTor->text().toInt());
        }
    }
    return QDialog::eventFilter(object, event);
}
