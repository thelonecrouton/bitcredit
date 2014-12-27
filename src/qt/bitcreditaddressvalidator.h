// Copyright (c) 2011-2014 The Bitcredit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDIT_QT_BITCREDITADDRESSVALIDATOR_H
#define BITCREDIT_QT_BITCREDITADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class BitcreditAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcreditAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Bitcredit address widget validator, checks for a valid bitcredit address.
 */
class BitcreditAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcreditAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // BITCREDIT_QT_BITCREDITADDRESSVALIDATOR_H
