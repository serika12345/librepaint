/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_dom_utils.h"

#include <QColor>
#include <QDomDocument>
#include <QList>
#include <QTest>
#include <QTransform>
#include <QVector3D>

#include <tuple>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("Unexpected DOM utility assertion");
}

namespace ContractValue
{
struct Number {
    int value = 0;

    bool operator==(const Number &other) const
    {
        return value == other.value;
    }
};

bool loadValue(const QDomElement &element, Number *number, const int &multiplier, const int &offset)
{
    if (element.attribute(QStringLiteral("type")) != QStringLiteral("value")) {
        return false;
    }

    number->value = element.attribute(QStringLiteral("value")).toInt() * multiplier + offset;
    return true;
}
} // namespace ContractValue

namespace
{
QDomElement elementForTag(const QDomElement &root, const QString &tag)
{
    const QDomElement element = root.firstChildElement(tag);
    Q_ASSERT(!element.isNull());
    return element;
}

QDomElement appendValueElement(QDomDocument &document, QDomElement &parent, const QString &tag, int value)
{
    QDomElement element = document.createElement(tag);
    element.setAttribute(QStringLiteral("type"), QStringLiteral("value"));
    element.setAttribute(QStringLiteral("value"), value);
    parent.appendChild(element);
    return element;
}
} // namespace

class KisDomUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void numberConversionsUseStableTextAndGermanFallbacks();
    void colorHelpersAndXmlColorLoadingExposeCurrentBehavior();
    void scalarAndGeometryValuesRoundTripByType();
    void fractionalRectanglesCurrentlyLoadThroughIntegerConversion();
    void arraysUseDefaultAndExplicitEnvironments();
    void typeChecksRejectMismatchesWithoutChangingDestinations();
    void elementLookupAndRemovalReportTheirResult();
};

void KisDomUtilsContractTest::numberConversionsUseStableTextAndGermanFallbacks()
{
    QCOMPARE(KisDomUtils::toString(QStringLiteral("text")), QStringLiteral("text"));
    QCOMPARE(KisDomUtils::toString(42), QStringLiteral("42"));

    bool ok = false;
    const QString floatText = KisDomUtils::toString(0.125f);
    QCOMPARE(KisDomUtils::toDouble(floatText, &ok), 0.125);
    QVERIFY(ok);

    const QString doubleText = KisDomUtils::toString(1234567.89012345);
    QCOMPARE(KisDomUtils::toDouble(doubleText, &ok), 1234567.89012345);
    QVERIFY(ok);

    QCOMPARE(KisDomUtils::toInt(QStringLiteral("1.234"), &ok), 1234);
    QVERIFY(ok);
    QCOMPARE(KisDomUtils::toDouble(QStringLiteral("1,5"), &ok), 1.5);
    QVERIFY(ok);

    QCOMPARE(KisDomUtils::toInt(QStringLiteral("invalid"), &ok), 0);
    QVERIFY(!ok);
    QCOMPARE(KisDomUtils::toDouble(QStringLiteral("invalid"), &ok), 0.0);
    QVERIFY(!ok);
}

