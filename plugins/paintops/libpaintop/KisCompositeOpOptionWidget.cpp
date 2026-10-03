/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisCompositeOpOptionWidget.h"

#include <functional>

#include <QLabel>
#include <QWidget>

#include <KLocalizedString>
#include <KoCompositeOpRegistry.h>
#include <KoID.h>

#include <kis_paintop_option.h>
#include <kis_types.h>
#include <kis_icon_utils.h>

#include <KisCompositeOpListConnectionHelper.h>
#include <KisCompositeOpOptionData.h>
#include <KisCompositeOpOptionModel.h>
#include <KisWidgetConnectionUtils.h>

#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>

#include <ui_wdgCompositeOpOption.h>

struct KisCompositeOpOptionWidget::Private
{
    Private(lager::cursor<KisCompositeOpOptionData> optionData)
        : model(optionData)
    {
    }

    KisCompositeOpOptionModel model;

    QLabel *lblCurrentCompositeOp = 0;
};

KisCompositeOpOptionWidget::KisCompositeOpOptionWidget(lager::cursor<KisCompositeOpOptionData> optionData)
    : KisPaintOpOption(i18n("Blending Mode"), KisPaintOpOption::GENERAL, true),
      m_d(new Private(optionData))
{
    using namespace KisWidgetConnectionUtils;
    m_checkable = false;

    setObjectName("KisCompositeOpOption");

    QWidget* widget = new QWidget();

    Ui_wdgCompositeOpOption ui;
    ui.setupUi(widget);
    ui.bnEraser->setIcon(KisIconUtils::loadIcon("draw-eraser"));

    m_d->lblCurrentCompositeOp = ui.lbChosenMode;
    m_d->model.LAGER_QT(compositeOpId).bind(std::bind(&KisCompositeOpOptionWidget::updateCompositeOpLabel, this, std::placeholders::_1));

    connectControl(ui.bnEraser, &m_d->model, "eraserMode");
    connectControl(ui.list, &m_d->model, "compositeOpId");

    setConfigurationPage(widget);
    m_d->model.optionData.bind(std::bind(&KisCompositeOpOptionWidget::emitSettingChanged, this));
}

KisCompositeOpOptionWidget::~KisCompositeOpOptionWidget()
{
}

void KisCompositeOpOptionWidget::writeOptionSetting(KisPropertiesConfigurationSP setting) const
{
    m_d->model.optionData->write(setting.data());
}

void KisCompositeOpOptionWidget::readOptionSetting(const KisPropertiesConfigurationSP setting)
{
    KisCompositeOpOptionData data = *m_d->model.optionData;
    data.read(setting.data());
    m_d->model.optionData.set(data);
}

void KisCompositeOpOptionWidget::updateCompositeOpLabel(const QString &id)
{
    const KoID compositeOp = KoCompositeOpRegistry::instance().getKoID(id);
    m_d->lblCurrentCompositeOp->setText(compositeOp.name());
}
