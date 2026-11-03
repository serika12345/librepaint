/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <compositeops/KoCompositeOpFunctions.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (argc != 3) return 2;
    struct Formula { const char *id; quint8 (*channel)(quint8, quint8); };
    const Formula formulas[] = {
        {"geometric_mean", CFGeometricMean<quint8>::composeChannel},
        {"interpolation", cfInterpolation<quint8>},
        {"interpolation 2x", cfInterpolationB<quint8>},
        {"penumbra c", cfPenumbraC<quint8>},
        {"penumbra d", cfPenumbraD<quint8>},
        {"soft_light_ifs_illusions", cfSoftLightIFSIllusions<quint8>},
        {"soft_light_svg", CFSoftLightSvg<quint8>::composeChannel},
        {"soft_light", CFSoftLight<quint8>::composeChannel},
        {"gamma_light", CFGammaLight<quint8>::composeChannel},
        {"gamma_illumination", CFGammaIllumination<quint8>::composeChannel},
        {"pnorm_a", cfPNormA<quint8>},
        {"pnorm_b", cfPNormB<quint8>},
        {"super_light", cfSuperLight<quint8>},
        {"tint_ifs_illusions", cfTintIFSIllusions<quint8>},
        {"fog_lighten_ifs_illusions", cfFogLightenIFSIllusions<quint8>},
        {"easy dodge", cfEasyDodge<quint8>},
        {"gamma_dark", CFGammaDark<quint8>::composeChannel},
        {"shade_ifs_illusions", cfShadeIFSIllusions<quint8>},
        {"fog_darken_ifs_illusions", cfFogDarkenIFSIllusions<quint8>},
        {"easy burn", cfEasyBurn<quint8>},
        {"modulo", cfModulo<quint8>},
        {"modulo_continuous", cfModuloContinuous<quint8>},
        {"divisive_modulo", cfDivisiveModulo<quint8>},
        {"divisive_modulo_continuous", cfDivisiveModuloContinuous<quint8>},
        {"modulo_shift", cfModuloShift<quint8>},
        {"modulo_shift_continuous", cfModuloShiftContinuous<quint8>},
        {"arc_tangent", cfArcTangent<quint8>},
        {"additive_subtractive", CFAdditiveSubtractive<quint8>::composeChannel},
    };
    constexpr int count = sizeof(formulas) / sizeof(formulas[0]);
    QByteArray bytes(((count + 3) / 4) * 256 * 256 * 4, '\0');
    QJsonArray ids;
    for (int mode = 0; mode < count; ++mode) {
        ids.append(QString::fromLatin1(formulas[mode].id));
        for (int source = 0; source < 256; ++source) for (int destination = 0; destination < 256; ++destination)
            bytes[((mode / 4) * 65536 + source * 256 + destination) * 4 + mode % 4] =
                char(formulas[mode].channel(quint8(source), quint8(destination)));
    }
    const QDir directory(QString::fromLocal8Bit(argv[1]));
    if (!directory.mkpath(QStringLiteral("."))) return 3;
    QFile output(directory.filePath(QStringLiteral("rgba8-nonlinear.bin")));
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()) return 4;
    output.close();
    const QJsonObject manifest{
        {QStringLiteral("formatVersion"), 1},
        {QStringLiteral("purpose"), QStringLiteral("Exact finite RGBA8 separable channel values for GPU editing and implicit defaults")},
        {QStringLiteral("sourceRevision"), QString::fromLatin1(argv[2])},
        {QStringLiteral("source"), QStringLiteral("libs/pigment/compositeops/KoCompositeOpFunctions.h")},
        {QStringLiteral("sourceAuthor"), QStringLiteral("Silvio Heinrich, Cyrille Berger, Adrian Page, and LibrePaint/Krita contributors")},
        {QStringLiteral("sourceLicense"), QStringLiteral("LGPL-2.1-or-later")},
        {QStringLiteral("packing"), QStringLiteral("RGBA8Uint 256x256 array; x=destination, y=source, layer=mode/4, component=mode%4")},
        {QStringLiteral("modes"), ids},
        {QStringLiteral("bytes"), bytes.size()},
        {QStringLiteral("sha256"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}
    };
    QFile description(directory.filePath(QStringLiteral("rgba8-nonlinear.json")));
    const auto json = QJsonDocument(manifest).toJson();
    if (!description.open(QIODevice::WriteOnly) || description.write(json) != json.size()) return 5;
    std::printf("Generated %d modes, %lld bytes\n", count, static_cast<long long>(bytes.size()));
    return 0;
}
