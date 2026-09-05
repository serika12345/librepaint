/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KoPattern.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PATTERN_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPattern::method)), signature>)
} // namespace

class KoPatternSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void patternTypeLifetimeAndValueSemanticsSchemaRemainStable();
    void patternConstructionAndCloneSchemaRemainStable();
    void patternPersistenceSignaturesRemainStable();
    void patternImageAndGeometrySignaturesRemainStable();
    void patternClassificationAndAlphaSignaturesRemainStable();
};

void KoPatternSchemaContractTest::patternTypeLifetimeAndValueSemanticsSchemaRemainStable()
{
    static_assert(std::is_same_v<KoPatternSP, QSharedPointer<KoPattern>>);
    static_assert(std::is_class_v<KoPattern>);
    static_assert(std::is_base_of_v<KoResource, KoPattern>);
    static_assert(std::is_constructible_v<KoPattern, const KoPattern &>);
    static_assert(!std::is_copy_assignable_v<KoPattern>);
    static_assert(std::has_virtual_destructor_v<KoPattern>);
}

void KoPatternSchemaContractTest::patternConstructionAndCloneSchemaRemainStable()
{
    static_assert(std::is_constructible_v<KoPattern, const QString &>);
    static_assert(std::is_constructible_v<KoPattern, const QImage &, const QString &, const QString &>);
    static_assert(std::is_same_v<decltype(KoPattern(std::declval<const QString &>())), KoPattern>);
    static_assert(std::is_same_v<decltype(KoPattern(std::declval<const QImage &>(),
                                                    std::declval<const QString &>(),
                                                    std::declval<const QString &>())),
                                 KoPattern>);

    ASSERT_PATTERN_SIGNATURE(clone, KoResourceSP (KoPattern::*)() const);
    ASSERT_PATTERN_SIGNATURE(cloneWithoutAlpha, KoPatternSP (KoPattern::*)() const);
}

void KoPatternSchemaContractTest::patternPersistenceSignaturesRemainStable()
{
    ASSERT_PATTERN_SIGNATURE(defaultFileExtension, QString (KoPattern::*)() const);
    ASSERT_PATTERN_SIGNATURE(loadFromDevice, bool (KoPattern::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_PATTERN_SIGNATURE(loadPatFromDevice, bool (KoPattern::*)(QIODevice *));
    ASSERT_PATTERN_SIGNATURE(savePatToDevice, bool (KoPattern::*)(QIODevice *) const);
    ASSERT_PATTERN_SIGNATURE(saveToDevice, bool (KoPattern::*)(QIODevice *) const);
}

void KoPatternSchemaContractTest::patternImageAndGeometrySignaturesRemainStable()
{
    ASSERT_PATTERN_SIGNATURE(height, qint32 (KoPattern::*)() const);
    ASSERT_PATTERN_SIGNATURE(pattern, QImage (KoPattern::*)() const);
    ASSERT_PATTERN_SIGNATURE(width, qint32 (KoPattern::*)() const);
}

void KoPatternSchemaContractTest::patternClassificationAndAlphaSignaturesRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KoPattern::*)() const;

    ASSERT_PATTERN_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_PATTERN_SIGNATURE(hasAlpha, bool (KoPattern::*)() const);
}

QTEST_GUILESS_MAIN(KoPatternSchemaContractTest)

#include "KoPatternSchemaContractTest.moc"
