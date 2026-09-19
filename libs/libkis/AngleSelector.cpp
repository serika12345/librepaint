/*
 *  SPDX-FileCopyrightText: 2020 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "AngleSelector.h"

#include "kis_debug.h"


namespace
{

QString flipOptionsModeName(KisAngleSelector::FlipOptionsMode mode)
{
    switch (mode) {
    case KisAngleSelector::FlipOptionsMode_NoFlipOptions:
        return QStringLiteral("NoFlipOptions");
    case KisAngleSelector::FlipOptionsMode_MenuButton:
        return QStringLiteral("MenuButton");
    case KisAngleSelector::FlipOptionsMode_Buttons:
        return QStringLiteral("Buttons");
    case KisAngleSelector::FlipOptionsMode_ContextMenu:
        return QStringLiteral("ContextMenu");
    }
    return {};
}

bool parseFlipOptionsMode(const QString &name, KisAngleSelector::FlipOptionsMode *mode)
{
    if (name == QLatin1String("NoFlipOptions")) {
        *mode = KisAngleSelector::FlipOptionsMode_NoFlipOptions;
    } else if (name == QLatin1String("MenuButton")) {
        *mode = KisAngleSelector::FlipOptionsMode_MenuButton;
    } else if (name == QLatin1String("Buttons")) {
        *mode = KisAngleSelector::FlipOptionsMode_Buttons;
    } else if (name == QLatin1String("ContextMenu")) {
        *mode = KisAngleSelector::FlipOptionsMode_ContextMenu;
    } else {
        return false;
    }
    return true;
}

QString increasingDirectionName(KisAngleGauge::IncreasingDirection direction)
{
    switch (direction) {
    case KisAngleGauge::IncreasingDirection_CounterClockwise:
        return QStringLiteral("CounterClockwise");
    case KisAngleGauge::IncreasingDirection_Clockwise:
        return QStringLiteral("Clockwise");
    }
    return {};
}

bool parseIncreasingDirection(const QString &name, KisAngleGauge::IncreasingDirection *direction)
{
    if (name == QLatin1String("CounterClockwise")) {
        *direction = KisAngleGauge::IncreasingDirection_CounterClockwise;
    } else if (name == QLatin1String("Clockwise")) {
        *direction = KisAngleGauge::IncreasingDirection_Clockwise;
    } else {
        return false;
    }
    return true;
}

} // namespace

struct AngleSelector::Private {
    Private() {}

    KisAngleSelector *widget;
};

AngleSelector::AngleSelector()
    : QObject()
    , d(new Private)
{
    d->widget = new KisAngleSelector();

    // Forward KisAngleSelector::angleChanged to AngleSelector::angleChanged
    connect(d->widget, SIGNAL(angleChanged(qreal)), this, SIGNAL(angleChanged(qreal)));
}

AngleSelector::~AngleSelector()
{
    delete d;
}

QWidget* AngleSelector::widget() const
{
    return d->widget;
}

qreal AngleSelector::angle() const
{
    return d->widget->angle();
}

qreal AngleSelector::snapAngle() const
{
    return d->widget->snapAngle();
}

qreal AngleSelector::resetAngle() const
{
    return d->widget->resetAngle();
}

int	AngleSelector::decimals() const
{
    return d->widget->decimals();
}

qreal AngleSelector::maximum() const
{
    return d->widget->maximum();
}

qreal AngleSelector::minimum() const
{
    return d->widget->minimum();
}

QString AngleSelector::prefix() const
{
    return d->widget->prefix();
}

bool AngleSelector::wrapping() const
{
    return d->widget->wrapping();
}

QString AngleSelector::flipOptionsMode() const
{
    const KisAngleSelector::FlipOptionsMode mode = d->widget->flipOptionsMode();
    const QString name = flipOptionsModeName(mode);
    if (name.isEmpty()) {
        warnScript << "AngleSelector::flipOptionsMode() doesn't handle mode '" << mode << "'!";
    }
    return name;
}

int AngleSelector::widgetsHeight() const
{
    return d->widget->widgetsHeight();
}

QString AngleSelector::increasingDirection() const
{
    const KisAngleGauge::IncreasingDirection direction = d->widget->increasingDirection();
    const QString name = increasingDirectionName(direction);
    if (name.isEmpty()) {
        warnScript << "AngleSelector::increasingDirection() doesn't handle mode '" << direction << "'!";
    }
    return name;
}

bool AngleSelector::isUsingFlatSpinBox() const
{
    return d->widget->isUsingFlatSpinBox();
}

void AngleSelector::setAngle(qreal newAngle)
{
    d->widget->setAngle(newAngle);
}

void AngleSelector::setSnapAngle(qreal newSnapAngle)
{
    d->widget->setSnapAngle(newSnapAngle);
}

void AngleSelector::setResetAngle(qreal newResetAngle)
{
    d->widget->setResetAngle(newResetAngle);
}

void AngleSelector::setDecimals(int newNumberOfDecimals)
{
    d->widget->setDecimals(newNumberOfDecimals);
}

void AngleSelector::setMaximum(qreal newMaximum)
{
    d->widget->setMaximum(newMaximum);
}

void AngleSelector::setMinimum(qreal newMinimum)
{
    d->widget->setMinimum(newMinimum);
}

void AngleSelector::setRange(qreal newMinimum, qreal newMaximum)
{
    d->widget->setRange(newMinimum, newMaximum);
}

void AngleSelector::setPrefix(const QString &newPrefix)
{
    d->widget->setPrefix(newPrefix);
}

void AngleSelector::setWrapping(bool newWrapping)
{
    d->widget->setWrapping(newWrapping);
}

void AngleSelector::setFlipOptionsMode(QString newMode)
{
    KisAngleSelector::FlipOptionsMode mode;
    if (!parseFlipOptionsMode(newMode, &mode)) {
        dbgScript << "Script using AngleSelector.setFlipOptionsMode() passed invalid mode '" << newMode << "', ignoring.";
        return;
    }
    d->widget->setFlipOptionsMode(mode);
}

void AngleSelector::setWidgetsHeight(int newHeight)
{
    d->widget->setWidgetsHeight(newHeight);
}

void AngleSelector::setIncreasingDirection(QString newIncreasingDirection)
{
    KisAngleGauge::IncreasingDirection direction;
    if (!parseIncreasingDirection(newIncreasingDirection, &direction)) {
        dbgScript << "Script using AngleSelector.setIncreasingDirection() passed invalid mode '" << newIncreasingDirection << "', ignoring.";
        return;
    }
    d->widget->setIncreasingDirection(direction);
}

void AngleSelector::useFlatSpinBox(bool newUseFlatSpinBox)
{
    d->widget->useFlatSpinBox(newUseFlatSpinBox);
}

void AngleSelector::reset()
{
    d->widget->reset();
}

qreal AngleSelector::closestCoterminalAngleInRange(qreal angle, qreal minimum, qreal maximum, bool *ok)
{
    return KisAngleSelector::closestCoterminalAngleInRange(angle, minimum, maximum, ok);
}

qreal AngleSelector::closestCoterminalAngleInRange(qreal angle, bool *ok) const
{
    return d->widget->closestCoterminalAngleInRange(angle, ok);
}

qreal AngleSelector::flipAngle(qreal angle, Qt::Orientations orientations)
{
    return KisAngleSelector::flipAngle(angle, orientations);
}

qreal AngleSelector::flipAngle(qreal angle, qreal minimum, qreal maximum, Qt::Orientations orientations, bool *ok)
{
    return KisAngleSelector::flipAngle(angle, minimum, maximum, orientations, ok);
}

void AngleSelector::flip(Qt::Orientations orientations)
{
    bool ok = false;
    qreal flippedAngle = flipAngle(angle(), minimum(), maximum(), orientations, &ok);
    if (ok) {
        setAngle(flippedAngle);
    }
}
