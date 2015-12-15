// Copyright (c) 2014-2015 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcconsole.h"
#include "ui_rpcconsole.h"
#include "addeditadrenalinenode.h"
#include "adrenalinenodeconfigdialog.h"

#include "walletmodel.h"
#include "activebasenode.h"
#include "basenodeconfig.h"
#include "basenodeman.h"
#include "walletdb.h"
#include "wallet.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "peertablemodel.h"
#include "sync.h"
#include "init.h"
#include "stdio.h"
#include "util.h"
#include "guiutil.h"
#include "intro.h"
#include "bitcreditgui.h"
#include "main.h"
#include "chainparams.h"
#include "rpcserver.h"
#include "rpcclient.h"
#include "basenode.h"

#include <boost/filesystem.hpp>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <QScrollArea>
#include <QScroller>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

#include "json/json_spirit_value.h"

#include <openssl/crypto.h>

#ifdef ENABLE_WALLET
#include <db_cxx.h>
#endif

#include <QKeyEvent>
#include <QScrollBar>
#include <QThread>
#include <QTime>

#if QT_VERSION < 0x050000
#include <QUrl>
#endif

// TODO: add a scrollback limit, as there is currently none
// TODO: make it possible to filter out categories (esp debug messages when implemented)
// TODO: receive errors and debug messages through ClientModel

const int CONSOLE_HISTORY = 50;
const QSize ICON_SIZE(24, 24);

const int INITIAL_TRAFFIC_GRAPH_MINS = 30;

const struct {
    const char *url;
    const char *source;
} ICON_MAPPING[] = {
    {"cmd-request", ":/icons/tx_input"},
    {"cmd-reply", ":/icons/tx_output"},
    {"cmd-error", ":/icons/tx_output"},
    {"misc", ":/icons/tx_inout"},
    {NULL, NULL}
};

/* Object for executing console RPC commands in a separate thread.
*/
class RPCExecutor : public QObject
{
    Q_OBJECT

public slots:
    void request(const QString &command);

signals:
    void reply(int category, const QString &command);
};

#include "rpcconsole.moc"

/**
 * Split shell command line into a list of arguments. Aims to emulate \c bash and friends.
 *
 * - Arguments are delimited with whitespace
 * - Extra whitespace at the beginning and end and between arguments will be ignored
 * - Text can be "double" or 'single' quoted
 * - The backslash \c \ is used as escape character
 *   - Outside quotes, any character can be escaped
 *   - Within double quotes, only escape \c " and backslashes before a \c " or another backslash
 *   - Within single quotes, no escaping is possible and no special interpretation takes place
 *
 * @param[out]   args        Parsed arguments will be appended to this list
 * @param[in]    strCommand  Command line to split
 */
bool parseCommandLine(std::vector<std::string> &args, const std::string &strCommand)
{
    enum CmdParseState
    {
        STATE_EATING_SPACES,
        STATE_ARGUMENT,
        STATE_SINGLEQUOTED,
        STATE_DOUBLEQUOTED,
        STATE_ESCAPE_OUTER,
        STATE_ESCAPE_DOUBLEQUOTED
    } state = STATE_EATING_SPACES;
    std::string curarg;
    foreach(char ch, strCommand)
    {
        switch(state)
        {
        case STATE_ARGUMENT: // In or after argument
        case STATE_EATING_SPACES: // Handle runs of whitespace
            switch(ch)
            {
            case '"': state = STATE_DOUBLEQUOTED; break;
            case '\'': state = STATE_SINGLEQUOTED; break;
            case '\\': state = STATE_ESCAPE_OUTER; break;
            case ' ': case '\n': case '\t':
                if(state == STATE_ARGUMENT) // Space ends argument
                {
                    args.push_back(curarg);
                    curarg.clear();
                }
                state = STATE_EATING_SPACES;
                break;
            default: curarg += ch; state = STATE_ARGUMENT;
            }
            break;
        case STATE_SINGLEQUOTED: // Single-quoted string
            switch(ch)
            {
            case '\'': state = STATE_ARGUMENT; break;
            default: curarg += ch;
            }
            break;
        case STATE_DOUBLEQUOTED: // Double-quoted string
            switch(ch)
            {
            case '"': state = STATE_ARGUMENT; break;
            case '\\': state = STATE_ESCAPE_DOUBLEQUOTED; break;
            default: curarg += ch;
            }
            break;
        case STATE_ESCAPE_OUTER: // '\' outside quotes
            curarg += ch; state = STATE_ARGUMENT;
            break;
        case STATE_ESCAPE_DOUBLEQUOTED: // '\' in double-quoted text
            if(ch != '"' && ch != '\\') curarg += '\\'; // keep '\' for everything but the quote and '\' itself
            curarg += ch; state = STATE_DOUBLEQUOTED;
            break;
        }
    }
    switch(state) // final state
    {
    case STATE_EATING_SPACES:
        return true;
    case STATE_ARGUMENT:
        args.push_back(curarg);
        return true;
    default: // ERROR to end in one of the other states
        return false;
    }
}

