/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_KRA_DOCUMENT_SETTINGS_P_H
#define KIS_GPU_KRA_DOCUMENT_SETTINGS_P_H
#include <document/KisDocument.h>
#include <canvas/kis_grid_config.h>
#include <canvas/kis_guides_config.h>
#include <canvas/KisMirrorAxisConfig.h>
#include <QDomDocument>
#include <optional>

namespace KisGpuKraDocumentSettings {
struct Values {
    std::optional<KisGridConfig> grid;
    std::optional<KisGuidesConfig> guides;
    std::optional<KisMirrorAxisConfig> mirror;

    void apply(KisDocument &document) const
    {
        if (grid) document.setGridConfig(*grid);
        if (guides) document.setGuidesConfig(*guides);
        if (mirror) document.setMirrorAxisConfig(*mirror);
    }
};

inline QByteArray capture(const KisDocument &document)
{
    QDomDocument xml;
    auto root = xml.createElement(QStringLiteral("document-settings"));
    xml.appendChild(root);
    const auto grid = document.gridConfig();
    const auto guides = document.guidesConfig();
    const auto mirror = document.mirrorAxisConfig();
    if (!grid.isDefault()) root.appendChild(grid.saveDynamicDataToXml(xml, QStringLiteral("grid")));
    if (!guides.isDefault()) root.appendChild(guides.saveToXml(xml, QStringLiteral("guides")));
    if (!mirror.isDefault()) root.appendChild(mirror.saveToXml(xml, QStringLiteral("mirror-axis")));
    return root.hasChildNodes() ? xml.toByteArray() : QByteArray{};
}

inline std::optional<Values> parse(const QByteArray &bytes)
{
    Values values;
    if (bytes.isEmpty()) return values;
    QDomDocument xml;
    if (!xml.setContent(bytes) || xml.documentElement().tagName() != QStringLiteral("document-settings")) return {};
    for (auto node = xml.documentElement().firstChildElement(); !node.isNull(); node = node.nextSiblingElement()) {
        if (node.tagName() == QStringLiteral("grid") && !values.grid) {
            values.grid.emplace();
            if (!values.grid->loadDynamicDataFromXml(node)) return {};
        } else if (node.tagName() == QStringLiteral("guides") && !values.guides) {
            values.guides.emplace();
            if (!values.guides->loadFromXml(node)) return {};
        } else if (node.tagName() == QStringLiteral("mirror-axis") && !values.mirror) {
            values.mirror.emplace();
            if (!values.mirror->loadFromXml(node)) return {};
        } else {
            return {};
        }
    }
    return values;
}
}
#endif
