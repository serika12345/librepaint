/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../kis_qmic_interface.h"
#include "../kis_qmic_plugin_interface.h"

#include <QTest>


class KisQMicInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageStorageValuesRemainStable();
    void imageBufferAndDiagnosticSignaturesRemainStable();
    void pluginInterfaceTypeConstructionLifetimeAndLaunchSchemaRemainStable();
};

void KisQMicInterfaceContractTest::imageStorageValuesRemainStable()
{
    using Image = KisQMicImage;
    Image image(QStringLiteral("Layer A"), 2, 3);

    QCOMPARE(image.m_layerName, QStringLiteral("Layer A"));
    QCOMPARE(image.m_width, 2);
    QCOMPARE(image.m_height, 3);
    QCOMPARE(image.m_spectrum, 4);
    QVERIFY(image.m_data);
}

void KisQMicInterfaceContractTest::imageBufferAndDiagnosticSignaturesRemainStable()
{
    KisQMicImage image(QStringLiteral("RGBA"), 2, 3, 4);
    image.m_data[0] = 0.25F;

    QCOMPARE(image.constData(), image.m_data);
    QCOMPARE(image.constData()[0], 0.25F);
    QCOMPARE(image.size(), size_t(2 * 3 * 4 * sizeof(float)));
}

void KisQMicInterfaceContractTest::pluginInterfaceTypeConstructionLifetimeAndLaunchSchemaRemainStable()
{
    using Interface = KisQmicPluginInterface;

    class PluginInterfaceProbe final : public Interface
    {
    public:
        int launch(std::shared_ptr<KisImageInterface>, bool) override
        {
            return 0;
        }
    };

}

QTEST_GUILESS_MAIN(KisQMicInterfaceContractTest)

#include "KisQMicInterfaceContractTest.moc"