void KisDomUtilsContractTest::colorHelpersAndXmlColorLoadingExposeCurrentBehavior()
{
    const QColor source(12, 34, 56, 78);
    QCOMPARE(KisDomUtils::qColorToQString(source), QStringLiteral("12,34,56,78"));
    QCOMPARE(KisDomUtils::qStringToQColor(QStringLiteral("12,34,56,78")), source);

    QDomDocument document(QStringLiteral("colors"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);
    KisDomUtils::saveValue(&root, QStringLiteral("color"), source);

    const QDomElement element = elementForTag(root, QStringLiteral("color"));
    QCOMPARE(element.attribute(QStringLiteral("type")), QStringLiteral("qcolor"));
    QCOMPARE(element.attribute(QStringLiteral("value")), QStringLiteral("#4e0c2238"));

    QColor loaded;
    QVERIFY(KisDomUtils::loadValue(element, &loaded));
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    QCOMPARE(loaded, source);
#else
    QVERIFY(!loaded.isValid());
#endif
}

void KisDomUtilsContractTest::scalarAndGeometryValuesRoundTripByType()
{
    QDomDocument document(QStringLiteral("values"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);

    KisDomUtils::saveValue(&root, QStringLiteral("integer"), 23);
    KisDomUtils::saveValue(&root, QStringLiteral("float"), 0.125f);
    KisDomUtils::saveValue(&root, QStringLiteral("double"), 2.5);
    KisDomUtils::saveValue(&root, QStringLiteral("string"), QStringLiteral("stored"));
    KisDomUtils::saveValue(&root, QStringLiteral("size"), QSize(13, 17));
    KisDomUtils::saveValue(&root, QStringLiteral("rect"), QRect(1, 2, 30, 40));
    KisDomUtils::saveValue(&root, QStringLiteral("rectf"), QRectF(3.0, 4.0, 5.0, 6.0));
    KisDomUtils::saveValue(&root, QStringLiteral("point"), QPoint(7, 8));
    KisDomUtils::saveValue(&root, QStringLiteral("pointf"), QPointF(1.25, -2.5));
    KisDomUtils::saveValue(&root, QStringLiteral("vector"), QVector3D(1.5f, 2.5f, 3.5f));

    const QTransform sourceTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    KisDomUtils::saveValue(&root, QStringLiteral("transform"), sourceTransform);

    int integer = 0;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("integer")), &integer));
    QCOMPARE(integer, 23);

    std::tuple<> emptyEnvironment;
    integer = 0;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("integer")), &integer, emptyEnvironment));
    QCOMPARE(integer, 23);

    integer = 0;
    QVERIFY(KisDomUtils::loadValue(root, QStringLiteral("integer"), &integer));
    QCOMPARE(integer, 23);

    float floatValue = 0.0f;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("float")), &floatValue));
    QCOMPARE(floatValue, 0.125f);

    double doubleValue = 0.0;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("double")), &doubleValue));
    QCOMPARE(doubleValue, 2.5);

    QString stringValue;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("string")), &stringValue));
    QCOMPARE(stringValue, QStringLiteral("stored"));

    QSize size;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("size")), &size));
    QCOMPARE(size, QSize(13, 17));

    QRect rect;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("rect")), &rect));
    QCOMPARE(rect, QRect(1, 2, 30, 40));

    QRectF rectF;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("rectf")), &rectF));
    QCOMPARE(rectF, QRectF(3.0, 4.0, 5.0, 6.0));

    QPoint point;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("point")), &point));
    QCOMPARE(point, QPoint(7, 8));

    QPointF pointF;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("pointf")), &pointF));
    QCOMPARE(pointF, QPointF(1.25, -2.5));

    QVector3D vector;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("vector")), &vector));
    QCOMPARE(vector, QVector3D(1.5f, 2.5f, 3.5f));

    QTransform transform;
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("transform")), &transform));
    QCOMPARE(transform, sourceTransform);
}

