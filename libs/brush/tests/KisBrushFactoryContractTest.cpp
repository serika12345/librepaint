/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_brush_factory.h>

#include <KoResourceLoadResult.h>
#include <KoResourceSignature.h>

#include <QDomDocument>
#include <QTest>

#include <memory>
#include <utility>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class BrushFactoryProbe final : public KisBrushFactory
{
public:
    BrushFactoryProbe() = default;

    ~BrushFactoryProbe() override
    {
        if (destructionCount) {
            ++*destructionCount;
        }
    }

    QString id() const override
    {
        ++idCallCount;
        return QStringLiteral("brush.factory.contract");
    }

    KoResourceLoadResult createBrush(const QDomElement &element, KisResourcesInterfaceSP resourcesInterface) override
    {
        ++xmlCreateCallCount;
        receivedXmlElement = &element;
        xmlResourcesInterface = resourcesInterface;
        return KoResourceLoadResult(KoResourceSignature(QStringLiteral("brushes"),
                                                        QStringLiteral("xml-md5"),
                                                        QStringLiteral("xml.gbr"),
                                                        QStringLiteral("XML Brush")));
    }

    KoResourceLoadResult createBrush(const KisBrushModel::BrushData &data,
                                     KisResourcesInterfaceSP resourcesInterface) override
    {
        ++modelCreateCallCount;
        receivedBrushData = &data;
        modelResourcesInterface = resourcesInterface;
        return KoResourceLoadResult(KoResourceSignature(QStringLiteral("brushes"),
                                                        QStringLiteral("model-md5"),
                                                        QStringLiteral("model.gbr"),
                                                        QStringLiteral("Model Brush")));
    }

    std::optional<KisBrushModel::BrushData> createBrushModel(const QDomElement &element,
                                                             KisResourcesInterfaceSP resourcesInterface) override
    {
        ++modelDecodeCallCount;
        receivedModelElement = &element;
        decodeResourcesInterface = resourcesInterface;

        KisBrushModel::BrushData result;
        result.common.angle = 0.75;
        result.type = KisBrushModel::Text;
        result.textBrush.text = QStringLiteral("Decoded contract brush");
        result.textBrush.font = QStringLiteral("Contract Font");
        return result;
    }

    void toXML(QDomDocument &document, QDomElement &element, const KisBrushModel::BrushData &model) override
    {
        ++xmlWriteCallCount;
        receivedDocument = &document;
        receivedWritableElement = &element;
        receivedSerializedModel = &model;
        element.setAttribute(QStringLiteral("factory-contract"), QStringLiteral("serialized"));
    }

    int *destructionCount = nullptr;

    mutable int idCallCount = 0;
    int xmlCreateCallCount = 0;
    const QDomElement *receivedXmlElement = nullptr;
    KisResourcesInterfaceSP xmlResourcesInterface;
    int modelCreateCallCount = 0;
    const KisBrushModel::BrushData *receivedBrushData = nullptr;
    KisResourcesInterfaceSP modelResourcesInterface;
    int modelDecodeCallCount = 0;
    const QDomElement *receivedModelElement = nullptr;
    KisResourcesInterfaceSP decodeResourcesInterface;
    int xmlWriteCallCount = 0;
    QDomDocument *receivedDocument = nullptr;
    QDomElement *receivedWritableElement = nullptr;
    const KisBrushModel::BrushData *receivedSerializedModel = nullptr;
};

class KisBrushFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultIdentityAndNameRemainStable();
    void xmlCreationPreservesArgumentsAndReturn();
    void modelCreationPreservesArgumentsAndReturn();
    void modelDecodingAndSerializationPreserveArgumentsAndResults();
    void baseOwnershipDestroysDerivedOnce();
};

void KisBrushFactoryContractTest::defaultIdentityAndNameRemainStable()
{
    BrushFactoryProbe factory;
    const KisBrushFactory &interface = factory;

    QCOMPARE(interface.id(), QStringLiteral("brush.factory.contract"));
    QCOMPARE(factory.idCallCount, 1);
    QVERIFY(interface.name().isEmpty());
}

