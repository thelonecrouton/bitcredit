#include "poolbrowser.h"
#include "ui_bittrex.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcreditrpc.h"
#include <QDesktopServices>

#include <sstream>
#include <string>


// Markets pages
const QString kBittrexPage = "https://www.bittrex.com/Market/Index?MarketName=BTC-BCR";

// Bitcredit to USD
const QString kCurrencyUSDUrl    = "http://blockchain.info/tobtc?currency=USD&value=1";

// Bittrex API urls
const QString kBittrexSummaryUrl        = "http://bittrex.com/api/v1/public/getmarketsummaries";
const QString kBittrexOrdersUrl         = "http://bittrex.com/api/v1/public/getorderbook?market=BTC-CAIX&type=both&depth=50";
const QString kBittrexHistoryUrl        = "http://bittrex.com/api/v1/public/getmarkethistory?market=BTC-CAIX&count=100";

QString bitcreditp = "";
double bitcreditToUSD;
double lastuG;
QString bitcreditg;
QString dollarg;
int mode=1;
QString lastp = "";
QString askp = "";
QString bidp = "";
QString highp = "";
QString lowp = "";
QString volumebp = "";
QString volumesp = "";
QString bop = "";
QString sop = "";
QString lastp2 = "";
QString askp2 = "";
QString bidp2 = "";
QString highp2 = "";
QString lowp2 = "";
QString volumebp2 = "";
QString volumesp2 = "";
QString bop2 = "";
QString sop2 = "";

double lastBittrex = 0.0;
double askBittrex = 0.0;
double bidBittrex = 0.0;
double volumeSCBittrex = 0.0;
double volumeBTCBittrex = 0.0;
double highBittrex = 0.0;
double lowBittrex = 0.0;
double changeBittrex = 0.0;

Bittrex::Bittrex(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Bittrex)
{
    ui->setupUi(this);
    ui->buyQuantityTable->header()->resizeSection(0,120);
    ui->sellQuantityTable->header()->resizeSection(0,120);
    setFixedSize(400, 420);

    this->setupBittrexGraphs();

    this->downloadAllMarketsData();

    QObject::connect(&m_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(parseNetworkResponse(QNetworkReply*)));
    connect(ui->egal, SIGNAL(pressed()), this, SLOT( egaldo()));

    this->setupBittrexTabSlots();
}

void Bittrex::setupBittrexGraphs()
{
    ui->pricesBittrexPlot->addGraph();
    ui->pricesBittrexPlot->setBackground(Qt::transparent);
    ui->volumeSatoshiPlotBittrex->addGraph();
    ui->volumeSatoshiPlotBittrex->addGraph();
    ui->volumeSatoshiPlotBittrex->setBackground(Qt::transparent);
}

void Bittrex::setupBittrexTabSlots()
{
    connect(ui->refreshBittrexButton, SIGNAL(pressed()), this, SLOT( downloadBittrexMarketData()));
    connect(ui->bittrexButton, SIGNAL(pressed()), this, SLOT( openBittrexPage()));
}

void Bittrex::egaldo()
{
    QString temps = ui->egals->text();
    double totald = lastuG * temps.toDouble();
    double totaldq = bitcreditg.toDouble() * temps.toDouble();
    ui->egald->setText(QString::number(totald) + " $ / "+QString::number(totaldq)+" BTC");
}

void Bittrex::getRequest( const QString &urlString )
{
    QUrl url ( urlString );
    QNetworkRequest req ( url );
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslV3);
    req.setSslConfiguration(config);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    m_nam.get(req);
}

void Bittrex::parseNetworkResponse(QNetworkReply *finished )
{
    QUrl requestUrl = finished->url();

    if ( finished->error() != QNetworkReply::NoError )
    {
        // A communication error has occurred
        emit networkError( finished->error() );
        return;
    }


    if (requestUrl == kBittrexSummaryUrl)
    {
        this->parseBittrexSummary(finished);
    }
    else if (requestUrl == kCurrencyUSDUrl)
    {
        this->parseCurrencyUSD(finished);
    }
    else if (requestUrl == kBittrexOrdersUrl)
    {
        this->parseBittrexOrders(finished);
    }
    else (requestUrl == kBittrexHistoryUrl)
    {
        this->parseBittrexHistory(finished);
    }

    finished->deleteLater();
}

