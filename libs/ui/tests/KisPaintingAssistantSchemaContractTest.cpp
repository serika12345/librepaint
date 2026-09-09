/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "ConcentricEllipseAssistant.h"
#include "canvas/kis_painting_assistant.h"

#define ASSERT_ASSISTANT_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintingAssistant::method)), signature>)
#define ASSERT_HANDLE_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintingAssistantHandle::method)), signature>)
#define ASSERT_FACTORY_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintingAssistantFactory::method)), signature>)
#define ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&ConcentricEllipseAssistant::method)), signature>)
#define ASSERT_CONCENTRIC_FACTORY_SIGNATURE(method, signature)                                                         \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&ConcentricEllipseAssistantFactory::method)), signature>)

class PaintingAssistantProbe final : public KisPaintingAssistant
{
public:
    using KisPaintingAssistant::KisPaintingAssistant;

    KisPaintingAssistantSP clone(QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP> &) const override;
    QPointF adjustPosition(const QPointF &, const QPointF &, bool, qreal) override;
    void adjustLine(QPointF &, QPointF &) override;
    QPointF getDefaultEditorPosition() const override;
    int numHandles() const override;

protected:
    void drawCache(QPainter &, const KisCoordinatesConverter *, const KoColorDisplayRendererInterface *, bool) override;
};

class PaintingAssistantFactoryProbe final : public KisPaintingAssistantFactory
{
public:
    using KisPaintingAssistantFactory::KisPaintingAssistantFactory;

    QString id() const override;
    QString name() const override;
    KisPaintingAssistant *createPaintingAssistant() const override;
};

class KisPaintingAssistantSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void handleTypeAndLifetimeSchemaRemainStable();
    void assistantIdentityAndStateSchemaRemainStable();
    void assistantHandleCollectionAndPositionSchemaRemainStable();
    void assistantAdjustmentAndDrawingSchemaRemainStable();
    void assistantPersistenceFactoryAndRegistrySchemaRemainStable();
    void concentricEllipseAssistantTypeConstructionAndCloneSchemaRemainStable();
    void concentricEllipseAssistantAdjustmentSignaturesRemainStable();
    void concentricEllipseFactoryTypeAndLifetimeSchemaRemainStable();
    void concentricEllipseFactoryIdentityAndCreationSignaturesRemainStable();
};

void KisPaintingAssistantSchemaContractTest::handleTypeAndLifetimeSchemaRemainStable()
{
    using Handle = KisPaintingAssistantHandle;

    static_assert(std::is_same_v<KisPaintingAssistantHandleSP, KisSharedPtr<Handle>>);
    static_assert(std::is_class_v<Handle>);
    static_assert(std::is_same_v<decltype(NORMAL), HandleType>);
    static_assert(NORMAL == 0);
    static_assert(SIDE == 1);
    static_assert(CORNER == 2);
    static_assert(VANISHING_POINT == 3);
    static_assert(ANCHOR == 4);
    static_assert(std::is_constructible_v<Handle, QPointF>);
    static_assert(std::is_constructible_v<Handle, const Handle &>);
    static_assert(std::is_constructible_v<Handle, double, double>);
    static_assert(std::is_destructible_v<Handle>);
    ASSERT_HANDLE_SIGNATURE(chiefAssistant, KisPaintingAssistant * (Handle::*)() const);
    ASSERT_HANDLE_SIGNATURE(handleType, char (Handle::*)() const);
    ASSERT_HANDLE_SIGNATURE(mergeWith, void (Handle::*)(KisPaintingAssistantHandleSP));
    ASSERT_HANDLE_SIGNATURE(operator=, Handle & (Handle::*)(const QPointF &));
    ASSERT_HANDLE_SIGNATURE(setType, void (Handle::*)(char));
    ASSERT_HANDLE_SIGNATURE(uncache, void (Handle::*)());

    QVERIFY(true);
}