void KisBrushFactoryContractTest::xmlCreationPreservesArgumentsAndReturn()
{
    BrushFactoryProbe factory;
    KisBrushFactory &interface = factory;
    QDomDocument document(QStringLiteral("brush-contract"));
    QDomElement element = document.createElement(QStringLiteral("brush"));
    document.appendChild(element);
    KisResourcesInterfaceSP resourcesInterface;

    const KoResourceLoadResult result = interface.createBrush(element, resourcesInterface);

    QCOMPARE(factory.xmlCreateCallCount, 1);
    QCOMPARE(factory.receivedXmlElement, &element);
    QCOMPARE(factory.xmlResourcesInterface, resourcesInterface);
    QCOMPARE(result.type(), KoResourceLoadResult::FailedLink);
    QCOMPARE(result.signature().type, QStringLiteral("brushes"));
    QCOMPARE(result.signature().md5sum, QStringLiteral("xml-md5"));
    QCOMPARE(result.signature().filename, QStringLiteral("xml.gbr"));
    QCOMPARE(result.signature().name, QStringLiteral("XML Brush"));
}

void KisBrushFactoryContractTest::modelCreationPreservesArgumentsAndReturn()
{
    BrushFactoryProbe factory;
    KisBrushFactory &interface = factory;
    KisBrushModel::BrushData data;
    data.common.angle = 0.25;
    data.type = KisBrushModel::Text;
    data.textBrush.text = QStringLiteral("Input contract brush");
    data.textBrush.font = QStringLiteral("Contract Font");
    KisResourcesInterfaceSP resourcesInterface;

    const KoResourceLoadResult result = interface.createBrush(data, resourcesInterface);

    QCOMPARE(factory.modelCreateCallCount, 1);
    QCOMPARE(factory.receivedBrushData, &data);
    QCOMPARE(factory.modelResourcesInterface, resourcesInterface);
    QCOMPARE(result.type(), KoResourceLoadResult::FailedLink);
    QCOMPARE(result.signature().type, QStringLiteral("brushes"));
    QCOMPARE(result.signature().md5sum, QStringLiteral("model-md5"));
    QCOMPARE(result.signature().filename, QStringLiteral("model.gbr"));
    QCOMPARE(result.signature().name, QStringLiteral("Model Brush"));
}

void KisBrushFactoryContractTest::modelDecodingAndSerializationPreserveArgumentsAndResults()
{
    BrushFactoryProbe factory;
    KisBrushFactory &interface = factory;
    QDomDocument document(QStringLiteral("brush-contract"));
    QDomElement element = document.createElement(QStringLiteral("brush"));
    document.appendChild(element);
    KisResourcesInterfaceSP resourcesInterface;

    const auto decoded = interface.createBrushModel(element, resourcesInterface);

    QVERIFY(decoded.has_value());
    QCOMPARE(factory.modelDecodeCallCount, 1);
    QCOMPARE(factory.receivedModelElement, &element);
    QCOMPARE(factory.decodeResourcesInterface, resourcesInterface);
    QCOMPARE(decoded->common.angle, 0.75);
    QCOMPARE(decoded->type, KisBrushModel::Text);
    QCOMPARE(decoded->textBrush.text, QStringLiteral("Decoded contract brush"));
    QCOMPARE(decoded->textBrush.font, QStringLiteral("Contract Font"));

    interface.toXML(document, element, *decoded);

    QCOMPARE(factory.xmlWriteCallCount, 1);
    QCOMPARE(factory.receivedDocument, &document);
    QCOMPARE(factory.receivedWritableElement, &element);
    QCOMPARE(factory.receivedSerializedModel, &*decoded);
    QCOMPARE(element.attribute(QStringLiteral("factory-contract")), QStringLiteral("serialized"));
}

void KisBrushFactoryContractTest::baseOwnershipDestroysDerivedOnce()
{
    int destructionCount = 0;

    {
        auto factory = std::make_unique<BrushFactoryProbe>();
        factory->destructionCount = &destructionCount;
        std::unique_ptr<KisBrushFactory> interface = std::move(factory);

        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_MAIN(KisBrushFactoryContractTest)

#include "KisBrushFactoryContractTest.moc"
