/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTest>

#include <KisImportExportFilter.h>
#include <kis_image_config.h>

namespace
{
class SavedConfigurationFilter final : public KisImportExportFilter
{
public:
    KisImportExportErrorCode convert(KisDocument *, QIODevice *, KisPropertiesConfigurationSP) override
    {
        return KisImportExportErrorCode(ImportExportCodes::OK);
    }

    KisPropertiesConfigurationSP defaultConfiguration(const QByteArray &from, const QByteArray &to) const override
    {
        requestedSource = from;
        requestedDestination = to;
        KisPropertiesConfigurationSP configuration = new KisPropertiesConfiguration();
        configuration->setProperty(QStringLiteral("default-only"), QStringLiteral("kept"));
        configuration->setProperty(QStringLiteral("overridden"), QStringLiteral("default"));
        return configuration;
    }

    mutable QByteArray requestedSource;
    mutable QByteArray requestedDestination;
};
} // namespace

class KisImportExportFilterSavedConfigurationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void savedConfigurationOverlaysDefaultsForRequestedFormat();
};

void KisImportExportFilterSavedConfigurationTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisImportExportFilterSavedConfigurationTest::savedConfigurationOverlaysDefaultsForRequestedFormat()
{
    const QByteArray source = QByteArrayLiteral("image/source-contract");
    const QByteArray destination =
        QByteArrayLiteral("image/destination-contract-") + QByteArray::number(QCoreApplication::applicationPid());

    KisPropertiesConfigurationSP saved = new KisPropertiesConfiguration();
    saved->setProperty(QStringLiteral("saved-only"), QStringLiteral("loaded"));
    saved->setProperty(QStringLiteral("overridden"), QStringLiteral("saved"));
    KisImageConfig(false).setExportConfiguration(QString::fromLatin1(destination), saved);

    SavedConfigurationFilter filter;
    const KisPropertiesConfigurationSP configuration = filter.lastSavedConfiguration(source, destination);

    QCOMPARE(filter.requestedSource, source);
    QCOMPARE(filter.requestedDestination, destination);
    QCOMPARE(configuration->getString(QStringLiteral("default-only")), QStringLiteral("kept"));
    QCOMPARE(configuration->getString(QStringLiteral("saved-only")), QStringLiteral("loaded"));
    QCOMPARE(configuration->getString(QStringLiteral("overridden")), QStringLiteral("saved"));

    const QByteArray missingDestination = destination + QByteArrayLiteral("-missing");
    const KisPropertiesConfigurationSP defaults = filter.lastSavedConfiguration(source, missingDestination);
    QCOMPARE(filter.requestedDestination, missingDestination);
    QCOMPARE(defaults->getString(QStringLiteral("default-only")), QStringLiteral("kept"));
    QCOMPARE(defaults->getString(QStringLiteral("overridden")), QStringLiteral("default"));
    QVERIFY(!defaults->hasProperty(QStringLiteral("saved-only")));
}

QTEST_GUILESS_MAIN(KisImportExportFilterSavedConfigurationTest)

#include "kis_import_export_filter_saved_configuration_test.moc"
