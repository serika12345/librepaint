/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDocumentProofing_p.h"
#include <KisProofingConfiguration.h>
#include <kis_image_config.h>
#include <kis_dom_utils.h>
#include <KoColorProfile.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <QDomDocument>

namespace KisDocumentProofing {
namespace Kra = LibrePaint::ImportExport::Kra;

KisProofingConfigurationSP read(const Kra::ProofingMetadata &metadata)
{
    const auto &values = metadata.attributes;
    if (!values.contains(QStringLiteral("proofing-profile-name")) && metadata.profile.isEmpty()) return {};
    auto configuration = KisImageConfig(true).defaultProofingconfiguration();
    const auto stringValue = [&](const QString &name, QString &value) {
        if (values.contains(name)) value = values.value(name);
    };
    stringValue(QStringLiteral("proofing-profile-name"), configuration->proofingProfile);
    stringValue(QStringLiteral("proofing-model"), configuration->proofingModel);
    stringValue(QStringLiteral("proofing-depth"), configuration->proofingDepth);
    const auto intentValue = [&](const QString &name, KoColorConversionTransformation::Intent &value) {
        if (values.contains(name)) value = KoColorConversionTransformation::Intent(KisDomUtils::toInt(values.value(name)));
    };
    intentValue(QStringLiteral("proofing-intent"), configuration->conversionIntent);
    intentValue(QStringLiteral("proofing-display-intent"), configuration->displayIntent);
    if (values.contains(QStringLiteral("proofing-blackpoint-compensation")))
        configuration->useBlackPointCompensationFirstTransform = values.value(QStringLiteral("proofing-blackpoint-compensation")) == QLatin1String("true");
    if (values.contains(QStringLiteral("proofing-display-blackpoint-compensation")))
        configuration->displayFlags.setFlag(KoColorConversionTransformation::BlackpointCompensation,
            values.value(QStringLiteral("proofing-display-blackpoint-compensation")) == QLatin1String("true"));
    if (values.contains(QStringLiteral("proofing-display-mode"))) {
        const auto mode = values.value(QStringLiteral("proofing-display-mode"));
        configuration->displayMode = mode == QLatin1String("monitor") ? KisProofingConfiguration::Monitor :
            mode == QLatin1String("paper") ? KisProofingConfiguration::Paper : KisProofingConfiguration::Custom;
    }
    if (values.contains(QStringLiteral("proofing-adaptation-state")))
        configuration->setLegacyAdaptationState(KisDomUtils::toDouble(values.value(QStringLiteral("proofing-adaptation-state"))));
    if (!metadata.warningColor.isEmpty()) {
        QDomDocument warning;
        warning.setContent(metadata.warningColor);
        configuration->warningColor = KoColor::fromXML(warning.documentElement().firstChildElement(), Integer8BitsColorDepthID.id());
    }
    if (!metadata.profile.isEmpty())
        KoColorSpaceRegistry::instance()->createColorProfileFromData(configuration->proofingModel, configuration->proofingDepth, metadata.profile);
    return configuration;
}

Kra::ProofingMetadata write(const KisProofingConfigurationSP &configuration,
                          const std::optional<Kra::ProofingMetadata> &retained)
{
    Kra::ProofingMetadata metadata;
    if (!configuration) return metadata;
    metadata.attributes = {
        {QStringLiteral("proofing-profile-name"), configuration->proofingProfile},
        {QStringLiteral("proofing-model"), configuration->proofingModel},
        {QStringLiteral("proofing-depth"), configuration->proofingDepth},
        {QStringLiteral("proofing-intent"), KisDomUtils::toString(configuration->conversionIntent)},
        {QStringLiteral("proofing-display-intent"), KisDomUtils::toString(configuration->displayIntent)},
        {QStringLiteral("proofing-blackpoint-compensation"), configuration->useBlackPointCompensationFirstTransform ? QStringLiteral("true") : QStringLiteral("false")},
        {QStringLiteral("proofing-display-blackpoint-compensation"), configuration->displayFlags.testFlag(KoColorConversionTransformation::BlackpointCompensation) ? QStringLiteral("true") : QStringLiteral("false")},
        {QStringLiteral("proofing-display-mode"), configuration->displayMode == KisProofingConfiguration::Monitor ? QStringLiteral("monitor") :
            configuration->displayMode == KisProofingConfiguration::Paper ? QStringLiteral("paper") : QStringLiteral("custom")},
        {QStringLiteral("proofing-adaptation-state"), KisDomUtils::toString(configuration->legacyAdaptationState())}
    };
    QDomDocument warning;
    auto element = warning.createElement(QStringLiteral("ProofingWarningColor"));
    warning.appendChild(element);
    configuration->warningColor.toXML(warning, element);
    metadata.warningColor = warning.toByteArray();
    // Keep the embedded encoding when the artist keeps the same profile,
    // including a host profile with the same name and different bytes.
    if (retained && !retained->profile.isEmpty() &&
        retained->attributes.value(QStringLiteral("proofing-profile-name")) == configuration->proofingProfile &&
        retained->attributes.value(QStringLiteral("proofing-model")) == configuration->proofingModel &&
        retained->attributes.value(QStringLiteral("proofing-depth")) == configuration->proofingDepth) {
        metadata.profile = retained->profile;
    } else if (const auto profile = KoColorSpaceRegistry::instance()->profileByName(configuration->proofingProfile)) {
        if (profile->valid()) metadata.profile = profile->rawData();
    }
    return metadata;
}
}
