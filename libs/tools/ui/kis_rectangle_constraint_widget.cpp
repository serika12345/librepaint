/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_rectangle_constraint_widget.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <cmath>

#include <kis_aspect_ratio_locker.h>
#include <kis_icon.h>
#include <kis_signals_blocker.h>

#include "ui_wdgrectangleconstraints.h"

struct KisRectangleConstraintWidget::Private
{
    explicit Private(const QString &configGroup)
        : configGroup(configGroup)
    {
    }

    QString configGroup;
    Ui::WdgRectangleConstraints ui;
    KisAspectRatioLocker *cornersAspectLocker = nullptr;
};

namespace
{
void updateLockIcons(Ui::WdgRectangleConstraints &ui)
{
    const QIcon lockedIcon = KisIconUtils::loadIcon("locked");
    const QIcon unlockedIcon = KisIconUtils::loadIcon("unlocked");
    ui.lockWidthButton->setIcon(ui.lockWidthButton->isChecked()
                                    ? lockedIcon
                                    : unlockedIcon);
    ui.lockHeightButton->setIcon(ui.lockHeightButton->isChecked()
                                     ? lockedIcon
                                     : unlockedIcon);
    ui.lockRatioButton->setIcon(ui.lockRatioButton->isChecked()
                                    ? lockedIcon
                                    : unlockedIcon);
}
}

KisRectangleConstraintWidget::KisRectangleConstraintWidget(
    const QString &configGroup,
    bool showRoundCornersGui,
    QWidget *parent)
    : QWidget(parent)
    , m_d(new Private(configGroup))
{
    m_d->ui.setupUi(this);

    connect(m_d->ui.lockWidthButton,
            &QPushButton::toggled,
            this,
            &KisRectangleConstraintWidget::inputsChanged);
    connect(m_d->ui.lockHeightButton,
            &QPushButton::toggled,
            this,
            &KisRectangleConstraintWidget::inputsChanged);
    connect(m_d->ui.lockRatioButton,
            &QPushButton::toggled,
            this,
            &KisRectangleConstraintWidget::inputsChanged);
    updateLockIcons(m_d->ui);

    connect(m_d->ui.intWidth,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &KisRectangleConstraintWidget::inputWidthChanged);
    connect(m_d->ui.intHeight,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &KisRectangleConstraintWidget::inputHeightChanged);
    connect(m_d->ui.doubleRatio,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &KisRectangleConstraintWidget::inputRatioChanged);

    m_d->cornersAspectLocker = new KisAspectRatioLocker(this);
    m_d->cornersAspectLocker->connectSpinBoxes(m_d->ui.intRoundCornersX,
                                               m_d->ui.intRoundCornersY,
                                               m_d->ui.cornersAspectButton);
    connect(m_d->cornersAspectLocker,
            &KisAspectRatioLocker::sliderValueChanged,
            this,
            &KisRectangleConstraintWidget::slotRoundCornersChanged);
    connect(m_d->cornersAspectLocker,
            &KisAspectRatioLocker::aspectButtonChanged,
            this,
            &KisRectangleConstraintWidget::slotRoundCornersAspectLockChanged);

    if (!showRoundCornersGui) {
        m_d->ui.intRoundCornersX->hide();
        m_d->ui.intRoundCornersY->hide();
        m_d->ui.cornersAspectButton->hide();
    }
}

KisRectangleConstraintWidget::~KisRectangleConstraintWidget() = default;

void KisRectangleConstraintWidget::inputWidthChanged()
{
    m_d->ui.lockWidthButton->setChecked(true);
    inputsChanged();
}

void KisRectangleConstraintWidget::inputHeightChanged()
{
    m_d->ui.lockHeightButton->setChecked(true);
    inputsChanged();
}

void KisRectangleConstraintWidget::inputRatioChanged()
{
    m_d->ui.lockRatioButton->setChecked(true);
    inputsChanged();
}

void KisRectangleConstraintWidget::inputsChanged()
{
    Q_EMIT constraintsChanged(
        m_d->ui.lockRatioButton->isChecked(),
        m_d->ui.lockWidthButton->isChecked(),
        m_d->ui.lockHeightButton->isChecked(),
        m_d->ui.doubleRatio->value(),
        m_d->ui.intWidth->value(),
        m_d->ui.intHeight->value());
    updateLockIcons(m_d->ui);
}

void KisRectangleConstraintWidget::slotRoundCornersChanged()
{
    const int xRadius = m_d->ui.intRoundCornersX->value();
    const int yRadius = m_d->ui.intRoundCornersY->value();
    Q_EMIT roundCornersChanged(xRadius, yRadius);

    KConfigGroup cfg = KSharedConfig::openConfig()->group(m_d->configGroup);
    cfg.writeEntry("roundCornersX", xRadius);
    cfg.writeEntry("roundCornersY", yRadius);
}

void KisRectangleConstraintWidget::slotRoundCornersAspectLockChanged()
{
    KConfigGroup cfg = KSharedConfig::openConfig()->group(m_d->configGroup);
    cfg.writeEntry("roundCornersAspectLocked",
                   m_d->ui.cornersAspectButton->keepAspectRatio());
}

void KisRectangleConstraintWidget::reloadConfig()
{
    KConfigGroup cfg = KSharedConfig::openConfig()->group(m_d->configGroup);

    {
        KisSignalsBlocker blocker(m_d->ui.intRoundCornersX,
                                  m_d->ui.intRoundCornersY,
                                  m_d->ui.cornersAspectButton);
        m_d->ui.intRoundCornersX->setValue(
            cfg.readEntry("roundCornersX", 0));
        m_d->ui.intRoundCornersY->setValue(
            cfg.readEntry("roundCornersY", 0));
        m_d->ui.cornersAspectButton->setKeepAspectRatio(
            cfg.readEntry("roundCornersAspectLocked", true));
        m_d->cornersAspectLocker->updateAspect();
    }

    slotRoundCornersChanged();
}

void KisRectangleConstraintWidget::setRectangle(const QRectF &rect)
{
    KisSignalsBlocker blocker(m_d->ui.intWidth,
                              m_d->ui.intHeight,
                              m_d->ui.doubleRatio);

    if (!m_d->ui.lockWidthButton->isChecked()) {
        m_d->ui.intWidth->setValue(rect.width());
    }
    if (!m_d->ui.lockHeightButton->isChecked()) {
        m_d->ui.intHeight->setValue(rect.height());
    }

    if (!m_d->ui.lockRatioButton->isChecked()
        && !(rect.width() == 0 && rect.height() == 0)) {
        m_d->ui.doubleRatio->setValue(std::abs(rect.width())
                                     / std::abs(rect.height()));
    }
}
