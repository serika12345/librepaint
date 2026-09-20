/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorTransformationFactory.h>
#include <KoID.h>

#include <QHash>
#include <QTest>
#include <QVariant>

#include <memory>
#include <utility>

namespace
{

class FactoryProbe : public KoColorTransformationFactory
{
public:
    FactoryProbe(const QString &id,
                 QList<QPair<KoID, KoID>> models,
                 KoColorTransformation *result,
                 int *destructionCount = nullptr)
        : KoColorTransformationFactory(id)
        , m_models(std::move(models))
        , m_result(result)
        , m_destructionCount(destructionCount)
    {
    }

    ~FactoryProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QList<QPair<KoID, KoID>> supportedModels() const override
    {
        ++supportedModelsCallCount;
        return m_models;
    }

    KoColorTransformation *createTransformation(const KoColorSpace *colorSpace,
                                                QHash<QString, QVariant> parameters) const override
    {
        ++createCallCount;
        receivedColorSpace = colorSpace;
        receivedParameters = std::move(parameters);
        return m_result;
    }

    mutable int supportedModelsCallCount = 0;
    mutable int createCallCount = 0;
    mutable const KoColorSpace *receivedColorSpace = nullptr;
    mutable QHash<QString, QVariant> receivedParameters;

private:
    QList<QPair<KoID, KoID>> m_models;
    KoColorTransformation *m_result;
    int *m_destructionCount;
};

} // namespace

class KoColorTransformationFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identifierAndSupportedModelsPreserveValues();
    void creationDispatchesBorrowedSpaceParametersAndReturn();
    void baseOwnershipDestroysDerivedExactlyOnce();
};

void KoColorTransformationFactoryContractTest::identifierAndSupportedModelsPreserveValues()
{
    const QList<QPair<KoID, KoID>> models{
        {KoID(QStringLiteral("RGBA"), QStringLiteral("RGB/Alpha")),
         KoID(QStringLiteral("U8"), QStringLiteral("8-bit integer"))},
        {KoID(QStringLiteral("CMYKA"), QStringLiteral("CMYK/Alpha")),
         KoID(QStringLiteral("U16"), QStringLiteral("16-bit integer"))},
    };
    FactoryProbe factory(QStringLiteral("factory.contract"), models, nullptr);

    QCOMPARE(factory.id(), QStringLiteral("factory.contract"));

    const auto supportedModels = factory.supportedModels();
    QCOMPARE(factory.supportedModelsCallCount, 1);
    QCOMPARE(supportedModels.size(), 2);
    QCOMPARE(supportedModels.at(0).first.id(), QStringLiteral("RGBA"));
    QCOMPARE(supportedModels.at(0).first.name(), QStringLiteral("RGB/Alpha"));
    QCOMPARE(supportedModels.at(0).second.id(), QStringLiteral("U8"));
    QCOMPARE(supportedModels.at(0).second.name(), QStringLiteral("8-bit integer"));
    QCOMPARE(supportedModels.at(1).first.id(), QStringLiteral("CMYKA"));
    QCOMPARE(supportedModels.at(1).first.name(), QStringLiteral("CMYK/Alpha"));
    QCOMPARE(supportedModels.at(1).second.id(), QStringLiteral("U16"));
    QCOMPARE(supportedModels.at(1).second.name(), QStringLiteral("16-bit integer"));
}

void KoColorTransformationFactoryContractTest::creationDispatchesBorrowedSpaceParametersAndReturn()
{
    int colorSpaceStorage = 0;
    int transformationStorage = 0;
    const auto *colorSpace = reinterpret_cast<const KoColorSpace *>(&colorSpaceStorage);
    auto *transformation = reinterpret_cast<KoColorTransformation *>(&transformationStorage);
    FactoryProbe factory(QStringLiteral("factory.contract"), {}, transformation);
    const QHash<QString, QVariant> parameters{
        {QStringLiteral("gain"), 1.25},
        {QStringLiteral("mode"), QStringLiteral("perceptual")},
    };

    QCOMPARE(factory.createTransformation(colorSpace, parameters), transformation);
    QCOMPARE(factory.createCallCount, 1);
    QCOMPARE(factory.receivedColorSpace, colorSpace);
    QCOMPARE(factory.receivedParameters, parameters);
}

void KoColorTransformationFactoryContractTest::baseOwnershipDestroysDerivedExactlyOnce()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KoColorTransformationFactory> factory =
            std::make_unique<FactoryProbe>(QStringLiteral("owned.factory"),
                                           QList<QPair<KoID, KoID>>{},
                                           nullptr,
                                           &destructionCount);
        QCOMPARE(factory->id(), QStringLiteral("owned.factory"));
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoColorTransformationFactoryContractTest)

#include "KoColorTransformationFactoryContractTest.moc"
