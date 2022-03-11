// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2018-2022 Thought Network Ltd
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/thoughtamountfield.h>

#include <qt/thoughtunits.h>
#include <qt/guiutil.h>

#include <QApplication>
#include <QAbstractSpinBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>

/**
 * Parse a string into a number of base monetary units and
 * return validity.
 * @note Must return 0 if !valid.
 */
static CAmount parse(const QString &text, int nUnit, bool *valid_out=0)
{
    CAmount val = 0;
    bool valid = ThoughtUnits::parse(nUnit, text, &val);
    if(valid)
    {
        if(val < 0 || val > ThoughtUnits::maxMoney())
            valid = false;
    }
    if(valid_out)
        *valid_out = valid;
    return valid ? val : 0;
}

/** Amount widget validator, checks for valid CAmount value.
 */
class AmountValidator : public QValidator
{
    Q_OBJECT
    int currentUnit;
public:
    explicit AmountValidator(QObject *parent) :
        QValidator(parent),
        currentUnit(ThoughtUnits::THT) {}

    State validate(QString &input, int &pos) const
    {
        if(input.isEmpty())
            return QValidator::Intermediate;
        bool valid = false;
        parse(input, currentUnit, &valid);
        /* Make sure we return Intermediate so that fixup() is called on defocus */
        return valid ? QValidator::Intermediate : QValidator::Invalid;
    }

    void updateUnit(int nUnit)
    {
        currentUnit = nUnit;
    }
};

/** QLineEdit that uses fixed-point numbers internally and uses our own
 * formatting/parsing functions.
 */
class AmountLineEdit: public QLineEdit
{
    Q_OBJECT
    AmountValidator* amountValidator;
public:
    explicit AmountLineEdit(QWidget *parent):
        QLineEdit(parent),
        currentUnit(ThoughtUnits::THT)
    {
        setAlignment(Qt::AlignLeft);
        amountValidator = new AmountValidator(this);
        setValidator(amountValidator);
        connect(this, SIGNAL(textEdited(QString)), this, SIGNAL(valueChanged()));
    }

    void fixup(const QString &input)
    {
        bool valid = false;
        CAmount val = parse(input, currentUnit, &valid);
        if(valid)
        {
            setText(ThoughtUnits::format(currentUnit, val, false, ThoughtUnits::separatorAlways));
        }
    }

    CAmount value(bool *valid_out=0) const
    {
        return parse(text(), currentUnit, valid_out);
    }

    void setValue(const CAmount& value)
    {
        setText(ThoughtUnits::format(currentUnit, value, false, ThoughtUnits::separatorAlways));
        Q_EMIT valueChanged();
    }

    void setDisplayUnit(int unit)
    {
        bool valid = false;
        CAmount val = value(&valid);

        currentUnit = unit;
        amountValidator->updateUnit(unit);

        if(valid)
            setValue(val);
        else
            clear();
    }

    QSize minimumSizeHint() const
    {
        ensurePolished();
        const QFontMetrics fm(fontMetrics());
        int h = 0;
        int w = fm.width(ThoughtUnits::format(ThoughtUnits::THT, ThoughtUnits::maxMoney(), false, ThoughtUnits::separatorAlways));
        w += 2; // cursor blinking space
        w += GUIUtil::thoughtThemeActive() ? 24 : 0; // counteract padding from css
        return QSize(w, h);
    }

private:
    int currentUnit;

protected:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Comma)
            {
                // Translate a comma into a period
                QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
                return QLineEdit::event(&periodKeyEvent);
            }
            if(keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
            {
                clearFocus();
            }
        }
        if (event->type() == QEvent::FocusOut)
        {
            fixup(text());
        }
        return QLineEdit::event(event);
    }

Q_SIGNALS:
    void valueChanged();
};

#include <qt/thoughtamountfield.moc>

ThoughtAmountField::ThoughtAmountField(QWidget *parent) :
    QWidget(parent),
    amount(0)
{
    amount = new AmountLineEdit(this);
    amount->setLocale(QLocale::c());
    amount->installEventFilter(this);
    amount->setMaximumWidth(300);

    units = new ThoughtUnits(this);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(amount);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(valueChanged()), this, SIGNAL(valueChanged()));
}

void ThoughtAmountField::clear()
{
    amount->clear();
}

void ThoughtAmountField::setEnabled(bool fEnabled)
{
    amount->setEnabled(fEnabled);
}

bool ThoughtAmountField::validate()
{
    bool valid = false;
    value(&valid);
    setValid(valid);
    return valid;
}

void ThoughtAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        amount->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_INVALID));
}

bool ThoughtAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    return QWidget::eventFilter(object, event);
}

QWidget *ThoughtAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    return amount;
}

CAmount ThoughtAmountField::value(bool *valid_out) const
{
    return amount->value(valid_out);
}

void ThoughtAmountField::setValue(const CAmount& value)
{
    amount->setValue(value);
}

void ThoughtAmountField::setReadOnly(bool fReadOnly)
{
    amount->setReadOnly(fReadOnly);
}

void ThoughtAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    amount->setToolTip(units->data(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    int newUnit = units->data(idx, ThoughtUnits::UnitRole).toInt();

    amount->setPlaceholderText(tr("Amount in %1").arg(units->data(idx,Qt::DisplayRole).toString()));

    amount->setDisplayUnit(newUnit);
}

void ThoughtAmountField::setDisplayUnit(int newUnit)
{
    unitChanged(newUnit);
}