void KisDomUtilsContractTest::fractionalRectanglesCurrentlyLoadThroughIntegerConversion()
{
    QDomDocument document(QStringLiteral("fractional-rect"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);
    KisDomUtils::saveValue(&root, QStringLiteral("rect"), QRectF(1.5, 2.25, 3.75, 4.5));

    QRectF loaded(10.0, 20.0, 30.0, 40.0);
    QVERIFY(KisDomUtils::loadValue(elementForTag(root, QStringLiteral("rect")), &loaded));
    QCOMPARE(loaded, QRectF());
}

void KisDomUtilsContractTest::arraysUseDefaultAndExplicitEnvironments()
{
    QDomDocument document(QStringLiteral("arrays"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);

    const QList<int> source{3, 5, 8};
    KisDomUtils::saveValue(&root, QStringLiteral("numbers"), source);
    const QDomElement numbersElement = elementForTag(root, QStringLiteral("numbers"));

    QList<int> direct;
    QVERIFY(KisDomUtils::loadValue(numbersElement, &direct, std::tuple<>()));
    QCOMPARE(direct, source);

    QList<int> tagged;
    QVERIFY(KisDomUtils::loadValue(root, QStringLiteral("numbers"), &tagged));
    QCOMPARE(tagged, source);

    QDomElement custom = document.createElement(QStringLiteral("custom"));
    custom.setAttribute(QStringLiteral("type"), QStringLiteral("array"));
    root.appendChild(custom);
    appendValueElement(document, custom, QStringLiteral("first"), 4);
    appendValueElement(document, custom, QStringLiteral("second"), 7);

    const QList<ContractValue::Number> expected{{11}, {17}};
    QList<ContractValue::Number> explicitEnvironment;
    QVERIFY(KisDomUtils::loadValue(custom, &explicitEnvironment, 2, 3));
    QCOMPARE(explicitEnvironment, expected);

    QList<ContractValue::Number> taggedEnvironment;
    QVERIFY(KisDomUtils::loadValue(root, QStringLiteral("custom"), &taggedEnvironment, 2, 3));
    QCOMPARE(taggedEnvironment, expected);
}

void KisDomUtilsContractTest::typeChecksRejectMismatchesWithoutChangingDestinations()
{
    QDomDocument document(QStringLiteral("types"));
    QDomElement element = document.createElement(QStringLiteral("point"));
    element.setAttribute(QStringLiteral("type"), QStringLiteral("point"));
    element.setAttribute(QStringLiteral("x"), 4);
    element.setAttribute(QStringLiteral("y"), 7);

    QVERIFY(KisDomUtils::Private::checkType(element, QStringLiteral("point")));
    QVERIFY(!KisDomUtils::Private::checkType(element, QStringLiteral("rect")));

    int destination = 19;
    QVERIFY(!KisDomUtils::loadValue(element, &destination));
    QCOMPARE(destination, 19);
}

void KisDomUtilsContractTest::elementLookupAndRemovalReportTheirResult()
{
    QDomDocument document(QStringLiteral("lookup"));
    QDomElement root = document.createElement(QStringLiteral("root"));
    document.appendChild(root);

    QDomElement only = document.createElement(QStringLiteral("only"));
    only.setAttribute(QStringLiteral("id"), QStringLiteral("wanted"));
    root.appendChild(only);

    QDomElement found;
    QStringList errors;
    QVERIFY(KisDomUtils::findOnlyElement(root, QStringLiteral("only"), &found, &errors));
    QCOMPARE(found, only);
    QVERIFY(errors.isEmpty());

    QCOMPARE(KisDomUtils::findElementByAttribute(root,
                                                 QStringLiteral("only"),
                                                 QStringLiteral("id"),
                                                 QStringLiteral("wanted")),
             only);
    QVERIFY(KisDomUtils::findElementByAttribute(root,
                                                QStringLiteral("only"),
                                                QStringLiteral("id"),
                                                QStringLiteral("missing"))
                .isNull());

    QVERIFY(!KisDomUtils::findOnlyElement(root, QStringLiteral("missing"), &found, &errors));
    QCOMPARE(errors.size(), 1);

    QDomElement duplicate = document.createElement(QStringLiteral("only"));
    root.appendChild(duplicate);
    QVERIFY(!KisDomUtils::findOnlyElement(root, QStringLiteral("only"), &found, &errors));
    QCOMPARE(errors.size(), 2);

    QDomElement removable = document.createElement(QStringLiteral("remove"));
    root.appendChild(removable);
    QVERIFY(!KisDomUtils::removeElements(root, QStringLiteral("remove")));
    QVERIFY(root.firstChildElement(QStringLiteral("remove")).isNull());
    QVERIFY(!KisDomUtils::removeElements(root, QStringLiteral("remove")));
}

QTEST_GUILESS_MAIN(KisDomUtilsContractTest)

#include "KisDomUtilsContractTest.moc"
