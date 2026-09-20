/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QLoggingCategory>
#include <QTest>

#include <cstring>
#include <memory>

#include <KoColor.h>

#include "kis_asl_callback_object_catcher.h"
#include "kis_asl_object_catcher.h"

const QLoggingCategory &_41000()
{
    static const QLoggingCategory category("krita.general", QtInfoMsg);
    return category;
}

KoColor::KoColor()
    : m_colorSpace(nullptr)
    , m_size(0)
{
    memset(m_data, 0, sizeof(m_data));
}

QDebug operator<<(QDebug debug, const KoColor &)
{
    return debug << "KoColor";
}

QString KoResource::storageLocation() const
{
    return QString();
}

bool KoResource::valid() const
{
    return false;
}

QString KoResource::md5Sum(bool) const
{
    return QString();
}

int KoResource::version() const
{
    return 0;
}

QString KoResource::filename() const
{
    return QString();
}

namespace
{
QStringList *capturedMessages = nullptr;

void captureMessage(QtMsgType, const QMessageLogContext &, const QString &message)
{
    if (capturedMessages) {
        capturedMessages->append(message);
    }
}

class MessageCapture
{
public:
    explicit MessageCapture(QStringList *messages)
        : m_previousHandler(qInstallMessageHandler(captureMessage))
    {
        capturedMessages = messages;
    }

    ~MessageCapture()
    {
        capturedMessages = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

private:
    QtMessageHandler m_previousHandler;
};

template<typename T>
QSharedPointer<T> markerSharedPointer(quintptr address)
{
    return QSharedPointer<T>(reinterpret_cast<T *>(address), [](T *) { });
}
} // namespace

class KisAslObjectCatcherContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseCatcherReportsUnhandledValuesAndArrayMode();
    void scalarCallbacksRouteOnlyMatchingPaths();
    void geometryCallbacksPreserveValues();
    void compoundCallbacksPreserveValues();
    void qualifiedCallbacksRequireMatchingTypeAndUnit();
    void subscriptionsReplaceCallbacksAndReleaseOnDestruction();
};

void KisAslObjectCatcherContractTest::baseCatcherReportsUnhandledValuesAndArrayMode()
{
    QStringList messages;
    QLoggingCategory &category = const_cast<QLoggingCategory &>(_41000());
    const bool debugWasEnabled = category.isDebugEnabled();
    category.setEnabled(QtDebugMsg, true);
    {
        MessageCapture capture(&messages);
        KisAslObjectCatcher catcher;
        catcher.setArrayMode(true);

        const KoColor color;
        catcher.addDouble(QStringLiteral("double/path"), 1.5);
        catcher.addInteger(QStringLiteral("integer/path"), 3);
        catcher.addEnum(QStringLiteral("enum/path"), QStringLiteral("type"), QStringLiteral("value"));
        catcher.addUnitFloat(QStringLiteral("unit-float/path"), QStringLiteral("#Pxl"), 2.5);
        catcher.addText(QStringLiteral("text/path"), QStringLiteral("text"));
        catcher.addBoolean(QStringLiteral("boolean/path"), true);
        catcher.addColor(QStringLiteral("color/path"), color);
        catcher.addPoint(QStringLiteral("point/path"), QPointF(4.0, 5.0));
        catcher.addCurve(QStringLiteral("curve/path"), QStringLiteral("curve"), {QPointF(1.0, 2.0)});
        catcher.addPattern(QStringLiteral("pattern/path"), KoPatternSP(), QStringLiteral("pattern-id"));
        catcher.addPatternRef(QStringLiteral("pattern-ref/path"),
                              QStringLiteral("pattern-id"),
                              QStringLiteral("pattern"));
        catcher.addGradient(QStringLiteral("gradient/path"), KoAbstractGradientSP());
        catcher.addRawData(QStringLiteral("raw/path"), QByteArrayLiteral("raw"));
        catcher.addTransform(QStringLiteral("transform/path"), QTransform::fromScale(2.0, 3.0));
        catcher.addRect(QStringLiteral("rect/path"), QRectF(1.0, 2.0, 3.0, 4.0));
        catcher.addUnitRect(QStringLiteral("unit-rect/path"), QStringLiteral("#Pxl"), QRectF(5.0, 6.0, 7.0, 8.0));
        catcher.newStyleStarted();
    }
    category.setEnabled(QtDebugMsg, debugWasEnabled);

    QCOMPARE(messages.size(), 17);
    for (const QString &message : messages) {
        QVERIFY2(message.contains(QStringLiteral("Unhandled:")), qPrintable(message));
    }
    QVERIFY(messages.constFirst().contains(QStringLiteral("[A]")));
    QVERIFY(messages.join(QLatin1Char('\n')).contains(QStringLiteral("new style started")));
}