void RPCExecutor::request(const QString &command)
{
    std::vector<std::string> args;
    if(!parseCommandLine(args, command.toStdString()))
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Parse error: unbalanced ' or \""));
        return;
    }
    if(args.empty())
        return; // Nothing to do
    try
    {
        std::string strPrint;
        // Convert argument list to JSON objects in method-dependent way,
        // and pass it along with the method name to the dispatcher.
        json_spirit::Value result = tableRPC.execute(
            args[0],
            RPCConvertValues(args[0], std::vector<std::string>(args.begin() + 1, args.end())));

        // Format result reply
        if (result.type() == json_spirit::null_type)
            strPrint = "";
        else if (result.type() == json_spirit::str_type)
            strPrint = result.get_str();
        else
            strPrint = write_string(result, true);

        emit reply(RPCConsole::CMD_REPLY, QString::fromStdString(strPrint));
    }
    catch (const json_spirit::Object& objError)
    {
        try // Nice formatting for standard-format error
        {
            int code = find_value(objError, "code").get_int();
            std::string message = find_value(objError, "message").get_str();
            emit reply(RPCConsole::CMD_ERROR, QString::fromStdString(message) + " (code " + QString::number(code) + ")");
        }
        catch (const std::runtime_error&) // raised when converting to invalid type, i.e. missing code or message
        {   // Show raw JSON object
            emit reply(RPCConsole::CMD_ERROR, QString::fromStdString(write_string(json_spirit::Value(objError), false)));
        }
    }
    catch (const std::exception& e)
    {
        emit reply(RPCConsole::CMD_ERROR, QString("Error: ") + QString::fromStdString(e.what()));
    }
}

RPCConsole::RPCConsole(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RPCConsole),
    clientModel(0),
    historyPtr(0),
    cachedNodeid(-1)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nRPCConsoleWindow", this->size(), this);

#ifndef Q_OS_MAC
    ui->openDebugLogfileButton->setIcon(QIcon(":/icons/export"));
#endif

    // Install event filter for up and down arrow
    ui->lineEdit->installEventFilter(this);
    ui->messagesWidget->installEventFilter(this);

    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(ui->btnClearTrafficGraph, SIGNAL(clicked()), ui->trafficGraph, SLOT(clear()));

    // set library version labels
    ui->openSSLVersion->setText(SSLeay_version(SSLEAY_VERSION));
#ifdef ENABLE_WALLET
    ui->berkeleyDBVersion->setText(DbEnv::version(0, 0, 0));
#else
    ui->label_berkeleyDBVersion->hide();
    ui->berkeleyDBVersion->hide();
#endif

    startExecutor();
    setTrafficGraphRange(INITIAL_TRAFFIC_GRAPH_MINS);

    ui->detailWidget->hide();
    ui->peerHeading->setText(tr("Select a peer to view detailed information."));

    ui->editButton->setEnabled(false);
    ui->getConfigButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->copyAddressButton->setEnabled(false);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    subscribeToCoreSignals();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    timer->start(30000);

    /*LOCK(cs_adrenaline);
    
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        updateAdrenalineNode(QString::fromStdString(adrenaline.second.sAlias), QString::fromStdString(adrenaline.second.sAddress), QString::fromStdString(adrenaline.second.sBasenodePrivKey), QString::fromStdString(adrenaline.second.sCollateralAddress));
    }*/

    updateNodeList();
    clear();
}

RPCConsole::~RPCConsole()
{
    GUIUtil::saveWindowGeometry("nRPCConsoleWindow", this);
    emit stopExecutor();
    delete ui;
}

