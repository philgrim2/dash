// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef THOUGHT_QT_CALLBACK_H
#define THOUGHT_QT_CALLBACK_H

#include <QObject>

class Callback : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    virtual void call() = 0;
};

template <typename F>
class FunctionCallback : public Callback
{
    F f;

public:
    explicit FunctionCallback(F f_) : f(std::move(f_)) {}
    ~FunctionCallback() override {}
    void call() override { f(this); }
};

template <typename F>
FunctionCallback<F>* makeCallback(F f)
{
    return new FunctionCallback<F>(std::move(f));
}

#endif // THOUGHT_QT_CALLBACK_H