void KisPaintingAssistantSchemaContractTest::assistantIdentityAndStateSchemaRemainStable()
{
    using Assistant = KisPaintingAssistant;

    static_assert(std::is_class_v<Assistant>);
    static_assert(std::is_abstract_v<Assistant>);
    static_assert(std::is_constructible_v<PaintingAssistantProbe, const QString &, const QString &>);
    static_assert(std::has_virtual_destructor_v<Assistant>);
    ASSERT_ASSISTANT_SIGNATURE(id, const QString &(Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(name, const QString &(Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(isSnappingActive, bool (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(setSnappingActive, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(copySharedData, void (Assistant::*)(KisPaintingAssistantSP));
    ASSERT_ASSISTANT_SIGNATURE(canBeLocal, bool (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(isLocal, bool (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(setLocal, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(isLocked, bool (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setLocked, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(isDuplicating, bool (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setDuplicating, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(editorWidgetOffset, QPointF (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setEditorWidgetOffset, void (Assistant::*)(QPointF));
    ASSERT_ASSISTANT_SIGNATURE(effectiveAssistantColor, QColor (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(useCustomColor, bool (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setUseCustomColor, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(setAssistantCustomColor, void (Assistant::*)(QColor));
    ASSERT_ASSISTANT_SIGNATURE(assistantCustomColor, QColor (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setAssistantGlobalColorCache, void (Assistant::*)(const QColor &));
    ASSERT_ASSISTANT_SIGNATURE(setDecorationThickness, void (Assistant::*)(int));

    QVERIFY(true);
}

void KisPaintingAssistantSchemaContractTest::assistantHandleCollectionAndPositionSchemaRemainStable()
{
    using Assistant = KisPaintingAssistant;
    using Handle = KisPaintingAssistantHandleSP;
    using HandleList = QList<Handle>;
    using ConstHandle = const Handle (Assistant::*)() const;
    using MutableHandle = Handle (Assistant::*)();

    ASSERT_ASSISTANT_SIGNATURE(addHandle, void (Assistant::*)(Handle, HandleType));
    ASSERT_ASSISTANT_SIGNATURE(replaceHandle, void (Assistant::*)(Handle, Handle));
    ASSERT_ASSISTANT_SIGNATURE(viewportConstrainedEditorPosition,
                               QPointF (Assistant::*)(const KisCoordinatesConverter *, QSize));
    ASSERT_ASSISTANT_SIGNATURE(handles, const HandleList &(Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(handles, HandleList (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(sideHandles, const HandleList &(Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(sideHandles, HandleList (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(topLeft, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(topLeft, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(topRight, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(topRight, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomLeft, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomLeft, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomRight, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomRight, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(topMiddle, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(topMiddle, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(rightMiddle, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(rightMiddle, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(leftMiddle, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(leftMiddle, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomMiddle, ConstHandle);
    ASSERT_ASSISTANT_SIGNATURE(bottomMiddle, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(oppHandleOne, MutableHandle);
    ASSERT_ASSISTANT_SIGNATURE(closestCornerHandleFromPoint, Handle (Assistant::*)(QPointF));
    ASSERT_ASSISTANT_SIGNATURE(areTwoPointsClose, bool (Assistant::*)(const QPointF &, const QPointF &));
    ASSERT_ASSISTANT_SIGNATURE(findPerspectiveAssistantHandleLocation, void (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(isAssistantComplete, bool (Assistant::*)() const);
    static_assert(std::is_same_v<decltype(&Assistant::cloneAssistantList),
                                 QList<KisPaintingAssistantSP> (*)(const QList<KisPaintingAssistantSP> &)>);

    QVERIFY(true);
}

void KisPaintingAssistantSchemaContractTest::assistantAdjustmentAndDrawingSchemaRemainStable()
{
    using Assistant = KisPaintingAssistant;
    using HandleMap = QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP>;
    using DisplayRenderer = KoColorDisplayRendererInterface;

    ASSERT_ASSISTANT_SIGNATURE(clone, KisPaintingAssistantSP (Assistant::*)(HandleMap &) const);
    ASSERT_ASSISTANT_SIGNATURE(adjustLine, void (Assistant::*)(QPointF &, QPointF &));
    ASSERT_ASSISTANT_SIGNATURE(adjustPosition, QPointF (Assistant::*)(const QPointF &, const QPointF &, bool, qreal));
    ASSERT_ASSISTANT_SIGNATURE(drawAssistant,
                               void (Assistant::*)(QPainter &,
                                                   const QRectF &,
                                                   const KisCoordinatesConverter *,
                                                   const DisplayRenderer *,
                                                   bool,
                                                   KisCanvas2 *,
                                                   bool,
                                                   bool));
    ASSERT_ASSISTANT_SIGNATURE(drawError,
                               void (Assistant::*)(QPainter &, const QPainterPath &, const DisplayRenderer *));
    ASSERT_ASSISTANT_SIGNATURE(drawPath,
                               void (Assistant::*)(QPainter &, const QPainterPath &, const DisplayRenderer *, bool));
    ASSERT_ASSISTANT_SIGNATURE(drawPreview,
                               void (Assistant::*)(QPainter &, const QPainterPath &, const DisplayRenderer *));
    ASSERT_ASSISTANT_SIGNATURE(drawX, void (Assistant::*)(QPainter &, const QPointF &, const DisplayRenderer *));
    ASSERT_ASSISTANT_SIGNATURE(endStroke, void (Assistant::*)());
    ASSERT_ASSISTANT_SIGNATURE(setAdjustedBrushPosition, void (Assistant::*)(QPointF));
    ASSERT_ASSISTANT_SIGNATURE(setFollowBrushPosition, void (Assistant::*)(bool));
    ASSERT_ASSISTANT_SIGNATURE(getDefaultEditorPosition, QPointF (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(getEditorPosition, QPointF (Assistant::*)() const);
    ASSERT_ASSISTANT_SIGNATURE(numHandles, int (Assistant::*)() const);
    static_assert(std::is_same_v<decltype(&Assistant::norm2), double (*)(const QPointF &)>);
    ASSERT_ASSISTANT_SIGNATURE(transform, void (Assistant::*)(const QTransform &));
    ASSERT_ASSISTANT_SIGNATURE(uncache, void (Assistant::*)());

    QVERIFY(true);
}

void KisPaintingAssistantSchemaContractTest::assistantPersistenceFactoryAndRegistrySchemaRemainStable()
{
    using Assistant = KisPaintingAssistant;
    using Factory = KisPaintingAssistantFactory;
    using Registry = KisPaintingAssistantFactoryRegistry;

    ASSERT_ASSISTANT_SIGNATURE(saveXml, QByteArray (Assistant::*)(QMap<KisPaintingAssistantHandleSP, int> &));
    ASSERT_ASSISTANT_SIGNATURE(saveCustomXml, void (Assistant::*)(QXmlStreamWriter *));
    ASSERT_ASSISTANT_SIGNATURE(loadXml,
                               void (Assistant::*)(KoStore *, QMap<int, KisPaintingAssistantHandleSP> &, QString));
    ASSERT_ASSISTANT_SIGNATURE(loadCustomXml, bool (Assistant::*)(QXmlStreamReader *));
    ASSERT_ASSISTANT_SIGNATURE(saveXmlList, void (Assistant::*)(QDomDocument &, QDomElement &, int));
    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_abstract_v<Factory>);
    static_assert(std::is_default_constructible_v<PaintingAssistantFactoryProbe>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    ASSERT_FACTORY_SIGNATURE(id, QString (Factory::*)() const);
    ASSERT_FACTORY_SIGNATURE(name, QString (Factory::*)() const);
    ASSERT_FACTORY_SIGNATURE(createPaintingAssistant, Assistant * (Factory::*)() const);
    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::is_destructible_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);

    QVERIFY(true);
}

void KisPaintingAssistantSchemaContractTest::concentricEllipseAssistantTypeConstructionAndCloneSchemaRemainStable()
{
    using Assistant = ConcentricEllipseAssistant;
    using HandleMap = QMap<KisPaintingAssistantHandleSP, KisPaintingAssistantHandleSP>;

    static_assert(std::is_class_v<Assistant>);
    static_assert(std::is_base_of_v<KisPaintingAssistant, Assistant>);
    static_assert(std::is_default_constructible_v<Assistant>);
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(clone, KisPaintingAssistantSP (Assistant::*)(HandleMap &) const);
}

void KisPaintingAssistantSchemaContractTest::concentricEllipseAssistantAdjustmentSignaturesRemainStable()
{
    using Assistant = ConcentricEllipseAssistant;

    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(adjustPosition,
                                          QPointF (Assistant::*)(const QPointF &, const QPointF &, bool, qreal));
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(adjustLine, void (Assistant::*)(QPointF &, QPointF &));
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(getDefaultEditorPosition, QPointF (Assistant::*)() const);
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(numHandles, int (Assistant::*)() const);
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(isAssistantComplete, bool (Assistant::*)() const);
    ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE(transform, void (Assistant::*)(const QTransform &));
}

void KisPaintingAssistantSchemaContractTest::concentricEllipseFactoryTypeAndLifetimeSchemaRemainStable()
{
    using Factory = ConcentricEllipseAssistantFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisPaintingAssistantFactory, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::has_virtual_destructor_v<Factory>);
}

void KisPaintingAssistantSchemaContractTest::concentricEllipseFactoryIdentityAndCreationSignaturesRemainStable()
{
    using Factory = ConcentricEllipseAssistantFactory;

    ASSERT_CONCENTRIC_FACTORY_SIGNATURE(id, QString (Factory::*)() const);
    ASSERT_CONCENTRIC_FACTORY_SIGNATURE(name, QString (Factory::*)() const);
    ASSERT_CONCENTRIC_FACTORY_SIGNATURE(createPaintingAssistant, KisPaintingAssistant * (Factory::*)() const);
}

#undef ASSERT_CONCENTRIC_FACTORY_SIGNATURE
#undef ASSERT_CONCENTRIC_ASSISTANT_SIGNATURE

QTEST_GUILESS_MAIN(KisPaintingAssistantSchemaContractTest)

#include "KisPaintingAssistantSchemaContractTest.moc"
