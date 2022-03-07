// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_QT_THOUGHTADDRESSVALIDATOR_H
#define THOUGHT_QT_THOUGHTADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class ThoughtAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit ThoughtAddressEntryValidator(QObject *parent, bool fAllowURI = false);

    State validate(QString &input, int &pos) const;

private:
    bool fAllowURI;
};

/** Thought address widget validator, checks for a valid bitcoin address.
 */
class ThoughtAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit ThoughtAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // THOUGHT_QT_THOUGHTADDRESSVALIDATOR_H