void Bittrex::parseCurrencyUSD(QNetworkReply *replay)
{
    // QNetworkReply is a QIODevice. So we read from it just like it was a file
    QString bitcredit = replay->readAll();
    bitcreditToUSD = (1 / bitcredit.toDouble());
    bitcredit = QSTRING_DOUBLE(bitcreditToUSD);
    if(bitcredit > bitcreditp) {
        ui->bitcredit->setText("<b><font color=\"green\">" + bitcredit + " $</font></b>");
    } else if (bitcredit < bitcreditp) {
        ui->bitcredit->setText("<b><font color=\"red\">" + bitcredit + " $</font></b>");
    } else {
        ui->bitcredit->setText(bitcredit + " $");
    }

    bitcreditp = bitcredit;
}

void Bittrex::parseBittrexSummary(QNetworkReply *replay)
{
    QString data = replay->readAll();

    qDebug() << "BittrexSummary response:" << data;

    QJsonParseError jsonParseError;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8(), &jsonParseError);
    if (jsonResponse.isObject()) {

       QJsonObject mainObject = jsonResponse.object();
       if (!mainObject.contains("success")
               && !mainObject["success"].toBool()) {
           return;
       }

       if (mainObject.contains("result")) {
           QJsonArray resultArray = mainObject["result"].toArray();
           foreach (const QJsonValue &value, resultArray) {
               QJsonObject marketObject = value.toObject();
               if (marketObject.contains("MarketName")
                   && marketObject["MarketName"].toString() == "BTC-CAIX") {

                   // Updating summary labels
                   highBittrex = this->refreshDoubleVarUsingField(marketObject, "High",
                                                                  highBittrex, ui->highBittrex);
                   lowBittrex = this->refreshDoubleVarUsingField(marketObject, "Low",
                                                                 lowBittrex, ui->lowBittrex);

                   this->refreshDoubleVarAsBTCUsingField(marketObject, "BaseVolume",
                                                         volumeBTCBittrex, ui->volumeBTCBittrex);
                   volumeBTCBittrex = this->refreshDoubleVarAsUSDUsingField(marketObject, "BaseVolume",
                                                                            volumeBTCBittrex, ui->volumeUSDBittrex);

                   this->refreshDoubleVarAsBTCUsingField(marketObject, "Last",
                                                         lastBittrex, ui->lastBTCBittrex);
                   lastBittrex = this->refreshDoubleVarAsUSDUsingField(marketObject, "Last",
                                                                       lastBittrex, ui->lastUSDBittrex);

                   volumeSCBittrex = this->refreshDoubleVarUsingField(marketObject, "Volume",
                                                                      volumeSCBittrex, ui->volumeSCBittrex);

                   this->refreshDoubleVarAsBTCUsingField(marketObject, "Bid",
                                                         bidBittrex, ui->bidBTCBittrex);
                   bidBittrex = this->refreshDoubleVarAsUSDUsingField(marketObject, "Bid",
                                                                      bidBittrex, ui->bidUSDBittrex);

                   qDebug() << "Refreshing ask:";
                   this->refreshDoubleVarAsBTCUsingField(marketObject, "Ask",
                                                         askBittrex, ui->askBTCBittrex);
                   askBittrex = this->refreshDoubleVarAsUSDUsingField(marketObject, "Ask",
                                                                      askBittrex, ui->askUSDBittrex);

                   if (marketObject.contains("OpenBuyOrders")) {
                       ui->openBuyersBittrex->setText(QSTRING_DOUBLE(marketObject["OpenBuyOrders"].toDouble()));
                   }
                   if (marketObject.contains("OpenSellOrders")) {
                       ui->openSellersBittrex->setText(QSTRING_DOUBLE(marketObject["OpenSellOrders"].toDouble()));
                   }

                   if (marketObject.contains("PrevDay")
                           && marketObject.contains("Last")) {
                       double yesterday = marketObject["PrevDay"].toDouble();
                       double today = marketObject["Last"].toDouble();

                       double change;
                       if (today > yesterday) {
                           change = ( (today - yesterday) / today ) * 100;
                           this->setGreenTextForLabel("+" + QSTRING_DOUBLE(change) + "%", ui->yest);
                       } else {
                           change = ( (yesterday - today) / yesterday) * 100;
                           this->setRedTextForLabel("-" + QSTRING_DOUBLE(change) + "%", ui->yest);
                       }
                   }
               }
           }
       }

    } else {
        qWarning() << "Parsing Bittrex summary failed with error: " << jsonParseError.errorString();
    }
}