void KisAslObjectCatcherContractTest::scalarCallbacksRouteOnlyMatchingPaths()
{
    KisAslCallbackObjectCatcher catcher;
    double doubleValue = 0.0;
    int integerValue = 0;
    QString textValue;
    bool booleanValue = false;
    QByteArray rawValue;

    const ASLCallbackDouble doubleCallback = [&doubleValue](double value) {
        doubleValue = value;
    };
    const ASLCallbackInteger integerCallback = [&integerValue](int value) {
        integerValue = value;
    };
    const ASLCallbackString textCallback = [&textValue](const QString &value) {
        textValue = value;
    };
    const ASLCallbackBoolean booleanCallback = [&booleanValue](bool value) {
        booleanValue = value;
    };
    const ASLCallbackRawData rawCallback = [&rawValue](QByteArray value) {
        rawValue = value;
    };

    catcher.subscribeDouble(QStringLiteral("double"), doubleCallback);
    catcher.subscribeInteger(QStringLiteral("integer"), integerCallback);
    catcher.subscribeText(QStringLiteral("text"), textCallback);
    catcher.subscribeBoolean(QStringLiteral("boolean"), booleanCallback);
    catcher.subscribeRawData(QStringLiteral("raw"), rawCallback);

    catcher.addDouble(QStringLiteral("double"), 4.5);
    catcher.addInteger(QStringLiteral("integer"), 17);
    catcher.addText(QStringLiteral("text"), QStringLiteral("layer"));
    catcher.addBoolean(QStringLiteral("boolean"), true);
    catcher.addRawData(QStringLiteral("raw"), QByteArrayLiteral("payload"));
    catcher.addDouble(QStringLiteral("other-double"), 9.0);
    catcher.addInteger(QStringLiteral("other-integer"), 99);
    catcher.addText(QStringLiteral("other-text"), QStringLiteral("other"));
    catcher.addBoolean(QStringLiteral("other-boolean"), false);
    catcher.addRawData(QStringLiteral("other-raw"), QByteArrayLiteral("other"));

    QCOMPARE(doubleValue, 4.5);
    QCOMPARE(integerValue, 17);
    QCOMPARE(textValue, QStringLiteral("layer"));
    QVERIFY(booleanValue);
    QCOMPARE(rawValue, QByteArrayLiteral("payload"));
}

void KisAslObjectCatcherContractTest::geometryCallbacksPreserveValues()
{
    KisAslCallbackObjectCatcher catcher;
    const KoColor color;
    const KoColor *receivedColor = nullptr;
    QPointF receivedPoint;
    QTransform receivedTransform;
    QRectF receivedRect;

    const ASLCallbackColor colorCallback = [&receivedColor](const KoColor &value) {
        receivedColor = &value;
    };
    const ASLCallbackPoint pointCallback = [&receivedPoint](const QPointF &value) {
        receivedPoint = value;
    };
    const ASLCallbackTransform transformCallback = [&receivedTransform](QTransform value) {
        receivedTransform = value;
    };
    const ASLCallbackRect rectCallback = [&receivedRect](QRectF value) {
        receivedRect = value;
    };

    catcher.subscribeColor(QStringLiteral("color"), colorCallback);
    catcher.subscribePoint(QStringLiteral("point"), pointCallback);
    catcher.subscribeTransform(QStringLiteral("transform"), transformCallback);
    catcher.subscribeRect(QStringLiteral("rect"), rectCallback);

    const QPointF point(1.25, 2.75);
    const QTransform transform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
    const QRectF rect(7.0, 8.0, 9.0, 10.0);
    catcher.addColor(QStringLiteral("color"), color);
    catcher.addPoint(QStringLiteral("point"), point);
    catcher.addTransform(QStringLiteral("transform"), transform);
    catcher.addRect(QStringLiteral("rect"), rect);

    QCOMPARE(receivedColor, &color);
    QCOMPARE(receivedPoint, point);
    QCOMPARE(receivedTransform, transform);
    QCOMPARE(receivedRect, rect);
}

