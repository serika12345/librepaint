/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoFillConfigWidget.h>
#include <KoMarkerSelector.h>
#include <KoStrokeConfigWidget.h>
#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_STROKE_CONFIG_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoStrokeConfigWidget::method)), signature>)

#define ASSERT_SHAPE_APPEARANCE_SIGNATURE(owner, method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&owner::method)), signature>)

class KoStrokeConfigWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokeConfigTypeAndLifetimeSchemaRemainsStable();
    void strokeConfigAppearanceSchemaRemainsStable();
    void strokeConfigMarkerAndStrokeSchemaRemainsStable();
    void strokeConfigControlSchemaRemainsStable();
    void strokeConfigActivationAndNotificationSchemaRemainsStable();
    void fillConfigTypeLifetimeAndOutputSchemaRemainStable();
    void fillConfigActivationAndControlSchemaRemainStable();
    void fillConfigNotificationSignaturesRemainStable();
    void markerSelectorTypeLifetimeAndSelectionSchemaRemainStable();
    void markerSelectorPresentationSchemaRemainStable();
};

void KoStrokeConfigWidgetSchemaContractTest::strokeConfigTypeAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoStrokeConfigWidget>);
    static_assert(std::is_constructible_v<KoStrokeConfigWidget, KoCanvasBase *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KoStrokeConfigWidget>);
    static_assert(std::is_same_v<decltype(&KoStrokeConfigWidget::thicknessLabel), QLabel * KoStrokeConfigWidget::*>);
    static_assert(std::is_same_v<decltype(&KoStrokeConfigWidget::strokeStyleLabel), QLabel * KoStrokeConfigWidget::*>);
    static_assert(std::is_same_v<decltype(&KoStrokeConfigWidget::separatorLine), QFrame * KoStrokeConfigWidget::*>);
}

void KoStrokeConfigWidgetSchemaContractTest::strokeConfigAppearanceSchemaRemainsStable()
{
    ASSERT_STROKE_CONFIG_SIGNATURE(lineStyle, Qt::PenStyle (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(lineDashes, QVector<qreal> (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(lineWidth, qreal (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(color, QColor (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(miterLimit, qreal (KoStrokeConfigWidget::*)() const);
}

void KoStrokeConfigWidgetSchemaContractTest::strokeConfigMarkerAndStrokeSchemaRemainsStable()
{
    ASSERT_STROKE_CONFIG_SIGNATURE(startMarker, KoMarker * (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(endMarker, KoMarker * (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(capStyle, Qt::PenCapStyle (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(joinStyle, Qt::PenJoinStyle (KoStrokeConfigWidget::*)() const);
    ASSERT_STROKE_CONFIG_SIGNATURE(createShapeStroke, KoShapeStrokeSP (KoStrokeConfigWidget::*)());
}

void KoStrokeConfigWidgetSchemaContractTest::strokeConfigControlSchemaRemainsStable()
{
    ASSERT_STROKE_CONFIG_SIGNATURE(setNoSelectionTrackingMode, void (KoStrokeConfigWidget::*)(bool));
    ASSERT_STROKE_CONFIG_SIGNATURE(setActive, void (KoStrokeConfigWidget::*)(bool));
    ASSERT_STROKE_CONFIG_SIGNATURE(updateStyleControlsAvailability, void (KoStrokeConfigWidget::*)(bool));
    ASSERT_STROKE_CONFIG_SIGNATURE(setUnitManagers,
                                   void (KoStrokeConfigWidget::*)(KisSpinBoxUnitManager *, KisSpinBoxUnitManager *));
}

void KoStrokeConfigWidgetSchemaContractTest::strokeConfigActivationAndNotificationSchemaRemainsStable()
{
    ASSERT_STROKE_CONFIG_SIGNATURE(activate, void (KoStrokeConfigWidget::*)());
    ASSERT_STROKE_CONFIG_SIGNATURE(deactivate, void (KoStrokeConfigWidget::*)());
    ASSERT_STROKE_CONFIG_SIGNATURE(sigStrokeChanged, void (KoStrokeConfigWidget::*)());
}

void KoStrokeConfigWidgetSchemaContractTest::fillConfigTypeLifetimeAndOutputSchemaRemainStable()
{
    static_assert(std::is_class_v<KoFillConfigWidget>);
    static_assert(std::is_base_of_v<QWidget, KoFillConfigWidget>);
    static_assert(std::is_constructible_v<KoFillConfigWidget, KoCanvasBase *, KoFlake::FillVariant, bool, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KoFillConfigWidget>);
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, currentShapes, QList<KoShape *> (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, selectedFillIndex, int (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, createShapeStroke, KoShapeStrokeSP (KoFillConfigWidget::*)());
}

void KoStrokeConfigWidgetSchemaContractTest::fillConfigActivationAndControlSchemaRemainStable()
{
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, activate, void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, deactivate, void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget,
                                      forceUpdateOnSelectionChanged,
                                      void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget,
                                      setNoSelectionTrackingMode,
                                      void (KoFillConfigWidget::*)(bool));
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget,
                                      setSelectedMeshGradientHandle,
                                      void (KoFillConfigWidget::*)(const SvgMeshPosition &));
}

void KoStrokeConfigWidgetSchemaContractTest::fillConfigNotificationSignaturesRemainStable()
{
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, sigFillChanged, void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget,
                                      sigInternalRecoverColorInResourceManager,
                                      void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget,
                                      sigInternalRequestColorToResourceManager,
                                      void (KoFillConfigWidget::*)());
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoFillConfigWidget, sigMeshGradientResetted, void (KoFillConfigWidget::*)());
}

void KoStrokeConfigWidgetSchemaContractTest::markerSelectorTypeLifetimeAndSelectionSchemaRemainStable()
{
    static_assert(std::is_class_v<KoMarkerSelector>);
    static_assert(std::is_base_of_v<QComboBox, KoMarkerSelector>);
    static_assert(std::is_constructible_v<KoMarkerSelector, KoFlake::MarkerPosition>);
    static_assert(std::is_constructible_v<KoMarkerSelector, KoFlake::MarkerPosition, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KoMarkerSelector>);
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoMarkerSelector, marker, KoMarker * (KoMarkerSelector::*)() const);
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoMarkerSelector, setMarker, void (KoMarkerSelector::*)(KoMarker *));
}

void KoStrokeConfigWidgetSchemaContractTest::markerSelectorPresentationSchemaRemainStable()
{
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoMarkerSelector, itemData, QVariant (KoMarkerSelector::*)(int, int) const);
    ASSERT_SHAPE_APPEARANCE_SIGNATURE(KoMarkerSelector, updateMarkers, void (KoMarkerSelector::*)(QList<KoMarker *>));

    using ItemDataWithDefaultRole = decltype(std::declval<const KoMarkerSelector &>().itemData(std::declval<int>()));
    static_assert(std::is_same_v<ItemDataWithDefaultRole, QVariant>);
}

QTEST_GUILESS_MAIN(KoStrokeConfigWidgetSchemaContractTest)
#include "KoStrokeConfigWidgetSchemaContractTest.moc"