void Bittrex::parseBittrexOrders(QNetworkReply *replay)
{
    QString data = replay->readAll();

    qDebug() << "BittrexSummary response:" << data;

    QJsonParseError jsonParseError;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8(), &jsonParseError);
    if (jsonResponse.isObject()) {

       QJsonObject mainObject = jsonResponse.object();
       if (!mainObject.contains("success")
               && !mainObject["success"].toBool()) {
           return;
       }

       if (mainObject.contains("result")) {
           QJsonObject resultObject = mainObject["result"].toObject();

           QVector<double> volumeSell(50), satoshiSell(50);
           QVector<double> volumeBuy(50), satoshiBuy(50);

           if (resultObject.contains("buy")) {

               ui->buyQuantityTable->clear();
               ui->buyQuantityTable->setSortingEnabled(true);

               QJsonArray buyArray = resultObject["buy"].toArray();

               int i = 0;
               double cumulation = 0.0;
               foreach (const QJsonValue &order, buyArray) {
                    QJsonObject orderObject = order.toObject();
                    QTreeWidgetItem *orderItem = new QTreeWidgetItem();

                    if (orderObject.contains("Rate")) {
                        double rate = orderObject["Rate"].toDouble();
                        orderItem->setText(1, QSTRING_DOUBLE(rate));

                        double satoshi = rate * 100000000;

                        satoshiBuy[i] = satoshi;
                        volumeBuy[i] = cumulation;
                    }
                    if (orderObject.contains("Quantity")) {
                        double quantity = orderObject["Quantity"].toDouble();
                        orderItem->setText(0, QSTRING_DOUBLE(quantity));

                        cumulation += quantity;
                    }

                    ui->buyQuantityTable->addTopLevelItem(orderItem);
                }
           }

           if (resultObject.contains("sell")) {

               ui->sellQuantityTable->clear();
               ui->sellQuantityTable->sortByColumn(0, Qt::AscendingOrder);
               ui->sellQuantityTable->setSortingEnabled(true);

               QJsonArray sellArray = resultObject["sell"].toArray();

               int i = 0;
               double cumulation = 0.0;
               foreach (const QJsonValue &order, sellArray) {
                    QJsonObject orderObject = order.toObject();
                    QTreeWidgetItem *orderItem = new QTreeWidgetItem();

                    if (orderObject.contains("Rate")) {
                        double rate = orderObject["Rate"].toDouble();
                        orderItem->setText(0, QSTRING_DOUBLE(rate));

                        double satoshi = rate * 100000000;

                        satoshiSell[i] = satoshi;
                        volumeSell[i] = cumulation;
                    }
                    if (orderObject.contains("Quantity")) {
                        double quantity = orderObject["Quantity"].toDouble();
                        orderItem->setText(1, QSTRING_DOUBLE(quantity));

                        cumulation += quantity;
                    }
                    ui->sellQuantityTable->addTopLevelItem(orderItem);
                }
           }

           this->updateVolumeSatoshiPlot(satoshiSell, satoshiBuy, volumeSell, volumeBuy, ui->volumeSatoshiPlotBittrex);
       }

    } else {
        qWarning() << "Parsing Bittrex summary failed with error: " << jsonParseError.errorString();
    }
}

