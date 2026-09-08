/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/kis_popup_palette.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_POPUP_PALETTE_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPopupPalette::method)), signature>)
} // namespace

class KisPopupPaletteSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cachedPresetLayoutSchemaRemainsStable();
    void popupPaletteTypeLifetimeAndSizeSchemaRemainStable();
    void popupPaletteSelectionParentAndPlacementSignaturesRemainStable();
    void popupPaletteVisibilityLifecycleSignaturesRemainStable();
    void popupPaletteNotificationAndRefreshSignaturesRemainStable();
};

void KisPopupPaletteSchemaContractTest::cachedPresetLayoutSchemaRemainsStable()
{
    using Layout = CachedPresetLayout;

    static_assert(std::is_aggregate_v<Layout>);
    static_assert(std::is_same_v<decltype(Layout::ringCount), int>);
    static_assert(std::is_same_v<decltype(Layout::firstRowRadius), qreal>);
    static_assert(std::is_same_v<decltype(Layout::secondRowRadius), qreal>);
    static_assert(std::is_same_v<decltype(Layout::thirdRowRadius), qreal>);
    static_assert(std::is_same_v<decltype(Layout::firstRowPos), qreal>);
    static_assert(std::is_same_v<decltype(Layout::secondRowPos), qreal>);
    static_assert(std::is_same_v<decltype(Layout::thirdRowPos), qreal>);

    const Layout layout;
    QCOMPARE(layout.ringCount, 1);
    QCOMPARE(layout.firstRowRadius, 0.0);
    QCOMPARE(layout.secondRowRadius, 0.0);
    QCOMPARE(layout.thirdRowRadius, 0.0);
    QCOMPARE(layout.firstRowPos, 0.0);
    QCOMPARE(layout.secondRowPos, 0.0);
    QCOMPARE(layout.thirdRowPos, 0.0);
}

void KisPopupPaletteSchemaContractTest::popupPaletteTypeLifetimeAndSizeSchemaRemainStable()
{
    using Palette = KisPopupPalette;

    static_assert(std::is_class_v<Palette>);
    static_assert(std::is_base_of_v<QWidget, Palette>);
    static_assert(std::is_base_of_v<KisPopupWidgetInterface, Palette>);
    static_assert(std::is_constructible_v<Palette,
                                          KisViewManager *,
                                          KisCoordinatesConverter *,
                                          KisFavoriteResourceManager *,
                                          const KoColorDisplayRendererInterface *,
                                          QWidget *>);
    static_assert(std::has_virtual_destructor_v<Palette>);
    ASSERT_POPUP_PALETTE_SIGNATURE(sizeHint, QSize (Palette::*)() const);

    QVERIFY(true);
}

// clang-format off
void KisPopupPaletteSchemaContractTest::popupPaletteSelectionParentAndPlacementSignaturesRemainStable()
// clang-format on
{
    using Palette = KisPopupPalette;

    ASSERT_POPUP_PALETTE_SIGNATURE(selectedColor, int (Palette::*)() const);
    ASSERT_POPUP_PALETTE_SIGNATURE(setSelectedColor, void (Palette::*)(int));
    ASSERT_POPUP_PALETTE_SIGNATURE(setParent, void (Palette::*)(QWidget *));
    ASSERT_POPUP_PALETTE_SIGNATURE(ensureWithinParent, void (Palette::*)(const QPoint &, bool));

    QVERIFY(true);
}

void KisPopupPaletteSchemaContractTest::popupPaletteVisibilityLifecycleSignaturesRemainStable()
{
    using Palette = KisPopupPalette;

    ASSERT_POPUP_PALETTE_SIGNATURE(popup, void (Palette::*)(const QPoint &));
    ASSERT_POPUP_PALETTE_SIGNATURE(dismiss, void (Palette::*)());
    ASSERT_POPUP_PALETTE_SIGNATURE(onScreen, bool (Palette::*)());
    ASSERT_POPUP_PALETTE_SIGNATURE(finished, void (Palette::*)());

    QVERIFY(true);
}

// clang-format off
void KisPopupPaletteSchemaContractTest::popupPaletteNotificationAndRefreshSignaturesRemainStable()
// clang-format on
{
    using Palette = KisPopupPalette;

    ASSERT_POPUP_PALETTE_SIGNATURE(sigChangeActivePaintop, void (Palette::*)(int));
    ASSERT_POPUP_PALETTE_SIGNATURE(sigUpdateRecentColor, void (Palette::*)(int));
    ASSERT_POPUP_PALETTE_SIGNATURE(sigChangefGColor, void (Palette::*)(const KoColor &));
    ASSERT_POPUP_PALETTE_SIGNATURE(sigUpdateCanvas, void (Palette::*)());
    ASSERT_POPUP_PALETTE_SIGNATURE(zoomLevelChanged, void (Palette::*)(int));
    ASSERT_POPUP_PALETTE_SIGNATURE(slotUpdateIcons, void (Palette::*)());

    QVERIFY(true);
}

#undef ASSERT_POPUP_PALETTE_SIGNATURE

QTEST_APPLESS_MAIN(KisPopupPaletteSchemaContractTest)

#include "KisPopupPaletteSchemaContractTest.moc"
