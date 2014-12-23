// Copyright (c) 2011-2014 The Bitcredits developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCREDITSADDRESSVALIDATOR_H
#define BITCREDITSADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class BitcreditsAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcreditsAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Bitcredits address widget validator, checks for a valid bitcredits address.
 */
class BitcreditsAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit BitcreditsAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // BITCREDITSADDRESSVALIDATOR_H
