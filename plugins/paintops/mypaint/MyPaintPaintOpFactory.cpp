/*
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.com>
 * SPDX-FileCopyrightText: 2020 Ashwin Dhakaita <ashwingpdhakaita@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MyPaintPaintOpFactory.h"

#include <QtGlobal>
#include <qicon.h>
#include <qlist.h>
#include <QJsonObject>
#include <QJsonDocument>

#include <KoResourceLoadResult.h>
#include <qobject.h>
#include <qwidget.h>

#include "KisPaintopPropertiesBase.h"
#include "MyPaintPaintOp.h"
#include "MyPaintPaintOpSettings.h"
#include "MyPaintPaintOpSettingsWidget.h"
#include "kis_icon_utils.h"
#include "kis_image.h"
#include "kis_paintop.h"
#include "kis_paintop_config_widget.h"
#include "kis_paintop_factory.h"
#include "kis_types.h"

class KisMyPaintOpFactory::Private {
};

KisMyPaintOpFactory::KisMyPaintOpFactory()
    : m_d(new Private)
{
}

KisMyPaintOpFactory::~KisMyPaintOpFactory() {

    delete m_d;
}

KisPaintOp* KisMyPaintOpFactory::createOp(const KisPaintOpSettingsSP settings, KisPainter *painter, KisNodeSP node, KisImageSP image) {

    KisPaintOp* op = new KisMyPaintPaintOp(settings, painter, node, image);
    Q_CHECK_PTR(op);
    return op;
}

KisPaintOpSettingsSP KisMyPaintOpFactory::createSettings(KisResourcesInterfaceSP resourcesInterface) {

    KisPaintOpSettingsSP settings = new KisMyPaintOpSettings(resourcesInterface);
    return settings;
}

KisPaintOpConfigWidget* KisMyPaintOpFactory::createConfigWidget(QWidget* parent, KisResourcesInterfaceSP resourcesInterface, KoCanvasResourcesInterfaceSP canvasResourcesInterface) {

    KisPaintOpConfigWidget *widget = new KisMyPaintOpSettingsWidget(parent);

    widget->setResourcesInterface(resourcesInterface);
    widget->setCanvasResourcesInterface(canvasResourcesInterface);

    return widget;
}

QString KisMyPaintOpFactory::id() const {

    return "mypaintbrush";
}

QString KisMyPaintOpFactory::name() const {

    return "MyPaint";
}

QIcon KisMyPaintOpFactory::icon() {

    return KisIconUtils::loadIcon(id());
}

QString KisMyPaintOpFactory::category() const {

    return KisPaintOpFactory::categoryStable();
}

QList<KoResourceLoadResult> KisMyPaintOpFactory::prepareLinkedResources(const KisPaintOpSettingsSP settings, KisResourcesInterfaceSP resourcesInterface)
{
    Q_UNUSED(settings)
    Q_UNUSED(resourcesInterface);

    return {};
}

QList<KoResourceLoadResult> KisMyPaintOpFactory::prepareEmbeddedResources(const KisPaintOpSettingsSP settings, KisResourcesInterfaceSP resourcesInterface)
{
    Q_UNUSED(settings)
    Q_UNUSED(resourcesInterface);

    return {};
}

bool KisMyPaintOpFactory::lodSizeThresholdSupported() const
{
    return true;
}
