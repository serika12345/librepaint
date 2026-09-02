/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoStrokeConfigWidget.h>
#include <QTest>

#include <type_traits>

#define ASSERT_STROKE_CONFIG_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoStrokeConfigWidget::method)), signature>)

class KoStrokeConfigWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strokeConfigTypeAndLifetimeSchemaRemainsStable();
    void strokeConfigAppearanceSchemaRemainsStable();
    void strokeConfigMarkerAndStrokeSchemaRemainsStable();
    void strokeConfigControlSchemaRemainsStable();
    void strokeConfigActivationAndNotificationSchemaRemainsStable();
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

QTEST_GUILESS_MAIN(KoStrokeConfigWidgetSchemaContractTest)
#include "KoStrokeConfigWidgetSchemaContractTest.moc"
