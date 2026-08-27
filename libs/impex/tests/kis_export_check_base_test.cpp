/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisExportCheckBase.h>

void kisSharedPtrAddReference(KisImage *);
bool kisSharedPtrRelease(KisImage *);

#include <kis_shared_ptr.h>

#include <memory>

void kisSharedPtrAddReference(KisImage *)
{
}

bool kisSharedPtrRelease(KisImage *)
{
    return true;
}

namespace
{
class ContractCheck final : public KisExportCheckBase
{
public:
    ContractCheck(const QString &id,
                  Level level,
                  const QString &warning = QString(),
                  bool perLayerCheck = false,
                  bool *destroyed = nullptr)
        : KisExportCheckBase(id, level, warning, perLayerCheck)
        , m_destroyed(destroyed)
    {
    }

    ~ContractCheck() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    bool checkNeeded(KisImageSP image) const override
    {
        return image.isNull();
    }

    Level check(KisImageSP) const override
    {
        return m_level;
    }

private:
    bool *m_destroyed;
};

class ContractFactory final : public KisExportCheckFactory
{
public:
    explicit ContractFactory(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~ContractFactory() override
    {
        *m_destroyed = true;
    }

    KisExportCheckBase *create(KisExportCheckBase::Level level, const QString &customWarning = QString()) override
    {
        return new ContractCheck(id(), level, customWarning);
    }

    QString id() const override
    {
        return QStringLiteral("ContractCheck");
    }

private:
    bool *m_destroyed;
};
} // namespace

class KisExportCheckBaseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultPropertiesAndDispatch();
    void customPropertiesAndPartialSupport();
    void unsupportedCheckUsesVirtualDestruction();
    void factoryCreatesChecksThroughItsInterface();
};

void KisExportCheckBaseTest::defaultPropertiesAndDispatch()
{
    ContractCheck check(QStringLiteral("DefaultContract"), KisExportCheckBase::SUPPORTED);
    KisExportCheckBase *interface = &check;

    QCOMPARE(interface->id(), QStringLiteral("DefaultContract"));
    QCOMPARE(interface->warning(), QString());
    QVERIFY(!interface->perLayerCheck());
    QVERIFY(interface->checkNeeded(KisImageSP()));
    QCOMPARE(interface->check(KisImageSP()), KisExportCheckBase::SUPPORTED);
}

void KisExportCheckBaseTest::customPropertiesAndPartialSupport()
{
    ContractCheck check(QStringLiteral("LayerContract"),
                        KisExportCheckBase::PARTIALLY,
                        QStringLiteral("Layer data will be simplified."),
                        true);

    QCOMPARE(check.id(), QStringLiteral("LayerContract"));
    QCOMPARE(check.warning(), QStringLiteral("Layer data will be simplified."));
    QVERIFY(check.perLayerCheck());
    QCOMPARE(check.check(KisImageSP()), KisExportCheckBase::PARTIALLY);
}

void KisExportCheckBaseTest::unsupportedCheckUsesVirtualDestruction()
{
    bool destroyed = false;
    std::unique_ptr<KisExportCheckBase> check = std::make_unique<ContractCheck>(QStringLiteral("UnsupportedContract"),
                                                                                KisExportCheckBase::UNSUPPORTED,
                                                                                QString(),
                                                                                false,
                                                                                &destroyed);

    QCOMPARE(check->check(KisImageSP()), KisExportCheckBase::UNSUPPORTED);
    check.reset();
    QVERIFY(destroyed);
}

void KisExportCheckBaseTest::factoryCreatesChecksThroughItsInterface()
{
    bool factoryDestroyed = false;
    std::unique_ptr<KisExportCheckFactory> factory = std::make_unique<ContractFactory>(&factoryDestroyed);

    QCOMPARE(factory->id(), QStringLiteral("ContractCheck"));
    std::unique_ptr<KisExportCheckBase> check(
        factory->create(KisExportCheckBase::PARTIALLY, QStringLiteral("Factory warning")));
    QCOMPARE(check->id(), factory->id());
    QCOMPARE(check->check(KisImageSP()), KisExportCheckBase::PARTIALLY);
    QCOMPARE(check->warning(), QStringLiteral("Factory warning"));

    factory.reset();
    QVERIFY(factoryDestroyed);
}

QTEST_GUILESS_MAIN(KisExportCheckBaseTest)

#include "kis_export_check_base_test.moc"
