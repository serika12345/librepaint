/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceUiDescriptor.h>

#include <QTest>

class KisResourceUiDescriptorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesResourceTypeAndPreviewState();
};

void KisResourceUiDescriptorContractTest::preservesResourceTypeAndPreviewState()
{
    const KisResourceUiDescriptor defaultDescriptor(QStringLiteral("paintoppresets"));
    QCOMPARE(defaultDescriptor.resourceType(), QStringLiteral("paintoppresets"));
    QVERIFY(!defaultDescriptor.previewEnabled());

    const KisResourceUiDescriptor previewDescriptor(QStringLiteral("patterns"), true);
    QCOMPARE(previewDescriptor.resourceType(), QStringLiteral("patterns"));
    QVERIFY(previewDescriptor.previewEnabled());
}

QTEST_GUILESS_MAIN(KisResourceUiDescriptorContractTest)

#include "KisResourceUiDescriptorContractTest.moc"