void Bittrex::parseBittrexHistory(QNetworkReply *replay)
{
    QString data = replay->readAll();

    qDebug() << "BittrexSummary response:" << data;

    QJsonParseError jsonParseError;
    QJsonDocument jsonResponse = QJsonDocument::fromJson(data.toUtf8(), &jsonParseError);
    if (jsonResponse.isObject()) {

       QJsonObject mainObject = jsonResponse.object();
       if (!mainObject.contains("success")
               && !mainObject["success"].toBool()) {
           return;
       }

       if (mainObject.contains("result")) {
           QJsonArray resultArray = mainObject["result"].toArray();

           ui->tradesTableBittrex->clear();
           ui->tradesTableBittrex->setColumnWidth(0,  60);
           ui->tradesTableBittrex->setColumnWidth(1,  100);
           ui->tradesTableBittrex->setColumnWidth(2,  100);
           ui->tradesTableBittrex->setColumnWidth(3,  100);
           ui->tradesTableBittrex->setColumnWidth(4,  180);
           ui->tradesTableBittrex->setColumnWidth(5,  100);
           ui->tradesTableBittrex->setSortingEnabled(true);

           QVector<double> count(50), prices(50);

           int i = 0;
           foreach (const QJsonValue &tradeValue, resultArray) {

               QJsonObject tradeObject = tradeValue.toObject();
               QTreeWidgetItem *tradeItem = new QTreeWidgetItem();

               if (tradeObject.contains("OrderType")) {
                   QString typeStr = tradeObject["OrderType"].toString();
                   tradeItem->setText(0, typeStr);
               }
               if (tradeObject.contains("Quantity")) {
                   double quantity = tradeObject["Quantity"].toDouble();
                   tradeItem->setText(1, QSTRING_DOUBLE(quantity));
               }
               if (tradeObject.contains("Price")) {
                   double rate = tradeObject["Price"].toDouble();
                   tradeItem->setText(2, QSTRING_DOUBLE(rate));

                   count[i]  = resultArray.count() - i;
                   prices[i] = rate * 100000000;
               }
               if (tradeObject.contains("Total")) {
                   double total = tradeObject["Total"].toDouble();
                   tradeItem->setText(3, QSTRING_DOUBLE(total));
               }
               if (tradeObject.contains("TimeStamp")) {
                   QString timeStampStr = tradeObject["TimeStamp"].toString();
                   tradeItem->setText(4, timeStampStr);
               }
               if (tradeObject.contains("Id")) {
                   double id = tradeObject["Id"].toDouble();
                   tradeItem->setText(5, QSTRING_DOUBLE(id));
               }

               ui->tradesTableBittrex->addTopLevelItem(tradeItem);
               i++;
           }
           this->updatePricesPlot(count, prices, ui->pricesBittrexPlot);
       }

    } else {
        qWarning() << "Parsing Bittrex summary failed with error: " << jsonParseError.errorString();
    }
}

void Bittrex::openBittrexPage()
{
    this->openUrl(kBittrexPage);
}

void Bittrex::openUrl(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void Bittrex::downloadAllMarketsData()
{
    this->downloadBittrexMarketData();
}

void Bittrex::downloadBittrexMarketData()
{
    this->getRequest(kCurrencyUSDUrl);
    this->getRequest(kBittrexSummaryUrl);
    this->getRequest(kBittrexOrdersUrl);
    this->getRequest(kBittrexHistoryUrl);
}

void Bittrex::setColoredTextBasedOnValues(const QString &text, QLabel *label, double prevValue, double actualValue)
{
    if(actualValue > prevValue) {
        this->setGreenTextForLabel(text, label);
    } else if (actualValue < prevValue) {
        this->setRedTextForLabel(text, label);
    } else {
        label->setText(text);
    }
}


void Bittrex::setRedTextForLabel(const QString &text, QLabel *label)
{
    label->setText("<b><font color=\"red\">" + text + "</font></b>");
}

void Bittrex::setGreenTextForLabel(const QString &text, QLabel *label)
{
    label->setText("<b><font color=\"green\">" + text + "</font></b>");
}

void Bittrex::refreshValueLabel(double actualValue, QLabel *label, double newDouble, const QString &suffix, QString newStr)
{
    QString uiString = newStr;
    if (suffix.length() > 0)
    {
        uiString += suffix;
    }

    this->setColoredTextBasedOnValues(uiString, label,
                                      actualValue, newDouble);
}

double Bittrex::refreshDoubleVarUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label, const QString &suffix)
{
    // Updating high label
    if (jsonObject.contains(fieldName)) {
        double newDouble = jsonObject[fieldName].toDouble();

        qDebug() << newDouble;

        refreshValueLabel(actualValue, label, newDouble, suffix, QSTRING_DOUBLE(newDouble));
        return newDouble;
    } else {
        return actualValue;
    }
}

double Bittrex::refreshStringVarUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label, const QString &suffix)
{
    // Updating high label
    if (jsonObject.contains(fieldName)) {

        QString newStr = jsonObject[fieldName].toString();
        double newDouble = newStr.toDouble();

        refreshValueLabel(actualValue, label, newDouble, suffix, newStr);
        return newDouble;
    } else {
        return actualValue;
    }
}

double Bittrex::refreshStringVarAsBTCUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label)
{
    return this->refreshStringVarUsingField(jsonObject, fieldName, actualValue, label, " B");
}

void Bittrex::refreshUSDLabel(QLabel *label, double actualValue, double newDouble)
{
    double newUSD = newDouble * bitcreditToUSD;
    double prevUSD = actualValue * bitcreditToUSD;
    qDebug() << "newUSD " << newUSD << " prevUSD " << prevUSD;
    this->setColoredTextBasedOnValues(QSTRING_DOUBLE(newUSD) + " $", label, prevUSD, newUSD);
}

double Bittrex::refreshStringVarAsUSDUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label)
{
    if (jsonObject.contains(fieldName)) {
        double newDouble = jsonObject[fieldName].toString().toDouble();
        refreshUSDLabel(label, actualValue, newDouble);
        return newDouble;
    } else {
        return actualValue;
    }
}

double Bittrex::refreshDoubleVarAsBTCUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label)
{
    return this->refreshDoubleVarUsingField(jsonObject, fieldName, actualValue, label, " B");
}

double Bittrex::refreshDoubleVarAsUSDUsingField(const QJsonObject &jsonObject, const QString &fieldName, double actualValue, QLabel *label)
{
    if (jsonObject.contains(fieldName)) {
        double newDouble = jsonObject[fieldName].toDouble();
        refreshUSDLabel(label, actualValue, newDouble);
        return newDouble;
    } else {
        return actualValue;
    }
}

void Bittrex::updatePricesPlot(const QVector<double> &count, const QVector<double> &prices, QCustomPlot *plot)
{
    double min = 100000000;
    double max = 0;

    QVectorIterator<double> i(prices);
    while (i.hasNext()) {
         double price = i.next();
         if (price > max) {
             max = price;
         } else if (price < min) {
             min = price;
         }
    }

    plot->graph(0)->setData(count, prices);
    plot->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    plot->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    plot->xAxis->setRange(1, prices.count());
    plot->yAxis->setRange(min, max);
    plot->replot();
}

void Bittrex::updateVolumeSatoshiPlot(const QVector<double> &satoshiSell, QVector<double> &satoshiBuy, const QVector<double> &volumeSell, const QVector<double> &volumeBuy, QCustomPlot *plot)
{
    double min = this->getMinValueFromVector(satoshiBuy);
    double max = this->getMaxValueFromVector(satoshiSell);
    double maxCumulationSell = this->getMaxValueFromVector(volumeSell);
    double maxCumulationBuy = this->getMaxValueFromVector(volumeBuy);
    double maxCumulation;
    if (maxCumulationSell > maxCumulationBuy) {
        maxCumulation = maxCumulationSell;
    } else {
        maxCumulation = maxCumulationBuy;
    }

    // create graph and assign data to it:
    plot->graph(0)->setData(satoshiBuy, volumeBuy);
    plot->graph(1)->setData(satoshiSell, volumeSell);

    plot->graph(0)->setPen(QPen(QColor(34, 177, 76)));
    plot->graph(0)->setBrush(QBrush(QColor(34, 177, 76, 20)));
    plot->graph(1)->setPen(QPen(QColor(237, 24, 35)));
    plot->graph(1)->setBrush(QBrush(QColor(237, 24, 35, 20)));

    // set axes ranges, so we see all data:
    plot->xAxis->setRange(min, max);
    plot->yAxis->setRange(min, maxCumulation);

    plot->replot();
}

double Bittrex::getMaxValueFromVector(const QVector<double> &vector) {
    double max = -999999999.0;
    QVectorIterator<double> iterator(vector);
    while (iterator.hasNext()) {
        double value = iterator.next();
        if (value > max) {
            max = value;
        }
    }
    return max;
}

double Bittrex::getMinValueFromVector(const QVector<double> &vector) {
    double min = 999999999.0;
    QVectorIterator<double> iterator(vector);
    while (iterator.hasNext()) {
        double value = iterator.next();
        if (value < min) {
            min = value;
        }
    }
    return min;
}

void Bittrex::setModel(ClientModel *model)
{
    this->model = model;
}

Bittrex::~Bittrex()
{
    delete ui;
}
