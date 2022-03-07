// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THOUGHT_QT_UTILITYDIALOG_H
#define THOUGHT_QT_UTILITYDIALOG_H

#include <QDialog>
#include <QObject>

class ThoughtGUI;

namespace interfaces {
    class Node;
}

namespace Ui {
    class HelpMessageDialog;
}

/** "Help message" dialog box */
class HelpMessageDialog : public QDialog
{
    Q_OBJECT

public:
    enum HelpMode {
        about,
        cmdline,
        pshelp
    };

    explicit HelpMessageDialog(interfaces::Node& node, QWidget *parent, HelpMode helpMode);
    ~HelpMessageDialog();

    void printToConsole();
    void showOrPrint();

private:
    Ui::HelpMessageDialog *ui;
    QString text;

private Q_SLOTS:
    void on_okButton_accepted();
};


/** "Shutdown" window */
class ShutdownWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ShutdownWindow(interfaces::Node& node, QWidget *parent=0, Qt::WindowFlags f=0);
    static QWidget *showShutdownWindow(interfaces::Node& node, ThoughtGUI *window);

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // THOUGHT_QT_UTILITYDIALOG_H
