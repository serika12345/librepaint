/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShape.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_SHAPE_STATE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShape::method)), signature>)
} // namespace

class KoShapeStateSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeIdentityAndLinkSignaturesRemainStable();
    void shapeAdditionalAttributeSignaturesRemainStable();
    void shapeProtectionSignaturesRemainStable();
    void shapePresentationPolicySignaturesRemainStable();
    void shapeEditabilityAndReadinessSignaturesRemainStable();
};

void KoShapeStateSchemaContractTest::shapeIdentityAndLinkSignaturesRemainStable()
{
    ASSERT_SHAPE_STATE_SIGNATURE(name, QString (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setName, void (KoShape::*)(const QString &));
    ASSERT_SHAPE_STATE_SIGNATURE(shapeId, QString (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setShapeId, void (KoShape::*)(const QString &));
    ASSERT_SHAPE_STATE_SIGNATURE(hyperLink, QString (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setHyperLink, void (KoShape::*)(const QString &));
}

void KoShapeStateSchemaContractTest::shapeAdditionalAttributeSignaturesRemainStable()
{
    ASSERT_SHAPE_STATE_SIGNATURE(additionalAttribute, QString (KoShape::*)(const QString &) const);
    ASSERT_SHAPE_STATE_SIGNATURE(hasAdditionalAttribute, bool (KoShape::*)(const QString &) const);
    ASSERT_SHAPE_STATE_SIGNATURE(removeAdditionalAttribute, void (KoShape::*)(const QString &));
    ASSERT_SHAPE_STATE_SIGNATURE(removeAdditionalStyleAttribute, void (KoShape::*)(const char *));
    ASSERT_SHAPE_STATE_SIGNATURE(setAdditionalAttribute, void (KoShape::*)(const QString &, const QString &));
    ASSERT_SHAPE_STATE_SIGNATURE(setAdditionalStyleAttribute, void (KoShape::*)(const char *, const QString &));
}

void KoShapeStateSchemaContractTest::shapeProtectionSignaturesRemainStable()
{
    ASSERT_SHAPE_STATE_SIGNATURE(isContentProtected, bool (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setContentProtected, void (KoShape::*)(bool));
    ASSERT_SHAPE_STATE_SIGNATURE(isGeometryProtected, bool (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setGeometryProtected, void (KoShape::*)(bool));
}

void KoShapeStateSchemaContractTest::shapePresentationPolicySignaturesRemainStable()
{
    ASSERT_SHAPE_STATE_SIGNATURE(isPrintable, bool (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setPrintable, void (KoShape::*)(bool));
    ASSERT_SHAPE_STATE_SIGNATURE(isSelectable, bool (KoShape::*)() const);
    ASSERT_SHAPE_STATE_SIGNATURE(setSelectable, void (KoShape::*)(bool));
    ASSERT_SHAPE_STATE_SIGNATURE(isVisible, bool (KoShape::*)(bool) const);
    ASSERT_SHAPE_STATE_SIGNATURE(setVisible, void (KoShape::*)(bool));

    static_assert(std::is_same_v<decltype(std::declval<const KoShape &>().isVisible()), bool>);
}

void KoShapeStateSchemaContractTest::shapeEditabilityAndReadinessSignaturesRemainStable()
{
    ASSERT_SHAPE_STATE_SIGNATURE(isShapeEditable, bool (KoShape::*)(bool) const);
    ASSERT_SHAPE_STATE_SIGNATURE(waitUntilReady, void (KoShape::*)(bool) const);

    static_assert(std::is_same_v<decltype(std::declval<const KoShape &>().isShapeEditable()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KoShape &>().waitUntilReady()), void>);
}

QTEST_APPLESS_MAIN(KoShapeStateSchemaContractTest)

#include "KoShapeStateSchemaContractTest.moc"