bool RPCConsole::eventFilter(QObject* obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress) // Special key handling
    {
        QKeyEvent *keyevt = static_cast<QKeyEvent*>(event);
        int key = keyevt->key();
        Qt::KeyboardModifiers mod = keyevt->modifiers();
        switch(key)
        {
        case Qt::Key_Up: if(obj == ui->lineEdit) { browseHistory(-1); return true; } break;
        case Qt::Key_Down: if(obj == ui->lineEdit) { browseHistory(1); return true; } break;
        case Qt::Key_PageUp: /* pass paging keys to messages widget */
        case Qt::Key_PageDown:
            if(obj == ui->lineEdit)
            {
                QApplication::postEvent(ui->messagesWidget, new QKeyEvent(*keyevt));
                return true;
            }
            break;
        default:
            // Typing in messages widget brings focus to line edit, and redirects key there
            // Exclude most combinations and keys that emit no text, except paste shortcuts
            if(obj == ui->messagesWidget && (
                  (!mod && !keyevt->text().isEmpty() && key != Qt::Key_Tab) ||
                  ((mod & Qt::ControlModifier) && key == Qt::Key_V) ||
                  ((mod & Qt::ShiftModifier) && key == Qt::Key_Insert)))
            {
                ui->lineEdit->setFocus();
                QApplication::postEvent(ui->lineEdit, new QKeyEvent(*keyevt));
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void RPCConsole::setClientModel(ClientModel *model)
{
    clientModel = model;
    ui->trafficGraph->setClientModel(model);
    if(model)
    {
        // Keep up to date with client
        setNumConnections(model->getNumConnections());
        connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(model->getNumBlocks());
        connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        setBasenodeCount(model->getBasenodeCountString());
        connect(model, SIGNAL(strBasenodesChanged(QString)), this, SLOT(setBasenodeCount(QString)));

        updateTrafficStats(model->getTotalBytesRecv(), model->getTotalBytesSent());
        connect(model, SIGNAL(bytesChanged(quint64,quint64)), this, SLOT(updateTrafficStats(quint64, quint64)));

        // set up peer table
        ui->peerWidget->setModel(model->getPeerTableModel());
        ui->peerWidget->verticalHeader()->hide();
        ui->peerWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->peerWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->peerWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->peerWidget->setColumnWidth(PeerTableModel::Address, ADDRESS_COLUMN_WIDTH);
        ui->peerWidget->setColumnWidth(PeerTableModel::Subversion, SUBVERSION_COLUMN_WIDTH);
        ui->peerWidget->setColumnWidth(PeerTableModel::Ping, PING_COLUMN_WIDTH);

        // connect the peerWidget selection model to our peerSelected() handler
        connect(ui->peerWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
             this, SLOT(peerSelected(const QItemSelection &, const QItemSelection &)));
        connect(model->getPeerTableModel(), SIGNAL(layoutChanged()), this, SLOT(peerLayoutChanged()));

        // Provide initial values
        ui->clientVersion->setText(model->formatFullVersion());
        ui->clientName->setText(model->clientName());
        ui->buildDate->setText(model->formatBuildDate());
        ui->startupTime->setText(model->formatClientStartupTime());

        ui->networkName->setText(QString::fromStdString(Params().NetworkIDString()));
    }
}

static QString categoryClass(int category)
{
    switch(category)
    {
    case RPCConsole::CMD_REQUEST:  return "cmd-request"; break;
    case RPCConsole::CMD_REPLY:    return "cmd-reply"; break;
    case RPCConsole::CMD_ERROR:    return "cmd-error"; break;
    default:                       return "misc";
    }
}

void RPCConsole::clear()
{
    ui->messagesWidget->clear();
    history.clear();
    historyPtr = 0;
    ui->lineEdit->clear();
    ui->lineEdit->setFocus();

    // Add smoothly scaled icon images.
    // (when using width/height on an img, Qt uses nearest instead of linear interpolation)
    for(int i=0; ICON_MAPPING[i].url; ++i)
    {
        ui->messagesWidget->document()->addResource(
                    QTextDocument::ImageResource,
                    QUrl(ICON_MAPPING[i].url),
                    QImage(ICON_MAPPING[i].source).scaled(ICON_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    // Set default style sheet
    ui->messagesWidget->document()->setDefaultStyleSheet(
                "table { }"
                "td.time { color: #808080; padding-top: 3px; } "
                "td.message { font-family: monospace; font-size: 12px; } " // Todo: Remove fixed font-size
                "td.cmd-request { color: #006060; } "
                "td.cmd-error { color: red; } "
                "b { color: #006060; } "
                );

    message(CMD_REPLY, (tr("Welcome to the Bitcredit Core RPC console.") + "<br>" +
                        tr("Use up and down arrows to navigate history, and <b>Ctrl-L</b> to clear screen.") + "<br>" +
                        tr("Type <b>help</b> for an overview of available commands.")), true);
}

void RPCConsole::keyPressEvent(QKeyEvent *event)
{
    if(windowType() != Qt::Widget && event->key() == Qt::Key_Escape)
    {
        close();
    }
}

void RPCConsole::message(int category, const QString &message, bool html)
{
    QTime time = QTime::currentTime();
    QString timeString = time.toString();
    QString out;
    out += "<table><tr><td class=\"time\" width=\"65\">" + timeString + "</td>";
    out += "<td class=\"icon\" width=\"32\"><img src=\"" + categoryClass(category) + "\"></td>";
    out += "<td class=\"message " + categoryClass(category) + "\" valign=\"middle\">";
    if(html)
        out += message;
    else
        out += GUIUtil::HtmlEscape(message, true);
    out += "</td></tr></table>";
    ui->messagesWidget->append(out);
}

void RPCConsole::setNumConnections(int count)
{
    if (!clientModel)
        return;

    QString connections = QString::number(count) + " (";
    connections += tr("In:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_IN)) + " / ";
    connections += tr("Out:") + " " + QString::number(clientModel->getNumConnections(CONNECTIONS_OUT)) + ")";

    ui->numberOfConnections->setText(connections);
}

void RPCConsole::setNumBlocks(int count)
{
    ui->numberOfBlocks->setText(QString::number(count));
    if(clientModel)
        ui->lastBlockTime->setText(clientModel->getLastBlockDate().toString());
}

void RPCConsole::setBasenodeCount(const QString &strBasenodes)
{
    ui->basenodeCount->setText(strBasenodes);
}

void RPCConsole::on_lineEdit_returnPressed()
{
    QString cmd = ui->lineEdit->text();
    ui->lineEdit->clear();

    if(!cmd.isEmpty())
    {
        message(CMD_REQUEST, cmd);
        emit cmdRequest(cmd);
        // Remove command, if already in history
        history.removeOne(cmd);
        // Append command to history
        history.append(cmd);
        // Enforce maximum history size
        while(history.size() > CONSOLE_HISTORY)
            history.removeFirst();
        // Set pointer to end of history
        historyPtr = history.size();
        // Scroll console view to end
        scrollToEnd();
    }
}

void RPCConsole::browseHistory(int offset)
{
    historyPtr += offset;
    if(historyPtr < 0)
        historyPtr = 0;
    if(historyPtr > history.size())
        historyPtr = history.size();
    QString cmd;
    if(historyPtr < history.size())
        cmd = history.at(historyPtr);
    ui->lineEdit->setText(cmd);
}

void RPCConsole::startExecutor()
{
    QThread *thread = new QThread;
    RPCExecutor *executor = new RPCExecutor();
    executor->moveToThread(thread);

    // Replies from executor object must go to this object
    connect(executor, SIGNAL(reply(int,QString)), this, SLOT(message(int,QString)));
    // Requests from this object must go to executor
    connect(this, SIGNAL(cmdRequest(QString)), executor, SLOT(request(QString)));

    // On stopExecutor signal
    // - queue executor for deletion (in execution thread)
    // - quit the Qt event loop in the execution thread
    connect(this, SIGNAL(stopExecutor()), executor, SLOT(deleteLater()));
    connect(this, SIGNAL(stopExecutor()), thread, SLOT(quit()));
    // Queue the thread for deletion (in this thread) when it is finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    // Default implementation of QThread::run() simply spins up an event loop in the thread,
    // which is what we want.
    thread->start();
}

void RPCConsole::on_tabWidget_currentChanged(int index)
{
    if(ui->tabWidget->widget(index) == ui->tab_console)
    {
        ui->lineEdit->setFocus();
    }
}

void RPCConsole::on_openDebugLogfileButton_clicked()
{
    GUIUtil::openDebugLogfile();
}

void RPCConsole::scrollToEnd()
{
    QScrollBar *scrollbar = ui->messagesWidget->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void RPCConsole::on_sldGraphRange_valueChanged(int value)
{
    const int multiplier = 5; // each position on the slider represents 5 min
    int mins = value * multiplier;
    setTrafficGraphRange(mins);
}

QString RPCConsole::FormatBytes(quint64 bytes)
{
    if(bytes < 1024)
        return QString(tr("%1 B")).arg(bytes);
    if(bytes < 1024 * 1024)
        return QString(tr("%1 KB")).arg(bytes / 1024);
    if(bytes < 1024 * 1024 * 1024)
        return QString(tr("%1 MB")).arg(bytes / 1024 / 1024);

    return QString(tr("%1 GB")).arg(bytes / 1024 / 1024 / 1024);
}

void RPCConsole::setTrafficGraphRange(int mins)
{
    ui->trafficGraph->setGraphRangeMins(mins);
    ui->lblGraphRange->setText(GUIUtil::formatDurationStr(mins * 60));
}

void RPCConsole::updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut)
{
    ui->lblBytesIn->setText(FormatBytes(totalBytesIn));
    ui->lblBytesOut->setText(FormatBytes(totalBytesOut));
}

void RPCConsole::peerSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (!clientModel || selected.indexes().isEmpty())
        return;

    const CNodeCombinedStats *stats = clientModel->getPeerTableModel()->getNodeStats(selected.indexes().first().row());
    if (stats)
        updateNodeDetail(stats);
}

void RPCConsole::peerLayoutChanged()
{
    if (!clientModel)
        return;

    const CNodeCombinedStats *stats = NULL;
    bool fUnselect = false;
    bool fReselect = false;

    if (cachedNodeid == -1) // no node selected yet
        return;

    // find the currently selected row
    int selectedRow;
    QModelIndexList selectedModelIndex = ui->peerWidget->selectionModel()->selectedIndexes();
    if (selectedModelIndex.isEmpty())
        selectedRow = -1;
    else
        selectedRow = selectedModelIndex.first().row();

    // check if our detail node has a row in the table (it may not necessarily
    // be at selectedRow since its position can change after a layout change)
    int detailNodeRow = clientModel->getPeerTableModel()->getRowByNodeId(cachedNodeid);

    if (detailNodeRow < 0)
    {
        // detail node dissapeared from table (node disconnected)
        fUnselect = true;
        cachedNodeid = -1;
        ui->detailWidget->hide();
        ui->peerHeading->setText(tr("Select a peer to view detailed information."));
    }
    else
    {
        if (detailNodeRow != selectedRow)
        {
            // detail node moved position
            fUnselect = true;
            fReselect = true;
        }

        // get fresh stats on the detail node.
        stats = clientModel->getPeerTableModel()->getNodeStats(detailNodeRow);
    }

    if (fUnselect && selectedRow >= 0)
    {
        ui->peerWidget->selectionModel()->select(QItemSelection(selectedModelIndex.first(), selectedModelIndex.last()),
            QItemSelectionModel::Deselect);
    }

    if (fReselect)
    {
        ui->peerWidget->selectRow(detailNodeRow);
    }

    if (stats)
        updateNodeDetail(stats);
}

void RPCConsole::updateNodeDetail(const CNodeCombinedStats *stats)
{
    // Update cached nodeid
    cachedNodeid = stats->nodeStats.nodeid;

    // update the detail ui with latest node information
    QString peerAddrDetails(QString::fromStdString(stats->nodeStats.addrName));
    if (!stats->nodeStats.addrLocal.empty())
        peerAddrDetails += "<br />" + tr("via %1").arg(QString::fromStdString(stats->nodeStats.addrLocal));
    ui->peerHeading->setText(peerAddrDetails);
    ui->peerServices->setText(GUIUtil::formatServicesStr(stats->nodeStats.nServices));
    ui->peerLastSend->setText(stats->nodeStats.nLastSend ? GUIUtil::formatDurationStr(GetTime() - stats->nodeStats.nLastSend) : tr("never"));
    ui->peerLastRecv->setText(stats->nodeStats.nLastRecv ? GUIUtil::formatDurationStr(GetTime() - stats->nodeStats.nLastRecv) : tr("never"));
    ui->peerBytesSent->setText(FormatBytes(stats->nodeStats.nSendBytes));
    ui->peerBytesRecv->setText(FormatBytes(stats->nodeStats.nRecvBytes));
    ui->peerConnTime->setText(GUIUtil::formatDurationStr(GetTime() - stats->nodeStats.nTimeConnected));
    ui->peerPingTime->setText(GUIUtil::formatPingTime(stats->nodeStats.dPingTime));
    ui->peerVersion->setText(QString("%1").arg(stats->nodeStats.nVersion));
    ui->peerSubversion->setText(QString::fromStdString(stats->nodeStats.cleanSubVer));
    ui->peerDirection->setText(stats->nodeStats.fInbound ? tr("Inbound") : tr("Outbound"));
    ui->peerHeight->setText(QString("%1").arg(stats->nodeStats.nStartingHeight));

    // This check fails for example if the lock was busy and
    // nodeStateStats couldn't be fetched.
    if (stats->fNodeStateStatsAvailable) {
        // Ban score is init to 0
        ui->peerBanScore->setText(QString("%1").arg(stats->nodeStateStats.nMisbehavior));

        // Sync height is init to -1
        if (stats->nodeStateStats.nSyncHeight > -1)
            ui->peerSyncHeight->setText(QString("%1").arg(stats->nodeStateStats.nSyncHeight));
        else
            ui->peerSyncHeight->setText(tr("Unknown"));
    } else {
        ui->peerBanScore->setText(tr("Fetching..."));
        ui->peerSyncHeight->setText(tr("Fetching..."));
    }

    ui->detailWidget->show();
}

void RPCConsole::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void RPCConsole::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!clientModel)
        return;

    // start PeerTableModel auto refresh
    clientModel->getPeerTableModel()->startAutoRefresh();
}

void RPCConsole::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    if (!clientModel)
        return;

    // stop PeerTableModel auto refresh
    clientModel->getPeerTableModel()->stopAutoRefresh();
}

static void NotifyAdrenalineNodeUpdated(RPCConsole *page, CAdrenalineNodeConfig nodeConfig)
{
    // alias, address, privkey, collateral address
    QString alias = QString::fromStdString(nodeConfig.sAlias);
    QString addr = QString::fromStdString(nodeConfig.sAddress);
    QString privkey = QString::fromStdString(nodeConfig.sBasenodePrivKey);
    QString collateral = QString::fromStdString(nodeConfig.sCollateralAddress);
    
    QMetaObject::invokeMethod(page, "updateAdrenalineNode", Qt::QueuedConnection,
                              Q_ARG(QString, alias),
                              Q_ARG(QString, addr),
                              Q_ARG(QString, privkey),
                              Q_ARG(QString, collateral)
                              );
}

void RPCConsole::subscribeToCoreSignals()
{
    // Connect signals to core
    uiInterface.NotifyAdrenalineNodeChanged.connect(boost::bind(&NotifyAdrenalineNodeUpdated, this, _1));
}

void RPCConsole::unsubscribeFromCoreSignals()
{
    // Disconnect signals from core
    uiInterface.NotifyAdrenalineNodeChanged.disconnect(boost::bind(&NotifyAdrenalineNodeUpdated, this, _1));
}

void RPCConsole::on_tableWidget_2_itemSelectionChanged()
{
    if(ui->tableWidget_2->selectedItems().count() > 0)
    {
        ui->editButton->setEnabled(true);
        ui->getConfigButton->setEnabled(true);
        ui->startButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
	ui->copyAddressButton->setEnabled(true);
    }
}

void RPCConsole::updateAdrenalineNode(QString alias, QString addr, QString privkey, QString collateral)
{
    LOCK(cs_adrenaline);
    bool bFound = false;
    int nodeRow = 0;
    for(int i=0; i < ui->tableWidget_2->rowCount(); i++)
    {
        if(ui->tableWidget_2->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound)
        ui->tableWidget_2->insertRow(0);

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    QTableWidgetItem *statusItem = new QTableWidgetItem("");
    QTableWidgetItem *collateralItem = new QTableWidgetItem(collateral);

    ui->tableWidget_2->setItem(nodeRow, 0, aliasItem);
    ui->tableWidget_2->setItem(nodeRow, 1, addrItem);
    ui->tableWidget_2->setItem(nodeRow, 2, statusItem);
    ui->tableWidget_2->setItem(nodeRow, 3, collateralItem);
}

static QString seconds_to_DHMS(quint32 duration)
{
  QString res;
  int seconds = (int) (duration % 60);
  duration /= 60;
  int minutes = (int) (duration % 60);
  duration /= 60;
  int hours = (int) (duration % 24);
  int days = (int) (duration / 24);
  if((hours == 0)&&(days == 0))
      return res.sprintf("%02dm:%02ds", minutes, seconds);
  if (days == 0)
      return res.sprintf("%02dh:%02dm:%02ds", hours, minutes, seconds);
  return res.sprintf("%dd %02dh:%02dm:%02ds", days, hours, minutes, seconds);
}

void RPCConsole::updateNodeList()
{
    TRY_LOCK(cs_basenodepayments, lockBasenodes);
    if(!lockBasenodes)
        return;

    ui->countLabel->setText("Updating...");
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    BOOST_FOREACH(CBasenode mn, mnodeman.vBasenodes) 
    {
        int mnRow = 0;
        ui->tableWidget->insertRow(0);

 	// populate list
	// Address, Rank, Active, Active Seconds, Last Seen, Pub Key
	QTableWidgetItem *activeItem = new QTableWidgetItem(QString::number(mn.IsEnabled()));
	QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
	QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(mnodeman.GetBasenodeRank(mn.vin, chainActive.Tip()->nHeight)));
	QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(seconds_to_DHMS((qint64)(mn.lastTimeSeen - mn.sigTime)));
	QTableWidgetItem *lastSeenItem = new QTableWidgetItem(seconds_to_DHMS((qint64)(GetTime()-mn.lastTimeSeen)));

	
	CScript pubkey;
        pubkey =GetScriptForDestination(mn.pubkey.GetID());
        CTxDestination address1;
        ExtractDestination(pubkey, address1);
        CBitcreditAddress address2(address1);
	QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(address2.ToString()));
	
	ui->tableWidget->setItem(mnRow, 0, addressItem);
	ui->tableWidget->setItem(mnRow, 1, rankItem);
	ui->tableWidget->setItem(mnRow, 2, activeItem);
	ui->tableWidget->setItem(mnRow, 3, activeSecondsItem);
	ui->tableWidget->setItem(mnRow, 4, lastSeenItem);
	ui->tableWidget->setItem(mnRow, 5, pubkeyItem);
    }

    
    // get default datadir and set path to mybasenodes.txt
    QString dataDir = getDefaultDataDirectory();
    QString path = QDir(dataDir).filePath("mybasenodes.txt");
        
    theme = GetArg("-theme", "");
    themestring = QString::fromUtf8(theme.c_str());  

    //check if file exists
    QFileInfo checkFile(path);
    if (checkFile.exists() && checkFile.isFile()) 
    {
        QFile myTextFile(path);
        QStringList myStringList;
        if (!myTextFile.open(QIODevice::ReadOnly))
            {
                QMessageBox::information(0, "Error opening file", myTextFile.errorString());
            }
        else
            {  
                while(!myTextFile.atEnd())
                {
                    myStringList.append(myTextFile.readLine());
                }
                myTextFile.close();
            }
        QString listitems = myStringList.join(""); 
        
        //search for pubkeys that match those in our list
        //ser our own count
        ours = 0;
        int rows = ui->tableWidget->rowCount();
        if (rows >1)
        for(int i = 0; i < rows; ++i)
        {
            QTableWidgetItem *temp = ui->tableWidget->item(i, 5);
            QString str1 = temp->text();
            //if showMineOnlychecked, hide everything else
            if ((!listitems.contains(str1)) && ui->showMineOnly->isChecked())
            {
            	ui->tableWidget->setRowHidden(i, true);
            	
            	
            }	
            //else just change background colour
            else if (listitems.contains(str1))
            {
                //highlight according to stylesheet
                if (themestring.contains("orange"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
                else if (themestring.contains("dark"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
                else if (themestring.contains("green"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#45f806");      
                }
                else if (themestring.contains("blue"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#088af8");         
                }
                else if (themestring.contains("pink"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#fb04db");        
                }
                else if (themestring.contains("purple"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#cb03d2");            
                }
                else if (themestring.contains("turq"))
                {
                    ui->tableWidget->item(i, 5)->setBackgroundColor("#0ab4dc");          
                } 
                //fallback on default
                else
                {    
                ui->tableWidget->item(i, 5)->setBackgroundColor("#ffa405");
                }
            ours += 1;
            }
            
        }
        QString ourcount = QString::number(ours);
        ui->countLabel->setText(QString::number(ui->tableWidget->rowCount()) + "   (Mine: " + ourcount + ")");    
    }
    else
    {
         ui->countLabel->setText(QString::number(ui->tableWidget->rowCount()));
    }
}

QString RPCConsole::getDefaultDataDirectory()
{
    return GUIUtil::boostPathToQString(GetDefaultDataDir());
}

void RPCConsole::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
    }

}

void RPCConsole::on_createButton_clicked()
{
    AddEditAdrenalineNode* aenode = new AddEditAdrenalineNode();
    aenode->exec();
}

void RPCConsole::on_copyAddressButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sCollateralAddress = ui->tableWidget_2->item(r, 3)->text().toStdString();

    QApplication::clipboard()->setText(QString::fromStdString(sCollateralAddress));
}

void RPCConsole::on_editButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    // get existing config entry
}

void RPCConsole::on_getConfigButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];
    std::string sPrivKey = c.sBasenodePrivKey;
    AdrenalineNodeConfigDialog* d = new AdrenalineNodeConfigDialog(this, QString::fromStdString(sAddress), QString::fromStdString(sPrivKey));
    d->exec();
}

void RPCConsole::on_removeButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this, "Delete Basenode?", "Are you sure you want to delete this Basenode configuration?", QMessageBox::Yes|QMessageBox::No);

    if(confirm == QMessageBox::Yes)
    {
        QModelIndex index = selected.at(0);
        int r = index.row();
        std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
        CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];
        CWalletDB walletdb(pwalletMain->strWalletFile);
        pwalletMain->mapMyAdrenalineNodes.erase(sAddress);
        walletdb.EraseAdrenalineNodeConfig(c.sAddress);
        ui->tableWidget_2->clearContents();
        ui->tableWidget_2->setRowCount(0);
        BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
        {
            updateAdrenalineNode(QString::fromStdString(adrenaline.second.sAlias), QString::fromStdString(adrenaline.second.sAddress), QString::fromStdString(adrenaline.second.sBasenodePrivKey), QString::fromStdString(adrenaline.second.sCollateralAddress));
        }
    }
}

void RPCConsole::on_startButton_clicked()
{
    // start the node
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];

    std::string errorMessage;
    bool result = activeBasenode.RegisterByPubKey(c.sAddress, c.sBasenodePrivKey, c.sCollateralAddress, errorMessage);

    QMessageBox msg;
    if(result)
        msg.setText("Basenode at " + QString::fromStdString(c.sAddress) + " started.");
    else
        msg.setText("Error: " + QString::fromStdString(errorMessage));

    msg.exec();
}

