/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TextTransformModel.h"

#include <QMetaProperty>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <lager/state.hpp>

namespace
{

using Transform = KoSvgText::TextTransformInfo;

void compareTransform(const lager::cursor<Transform> &cursor,
                      KoSvgText::TextTransform capitals,
                      bool fullWidth,
                      bool fullSizeKana)
{
    const Transform actual = cursor.get();
    QCOMPARE(int(actual.capitals), int(capitals));
    QCOMPARE(actual.fullWidth, fullWidth);
    QCOMPARE(actual.fullSizeKana, fullSizeKana);
}

} // namespace

class TextTransformModelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndQtPropertiesRemainStable();
    void injectedCursorSynchronizesBothDirections();
    void validCapitalModesAndBooleanFlagsRoundTripIndependently();
    void qObjectBaseProvidesVirtualLifetime();
};

void TextTransformModelContractTest::defaultStateAndQtPropertiesRemainStable()
{
    TextTransformModel model;

    compareTransform(model.data, KoSvgText::TextTransformNone, false, false);
    QCOMPARE(model.capitals(), int(KoSvgText::TextTransformNone));
    QVERIFY(!model.fullWidth());
    QVERIFY(!model.fullSizeKana());

    for (const char *name : {"capitals", "fullWidth", "fullSizeKana"}) {
        const int propertyIndex = model.metaObject()->indexOfProperty(name);
        QVERIFY(propertyIndex >= 0);
        const QMetaProperty property = model.metaObject()->property(propertyIndex);
        QVERIFY(property.isReadable());
        QVERIFY(property.isWritable());
        QVERIFY(property.hasNotifySignal());
    }
}

void TextTransformModelContractTest::injectedCursorSynchronizesBothDirections()
{
    auto state = lager::make_state(Transform(), lager::automatic_tag{});
    TextTransformModel model(state);
    QSignalSpy capitalsSpy(&model, &TextTransformModel::capitalsChanged);
    QSignalSpy fullWidthSpy(&model, &TextTransformModel::fullWidthChanged);
    QSignalSpy fullSizeKanaSpy(&model, &TextTransformModel::fullSizeKanaChanged);

    QVERIFY(capitalsSpy.isValid());
    QVERIFY(fullWidthSpy.isValid());
    QVERIFY(fullSizeKanaSpy.isValid());

    Transform external;
    external.capitals = KoSvgText::TextTransformUppercase;
    external.fullWidth = true;
    external.fullSizeKana = true;
    state.set(external);

    compareTransform(model.data, KoSvgText::TextTransformUppercase, true, true);
    QCOMPARE(model.capitals(), int(KoSvgText::TextTransformUppercase));
    QVERIFY(model.fullWidth());
    QVERIFY(model.fullSizeKana());
    QCOMPARE(capitalsSpy.size(), 1);
    QCOMPARE(fullWidthSpy.size(), 1);
    QCOMPARE(fullSizeKanaSpy.size(), 1);

    QVERIFY(model.setProperty("capitals", int(KoSvgText::TextTransformLowercase)));
    QVERIFY(model.setProperty("fullWidth", false));
    QVERIFY(model.setProperty("fullSizeKana", false));

    compareTransform(state, KoSvgText::TextTransformLowercase, false, false);
    QCOMPARE(capitalsSpy.size(), 2);
    QCOMPARE(fullWidthSpy.size(), 2);
    QCOMPARE(fullSizeKanaSpy.size(), 2);
}

void TextTransformModelContractTest::validCapitalModesAndBooleanFlagsRoundTripIndependently()
{
    auto state = lager::make_state(Transform(), lager::automatic_tag{});
    TextTransformModel model(state);
    model.setfullWidth(true);
    model.setfullSizeKana(true);

    for (const KoSvgText::TextTransform value : {KoSvgText::TextTransformNone,
                                                 KoSvgText::TextTransformCapitalize,
                                                 KoSvgText::TextTransformUppercase,
                                                 KoSvgText::TextTransformLowercase}) {
        model.setcapitals(int(value));
        QCOMPARE(int(state.get().capitals), int(value));
        QCOMPARE(model.capitals(), int(value));
        QVERIFY(state.get().fullWidth);
        QVERIFY(state.get().fullSizeKana);
    }

    model.setfullWidth(false);
    QCOMPARE(int(state.get().capitals), int(KoSvgText::TextTransformLowercase));
    QVERIFY(!state.get().fullWidth);
    QVERIFY(state.get().fullSizeKana);

    model.setfullSizeKana(false);
    QCOMPARE(int(state.get().capitals), int(KoSvgText::TextTransformLowercase));
    QVERIFY(!state.get().fullWidth);
    QVERIFY(!state.get().fullSizeKana);
}

void TextTransformModelContractTest::qObjectBaseProvidesVirtualLifetime()
{
    QPointer<TextTransformModel> model = new TextTransformModel;
    QObject *base = model;

    delete base;

    QVERIFY(model.isNull());
}

QTEST_GUILESS_MAIN(TextTransformModelContractTest)

#include "TextTransformModelContractTest.moc"
