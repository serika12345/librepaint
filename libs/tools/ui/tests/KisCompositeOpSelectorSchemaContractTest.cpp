/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_categorized_list_view.h"
#include "kis_cmb_composite.h"

#include <QTest>

#include <type_traits>

#define ASSERT_COMPOSITE_SELECTOR_SIGNATURE(type, method, signature)                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class KisCompositeOpSelectorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compositeOpComboBoxTypeLifetimeAndConstructionSchemaRemainStable();
    void compositeOpComboBoxSelectionAndSpecializationSignaturesRemainStable();
    void compositeOpComboBoxPopupAndInputSignaturesRemainStable();
    void compositeOpListWidgetTypeLifetimeAndConstructionSchemaRemainStable();
    void compositeOpListWidgetSelectionSignaturesRemainStable();
    void categorizedListViewTypeConstructionAndLifetimeSchemaRemainStable();
    void categorizedListViewModelDisplayAndControlSignaturesRemainStable();
    void categorizedListViewNotificationSignaturesRemainStable();
};

void KisCompositeOpSelectorSchemaContractTest::compositeOpComboBoxTypeLifetimeAndConstructionSchemaRemainStable()
{
    using ComboBox = KisCompositeOpComboBox;
    using LayerStyleComboBox = KisLayerStyleCompositeOpComboBox;

    static_assert(std::is_class_v<ComboBox>);
    static_assert(std::is_base_of_v<KisSqueezedComboBox, ComboBox>);
    static_assert(std::is_class_v<LayerStyleComboBox>);
    static_assert(std::is_base_of_v<ComboBox, LayerStyleComboBox>);
    static_assert(std::is_default_constructible_v<ComboBox>);
    static_assert(std::is_constructible_v<ComboBox, QWidget *>);
    static_assert(std::is_constructible_v<ComboBox, bool>);
    static_assert(std::is_constructible_v<ComboBox, bool, QWidget *>);
    static_assert(std::has_virtual_destructor_v<ComboBox>);
}

void KisCompositeOpSelectorSchemaContractTest::compositeOpComboBoxSelectionAndSpecializationSignaturesRemainStable()
{
    using ComboBox = KisCompositeOpComboBox;
    using LayerStyleComboBox = KisLayerStyleCompositeOpComboBox;

    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, selectCompositeOp, void (ComboBox::*)(const KoID &));
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, selectedCompositeOp, KoID (ComboBox::*)() const);
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, validate, void (ComboBox::*)(const KoColorSpace *));
    static_assert(std::is_default_constructible_v<LayerStyleComboBox>);
    static_assert(std::is_constructible_v<LayerStyleComboBox, QWidget *>);
}

void KisCompositeOpSelectorSchemaContractTest::compositeOpComboBoxPopupAndInputSignaturesRemainStable()
{
    using ComboBox = KisCompositeOpComboBox;

    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, hidePopup, void (ComboBox::*)());
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, keyPressEvent, void (ComboBox::*)(QKeyEvent *));
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ComboBox, wheelEvent, void (ComboBox::*)(QWheelEvent *));
}

void KisCompositeOpSelectorSchemaContractTest::compositeOpListWidgetTypeLifetimeAndConstructionSchemaRemainStable()
{
    using ListWidget = KisCompositeOpListWidget;

    static_assert(std::is_class_v<ListWidget>);
    static_assert(std::is_base_of_v<KisCategorizedListView, ListWidget>);
    static_assert(std::is_default_constructible_v<ListWidget>);
    static_assert(std::is_constructible_v<ListWidget, QWidget *>);
    static_assert(std::has_virtual_destructor_v<ListWidget>);
}

void KisCompositeOpSelectorSchemaContractTest::compositeOpListWidgetSelectionSignaturesRemainStable()
{
    using ListWidget = KisCompositeOpListWidget;

    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListWidget, selectedCompositeOp, KoID (ListWidget::*)() const);
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListWidget, setCompositeOp, void (ListWidget::*)(const KoID &));
}

void KisCompositeOpSelectorSchemaContractTest::categorizedListViewTypeConstructionAndLifetimeSchemaRemainStable()
{
    using ListView = KisCategorizedListView;

    static_assert(std::is_class_v<ListView>);
    static_assert(std::is_base_of_v<QListView, ListView>);
    static_assert(std::is_constructible_v<ListView, QWidget *>);
    static_assert(std::has_virtual_destructor_v<ListView>);
}

void KisCompositeOpSelectorSchemaContractTest::categorizedListViewModelDisplayAndControlSignaturesRemainStable()
{
    using ListView = KisCategorizedListView;

    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, setModel, void (ListView::*)(QAbstractItemModel *));
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, sizeHint, QSize (ListView::*)() const);
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, setCompositeBoxControl, void (ListView::*)(bool));
}

void KisCompositeOpSelectorSchemaContractTest::categorizedListViewNotificationSignaturesRemainStable()
{
    using ListView = KisCategorizedListView;

    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, sigCategoryToggled, void (ListView::*)(const QModelIndex &, bool));
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, sigEntryChecked, void (ListView::*)(const QModelIndex &));
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, rightClickedMenuDropSettingsTriggered, void (ListView::*)());
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, rightClickedMenuSaveSettingsTriggered, void (ListView::*)());
    ASSERT_COMPOSITE_SELECTOR_SIGNATURE(ListView, lockAreaTriggered, void (ListView::*)(const QModelIndex &));
}

QTEST_GUILESS_MAIN(KisCompositeOpSelectorSchemaContractTest)
#include "KisCompositeOpSelectorSchemaContractTest.moc"
