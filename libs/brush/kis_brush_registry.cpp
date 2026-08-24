/*
 *  SPDX-FileCopyrightText: 2008 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_brush_registry.h"

#include <QString>

#include <QGlobalStatic>
#include <klocalizedstring.h>

#include <KoPluginLoader.h>
#include <KisResourceLoader.h>
#include <KisResourceLoaderRegistry.h>
#include <KisResourceTypes.h>

#include <kis_debug.h>

#include "KoResourceServer.h"
#include "kis_auto_brush_factory.h"
#include "kis_text_brush_factory.h"
#include "kis_predefined_brush_factory.h"
#include "KisBrushTypeMetaDataFixup.h"
#include "kis_gbr_brush.h"
#include "kis_imagepipe_brush.h"
#include "kis_png_brush.h"
#include "kis_svg_brush.h"

Q_GLOBAL_STATIC(KisBrushRegistry, s_instance)


KisBrushRegistry::KisBrushRegistry()
{
}

KisBrushRegistry::~KisBrushRegistry()
{
    Q_FOREACH (const QString & id, keys()) {
        delete get(id);
    }
    dbgRegistry << "deleting KisBrushRegistry";
}

KisBrushRegistry* KisBrushRegistry::instance()
{
    if (!s_instance.exists()) {
        s_instance->add(new KisAutoBrushFactory());
        s_instance->add(new KisPredefinedBrushFactory("gbr_brush"));
        s_instance->add(new KisPredefinedBrushFactory("abr_brush"));
        s_instance->add(new KisTextBrushFactory());
        s_instance->add(new KisPredefinedBrushFactory("png_brush"));
        s_instance->add(new KisPredefinedBrushFactory("svg_brush"));
    }
    return s_instance;
}

void KisBrushRegistry::registerResourceLoaders(KisResourceLoaderRegistry &registry)
{
    registry.registerLoader(new KisResourceLoader<KisGbrBrush>(
        ResourceSubType::GbrBrushes,
        ResourceType::Brushes,
        i18n("Brush tips"),
        QStringList() << "image/x-gimp-brush"));
    registry.registerLoader(new KisResourceLoader<KisImagePipeBrush>(
        ResourceSubType::GihBrushes,
        ResourceType::Brushes,
        i18n("Brush tips"),
        QStringList() << "image/x-gimp-brush-animated"));
    registry.registerLoader(new KisResourceLoader<KisSvgBrush>(
        ResourceSubType::SvgBrushes,
        ResourceType::Brushes,
        i18n("Brush tips"),
        QStringList() << "image/svg+xml"));
    registry.registerLoader(new KisResourceLoader<KisPngBrush>(
        ResourceSubType::PngBrushes,
        ResourceType::Brushes,
        i18n("Brush tips"),
        QStringList() << "image/png"));
}

void KisBrushRegistry::registerResourceCacheFixup(KisResourceLoaderRegistry &registry)
{
    registry.registerFixup(10, new KisBrushTypeMetaDataFixup());
}


KoResourceLoadResult KisBrushRegistry::createBrush(const QDomElement& element, KisResourcesInterfaceSP resourcesInterface)
{
    QString brushType = element.attribute("type");

    if (brushType.isEmpty()) {
        return KoResourceSignature(ResourceType::Brushes, "", "unknown", "unknown");
    }

    KisBrushFactory *factory = get(brushType);
    if (!factory) {
        return KoResourceSignature(ResourceType::Brushes, "", "unknown", "unknown");
    }

    return factory->createBrush(element, resourcesInterface);
}

KoResourceLoadResult KisBrushRegistry::createBrush(const KisBrushModel::BrushData &data, KisResourcesInterfaceSP resourcesInterface)
{
    QDomDocument doc;
    QDomElement element = doc.createElement("brush_definition");
    toXML(doc, element, data);
    return createBrush(element, resourcesInterface);
}

std::optional<KisBrushModel::BrushData> KisBrushRegistry::createBrushModel(const QDomElement& element, KisResourcesInterfaceSP resourcesInterface)
{
    QString brushType = element.attribute("type");

    if (brushType.isEmpty()) {
        return std::nullopt;
    }

    KisBrushFactory *factory = get(brushType);

    if (!factory) {
        return std::nullopt;
    }

    return factory->createBrushModel(element, resourcesInterface);
}

void KisBrushRegistry::toXML(QDomDocument &doc, QDomElement &element, const KisBrushModel::BrushData &model)
{
    QString brushType;

    if (model.type == KisBrushModel::Auto) {
        brushType = "auto_brush";
    } else if (model.type == KisBrushModel::Text) {
        brushType = "kis_text_brush";
    } else {
        brushType = model.predefinedBrush.subtype;
    }

    KIS_SAFE_ASSERT_RECOVER_RETURN(!brushType.isEmpty());

    KisBrushFactory *factory = get(brushType);
    KIS_SAFE_ASSERT_RECOVER_RETURN(factory);

    factory->toXML(doc, element, model);
}