void KisAslObjectCatcherContractTest::compoundCallbacksPreserveValues()
{
    KisAslCallbackObjectCatcher catcher;
    const KoPatternSP pattern = markerSharedPointer<KoPattern>(0x1000);
    const KoAbstractGradientSP gradient = markerSharedPointer<KoAbstractGradient>(0x2000);
    QString curveName;
    QVector<QPointF> curvePoints;
    KoPatternSP receivedPattern;
    QString receivedPatternUuid;
    QString referencedUuid;
    QString referencedName;
    KoAbstractGradientSP receivedGradient;
    int newStyleCount = 0;

    const ASLCallbackCurve curveCallback = [&curveName, &curvePoints](const QString &name,
                                                                      const QVector<QPointF> &points) {
        curveName = name;
        curvePoints = points;
    };
    const ASLCallbackPattern patternCallback = [&receivedPattern, &receivedPatternUuid](const KoPatternSP value,
                                                                                        const QString &uuid) {
        receivedPattern = value;
        receivedPatternUuid = uuid;
    };
    const ASLCallbackPatternRef patternRefCallback = [&referencedUuid, &referencedName](const QString &uuid,
                                                                                        const QString &name) {
        referencedUuid = uuid;
        referencedName = name;
    };
    const ASLCallbackGradient gradientCallback = [&receivedGradient](KoAbstractGradientSP value) {
        receivedGradient = value;
    };
    const ASLCallbackNewStyle newStyleCallback = [&newStyleCount]() {
        ++newStyleCount;
    };

    catcher.subscribeCurve(QStringLiteral("curve"), curveCallback);
    catcher.subscribePattern(QStringLiteral("pattern"), patternCallback);
    catcher.subscribePatternRef(QStringLiteral("pattern-ref"), patternRefCallback);
    catcher.subscribeGradient(QStringLiteral("gradient"), gradientCallback);
    catcher.subscribeNewStyleStarted(newStyleCallback);

    const QVector<QPointF> points{QPointF(0.0, 1.0), QPointF(2.0, 3.0)};
    catcher.addCurve(QStringLiteral("curve"), QStringLiteral("tone"), points);
    catcher.addPattern(QStringLiteral("pattern"), pattern, QStringLiteral("pattern-id"));
    catcher.addPatternRef(QStringLiteral("pattern-ref"),
                          QStringLiteral("reference-id"),
                          QStringLiteral("reference-name"));
    catcher.addGradient(QStringLiteral("gradient"), gradient);
    catcher.newStyleStarted();

    QCOMPARE(curveName, QStringLiteral("tone"));
    QCOMPARE(curvePoints, points);
    QCOMPARE(receivedPattern.data(), pattern.data());
    QCOMPARE(receivedPatternUuid, QStringLiteral("pattern-id"));
    QCOMPARE(referencedUuid, QStringLiteral("reference-id"));
    QCOMPARE(referencedName, QStringLiteral("reference-name"));
    QCOMPARE(receivedGradient.data(), gradient.data());
    QCOMPARE(newStyleCount, 1);
}

void KisAslObjectCatcherContractTest::qualifiedCallbacksRequireMatchingTypeAndUnit()
{
    KisAslCallbackObjectCatcher catcher;
    QString enumValue;
    double pixelValue = 0.0;
    double percentValue = 0.0;
    QRectF unitRect;

    catcher.subscribeEnum(QStringLiteral("enum"), QStringLiteral("blend-mode"), [&enumValue](const QString &value) {
        enumValue = value;
    });
    catcher.subscribeUnitFloat(QStringLiteral("size"), QStringLiteral("#Pxl"), [&pixelValue](double value) {
        pixelValue = value;
    });
    catcher.subscribeUnitFloat(QStringLiteral("size"), QStringLiteral("#Prc"), [&percentValue](double value) {
        percentValue = value;
    });
    catcher.subscribeUnitRect(QStringLiteral("bounds"), QStringLiteral("#Pxl"), [&unitRect](QRectF value) {
        unitRect = value;
    });

    catcher.addEnum(QStringLiteral("enum"), QStringLiteral("wrong-type"), QStringLiteral("multiply"));
    catcher.addUnitFloat(QStringLiteral("size"), QStringLiteral("wrong-unit"), 99.0);
    catcher.addUnitRect(QStringLiteral("bounds"), QStringLiteral("wrong-unit"), QRectF(1.0, 1.0, 1.0, 1.0));
    QVERIFY(enumValue.isEmpty());
    QCOMPARE(pixelValue, 0.0);
    QCOMPARE(percentValue, 0.0);
    QVERIFY(unitRect.isNull());

    catcher.addEnum(QStringLiteral("enum"), QStringLiteral("blend-mode"), QStringLiteral("multiply"));
    catcher.addUnitFloat(QStringLiteral("size"), QStringLiteral("#Pxl"), 12.0);
    catcher.addUnitFloat(QStringLiteral("size"), QStringLiteral("#Prc"), 45.0);
    catcher.addUnitRect(QStringLiteral("bounds"), QStringLiteral("#Pxl"), QRectF(2.0, 3.0, 4.0, 5.0));

    QCOMPARE(enumValue, QStringLiteral("multiply"));
    QCOMPARE(pixelValue, 12.0);
    QCOMPARE(percentValue, 45.0);
    QCOMPARE(unitRect, QRectF(2.0, 3.0, 4.0, 5.0));
}

void KisAslObjectCatcherContractTest::subscriptionsReplaceCallbacksAndReleaseOnDestruction()
{
    int firstCount = 0;
    int secondCount = 0;
    std::weak_ptr<int> weakOwner;
    {
        KisAslCallbackObjectCatcher catcher;
        catcher.subscribeDouble(QStringLiteral("value"), [&firstCount](double) {
            ++firstCount;
        });
        catcher.subscribeDouble(QStringLiteral("value"), [&secondCount](double) {
            ++secondCount;
        });
        catcher.addDouble(QStringLiteral("value"), 1.0);

        auto owner = std::make_shared<int>(7);
        weakOwner = owner;
        catcher.subscribeInteger(QStringLiteral("owned"), [owner](int) { });
        owner.reset();
        QVERIFY(!weakOwner.expired());
    }

    QCOMPARE(firstCount, 0);
    QCOMPARE(secondCount, 1);
    QVERIFY(weakOwner.expired());
}

QTEST_GUILESS_MAIN(KisAslObjectCatcherContractTest)

#include "KisAslObjectCatcherContractTest.moc"
