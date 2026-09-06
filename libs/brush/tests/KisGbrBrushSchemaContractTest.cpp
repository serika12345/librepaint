/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_gbr_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_GBR_SIGNATURE(method, signature)                                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGbrBrush::method)), signature>)
} // namespace

class KisGbrBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void resourceIoSchemaRemainsStable();
    void maskImageSignatureRemainsStable();
};

void KisGbrBrushSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGbrBrush>);
    static_assert(std::is_same_v<KisGbrBrushSP, QSharedPointer<KisGbrBrush>>);
    static_assert(std::is_constructible_v<KisGbrBrush, const QString &>);
    static_assert(std::is_constructible_v<KisGbrBrush, const QString &, const QByteArray &, qint32 &>);
    static_assert(std::is_constructible_v<KisGbrBrush, KisPaintDeviceSP, int, int, int, int>);
    static_assert(std::is_constructible_v<KisGbrBrush, const QImage &>);
    static_assert(std::is_constructible_v<KisGbrBrush, const QImage &, const QString &>);
    static_assert(std::is_copy_constructible_v<KisGbrBrush>);
    static_assert(std::has_virtual_destructor_v<KisGbrBrush>);
    static_assert(std::is_copy_assignable_v<KisGbrBrush>);
}

void KisGbrBrushSchemaContractTest::resourceIoSchemaRemainsStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisGbrBrush::*)() const;

    ASSERT_GBR_SIGNATURE(clone, KoResourceSP (KisGbrBrush::*)() const);
    ASSERT_GBR_SIGNATURE(defaultFileExtension, QString (KisGbrBrush::*)() const);
    ASSERT_GBR_SIGNATURE(loadFromDevice, bool (KisGbrBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_GBR_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_GBR_SIGNATURE(saveToDevice, bool (KisGbrBrush::*)(QIODevice *) const);
}

void KisGbrBrushSchemaContractTest::maskImageSignatureRemainsStable()
{
    ASSERT_GBR_SIGNATURE(makeMaskImage, void (KisGbrBrush::*)(bool));
}

QTEST_GUILESS_MAIN(KisGbrBrushSchemaContractTest)

#include "KisGbrBrushSchemaContractTest.moc"
