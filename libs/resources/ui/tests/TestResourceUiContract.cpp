/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisResourceItemChooser.h>
#include <KisResourceTypes.h>
#include <KisResourceUiDescriptor.h>

class TestResourceUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void descriptorPreservesCatalogIdentity();
    void chooserUsesDescriptor();
};

void TestResourceUiContract::descriptorPreservesCatalogIdentity()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Palettes, true);

    QCOMPARE(descriptor.resourceType(), ResourceType::Palettes);
    QVERIFY(descriptor.previewEnabled());
}

void TestResourceUiContract::chooserUsesDescriptor()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Patterns, false);
    KisResourceItemChooser chooser(descriptor);

    QCOMPARE(chooser.descriptor().resourceType(), ResourceType::Patterns);
    QVERIFY(!chooser.descriptor().previewEnabled());
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
