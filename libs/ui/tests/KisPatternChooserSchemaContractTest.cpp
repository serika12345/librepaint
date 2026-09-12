/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/kis_pattern_chooser.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PATTERN_CHOOSER_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPatternChooser::method)), signature>)
} // namespace

class KisPatternChooserSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void patternChooserTypeConstructionAndLifetimeSchemaRemainStable();
    void patternChooserSelectionSchemaRemainStable();
    void patternChooserPreviewAndNotificationSchemaRemainStable();
};

void KisPatternChooserSchemaContractTest::patternChooserTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPatternChooser>);
    static_assert(std::is_base_of_v<QFrame, KisPatternChooser>);
    static_assert(std::is_constructible_v<KisPatternChooser, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisPatternChooser>);

    QVERIFY(true);
}

void KisPatternChooserSchemaContractTest::patternChooserSelectionSchemaRemainStable()
{
    ASSERT_PATTERN_CHOOSER_SIGNATURE(currentResource, KoResourceSP (KisPatternChooser::*)(bool));
    ASSERT_PATTERN_CHOOSER_SIGNATURE(setCurrentPattern, void (KisPatternChooser::*)(KoResourceSP));
    ASSERT_PATTERN_CHOOSER_SIGNATURE(setCurrentItem, void (KisPatternChooser::*)(int));

    QVERIFY(true);
}

void KisPatternChooserSchemaContractTest::patternChooserPreviewAndNotificationSchemaRemainStable()
{
    ASSERT_PATTERN_CHOOSER_SIGNATURE(setGrayscalePreview, void (KisPatternChooser::*)(bool));
    ASSERT_PATTERN_CHOOSER_SIGNATURE(setPreviewOrientation, void (KisPatternChooser::*)(Qt::Orientation));
    ASSERT_PATTERN_CHOOSER_SIGNATURE(resourceSelected, void (KisPatternChooser::*)(KoResourceSP));
    ASSERT_PATTERN_CHOOSER_SIGNATURE(updateItemSize, void (KisPatternChooser::*)());

    QVERIFY(true);
}

#undef ASSERT_PATTERN_CHOOSER_SIGNATURE

QTEST_APPLESS_MAIN(KisPatternChooserSchemaContractTest)

#include "KisPatternChooserSchemaContractTest.moc"