void RPCConsole::on_stopButton_clicked()
{
    // start the node
    QItemSelectionModel* selectionModel = ui->tableWidget_2->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string sAddress = ui->tableWidget_2->item(r, 1)->text().toStdString();
    CAdrenalineNodeConfig c = pwalletMain->mapMyAdrenalineNodes[sAddress];

    std::string errorMessage;
    bool result = activeBasenode.StopBaseNode(c.sAddress, c.sBasenodePrivKey, errorMessage);
    QMessageBox msg;
    if(result)
    {
        msg.setText("Basenode at " + QString::fromStdString(c.sAddress) + " stopped.");
    }
    else
    {
        msg.setText("Error: " + QString::fromStdString(errorMessage));
    }
    msg.exec();
}

void RPCConsole::on_startAllButton_clicked()
{
    std::string results;
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        CAdrenalineNodeConfig c = adrenaline.second;
	std::string errorMessage;
        bool result = activeBasenode.RegisterByPubKey(c.sAddress, c.sBasenodePrivKey, c.sCollateralAddress, errorMessage);
	if(result)
	{
   	    results += c.sAddress + ": STARTED\n";
	}	
	else
	{
	    results += c.sAddress + ": ERROR: " + errorMessage + "\n";
	}
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(results));
    msg.exec();
}

void RPCConsole::on_stopAllButton_clicked()
{
    std::string results;
    BOOST_FOREACH(PAIRTYPE(std::string, CAdrenalineNodeConfig) adrenaline, pwalletMain->mapMyAdrenalineNodes)
    {
        CAdrenalineNodeConfig c = adrenaline.second;
	std::string errorMessage;
        bool result = activeBasenode.StopBaseNode(c.sAddress, c.sBasenodePrivKey, errorMessage);
	if(result)
	{
   	    results += c.sAddress + ": STOPPED\n";
	}	
	else
	{
	    results += c.sAddress + ": ERROR: " + errorMessage + "\n";
	}
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(results));
    msg.exec();
}
