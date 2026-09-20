/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QBuffer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <KisImportExportFilter.h>
#include <KoUpdater.h>

void kisSharedPtrAddReference(KisImage *);
bool kisSharedPtrRelease(KisImage *);
void kisSharedPtrAddReference(KisPropertiesConfiguration *);
bool kisSharedPtrRelease(KisPropertiesConfiguration *);

void kisSharedPtrAddReference(KisImage *)
{
}

bool kisSharedPtrRelease(KisImage *)
{
    return true;
}

void kisSharedPtrAddReference(KisPropertiesConfiguration *)
{
}

bool kisSharedPtrRelease(KisPropertiesConfiguration *)
{
    return true;
}

namespace
{
class ContractCheck final : public KisExportCheckBase
{
public:
    explicit ContractCheck(bool *destroyed)
        : KisExportCheckBase(QStringLiteral("FilterCapabilityContract"),
                             PARTIALLY,
                             QStringLiteral("The format simplifies this feature."),
                             true)
        , m_destroyed(destroyed)
    {
    }

    ~ContractCheck() override
    {
        *m_destroyed = true;
    }

    bool checkNeeded(KisImageSP) const override
    {
        return true;
    }

    Level check(KisImageSP) const override
    {
        return m_level;
    }

private:
    bool *m_destroyed;
};

class ContractFilter final : public KisImportExportFilter
{
public:
    explicit ContractFilter(bool *firstCheckDestroyed = nullptr, bool *secondCheckDestroyed = nullptr)
        : m_firstCheckDestroyed(firstCheckDestroyed)
        , m_secondCheckDestroyed(secondCheckDestroyed)
    {
    }

    KisImportExportErrorCode
    convert(KisDocument *document, QIODevice *io, KisPropertiesConfigurationSP configuration) override
    {
        convertedDocument = document;
        convertedDevice = io;
        convertedWithNullConfiguration = configuration.isNull();
        return KisImportExportErrorCode(ImportExportCodes::OK);
    }

    QString storedFilename() const
    {
        return filename();
    }

    QString storedRealFilename() const
    {
        return realFilename();
    }

    bool storedBatchMode() const
    {
        return batchMode();
    }

    KisImportUserFeedbackInterface *storedFeedbackInterface() const
    {
        return importUserFeedBackInterface();
    }

    QByteArray storedMimeType() const
    {
        return mimeType();
    }

    void reportProgress(int value)
    {
        setProgress(value);
    }

    KisDocument *convertedDocument{nullptr};
    QIODevice *convertedDevice{nullptr};
    bool convertedWithNullConfiguration{false};

protected:
    void initializeCapabilities() override
    {
        ++m_capabilityGeneration;
        bool *destroyed = m_capabilityGeneration == 1 ? m_firstCheckDestroyed : m_secondCheckDestroyed;
        addCapability(new ContractCheck(destroyed));
    }

private:
    int m_capabilityGeneration{0};
    bool *m_firstCheckDestroyed;
    bool *m_secondCheckDestroyed;
};

void writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
    file.close();
}
} // namespace

class KisImportExportFilterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stableCapabilityTagsAndDefaultSupport();
    void stateSettersPreserveFilterInputs();
    void convertDispatchesThroughPublicInterface();
    void updaterReceivesProgressAndCompletion();
    void defaultConfigurationHooksAreEmpty();
    void exportChecksReplaceAndOwnCapabilities();
    void verifiesWrittenFileContent();
};

void KisImportExportFilterTest::stableCapabilityTagsAndDefaultSupport()
{
    ContractFilter filter;
    KisImportExportFilter *interface = &filter;

    QCOMPARE(KisImportExportFilter::ImageContainsTransparencyTag, QStringLiteral("ImageContainsTransparency"));
    QCOMPARE(KisImportExportFilter::ColorModelIDTag, QStringLiteral("ColorModelID"));
    QCOMPARE(KisImportExportFilter::ColorDepthIDTag, QStringLiteral("ColorDepthID"));
    QCOMPARE(KisImportExportFilter::sRGBTag, QStringLiteral("sRGB"));
    QCOMPARE(KisImportExportFilter::HDRTag, QStringLiteral("HDRSupported"));
    QCOMPARE(KisImportExportFilter::CICPPrimariesTag, QStringLiteral("CICPCompatiblePrimaries"));
    QCOMPARE(KisImportExportFilter::CICPTransferCharacteristicsTag, QStringLiteral("CICPCompatibleTransferFunction"));
    QVERIFY(interface->supportsIO());
    QVERIFY(!interface->exportSupportsGuides());
    QVERIFY(interface->inherits("KisImportExportFilter"));
}

void KisImportExportFilterTest::stateSettersPreserveFilterInputs()
{
    ContractFilter filter;
    QObject feedbackSentinel;
    auto *feedback = reinterpret_cast<KisImportUserFeedbackInterface *>(&feedbackSentinel);

    filter.setBatchMode(true);
    filter.setFilename(QStringLiteral("display-name.png"));
    filter.setRealFilename(QStringLiteral("/tmp/real-name.png"));
    filter.setMimeType(QStringLiteral("image/png"));
    filter.setImportUserFeedBackInterface(feedback);

    QVERIFY(filter.storedBatchMode());
    QCOMPARE(filter.storedFilename(), QStringLiteral("display-name.png"));
    QCOMPARE(filter.storedRealFilename(), QStringLiteral("/tmp/real-name.png"));
    QCOMPARE(filter.storedMimeType(), QByteArrayLiteral("image/png"));
    QCOMPARE(filter.storedFeedbackInterface(), feedback);
}

void KisImportExportFilterTest::convertDispatchesThroughPublicInterface()
{
    ContractFilter filter;
    KisImportExportFilter *interface = &filter;
    QBuffer device;
    QVERIFY(device.open(QIODevice::ReadWrite));

    const KisImportExportErrorCode result = interface->convert(nullptr, &device, KisPropertiesConfigurationSP());

    QVERIFY(result.isOk());
    QCOMPARE(filter.convertedDocument, nullptr);
    QCOMPARE(filter.convertedDevice, &device);
    QVERIFY(filter.convertedWithNullConfiguration);
}

void KisImportExportFilterTest::updaterReceivesProgressAndCompletion()
{
    KoDummyUpdaterHolder updaterHolder;
    KoUpdater *updater = updaterHolder.updater();
    QSignalSpy progressSpy(updater, &KoUpdater::sigProgress);

    {
        ContractFilter filter;
        filter.setUpdater(updater);
        QCOMPARE(filter.updater(), updater);

        filter.reportProgress(37);
        QCOMPARE(updater->progress(), 37);
    }

    QCOMPARE(updater->progress(), 100);
    QCOMPARE(progressSpy.count(), 2);
    QCOMPARE(progressSpy.at(0).at(0).toInt(), 37);
    QCOMPARE(progressSpy.at(1).at(0).toInt(), 100);
}

void KisImportExportFilterTest::defaultConfigurationHooksAreEmpty()
{
    ContractFilter filter;
    KisImportExportFilter *interface = &filter;

    QVERIFY(interface->defaultConfiguration(QByteArrayLiteral("image/source"), QByteArrayLiteral("image/destination"))
                .isNull());
    QCOMPARE(interface->createConfigurationWidget(nullptr,
                                                  QByteArrayLiteral("image/source"),
                                                  QByteArrayLiteral("image/destination")),
             nullptr);
}

void KisImportExportFilterTest::exportChecksReplaceAndOwnCapabilities()
{
    bool firstCheckDestroyed = false;
    bool secondCheckDestroyed = false;
    {
        ContractFilter filter(&firstCheckDestroyed, &secondCheckDestroyed);

        const QMap<QString, KisExportCheckBase *> firstChecks = filter.exportChecks();
        QCOMPARE(firstChecks.size(), 1);
        KisExportCheckBase *firstCheck = firstChecks.value(QStringLiteral("FilterCapabilityContract"));
        QVERIFY(firstCheck);
        QCOMPARE(firstCheck->check(KisImageSP()), KisExportCheckBase::PARTIALLY);
        QCOMPARE(firstCheck->warning(), QStringLiteral("The format simplifies this feature."));
        QVERIFY(firstCheck->perLayerCheck());
        QVERIFY(!firstCheckDestroyed);

        const QMap<QString, KisExportCheckBase *> secondChecks = filter.exportChecks();
        QVERIFY(firstCheckDestroyed);
        QCOMPARE(secondChecks.size(), 1);
        QVERIFY(secondChecks.value(QStringLiteral("FilterCapabilityContract")));
        QVERIFY(!secondCheckDestroyed);
    }
    QVERIFY(secondCheckDestroyed);
}

void KisImportExportFilterTest::verifiesWrittenFileContent()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    ContractFilter filter;

    const QString missingPath = directory.filePath(QStringLiteral("missing.bin"));
    QVERIFY(filter.verify(missingPath).contains(QStringLiteral("does not exist")));

    const QString shortPath = directory.filePath(QStringLiteral("short.bin"));
    writeFile(shortPath, QByteArrayLiteral("123456789"));
    QVERIFY(filter.verify(shortPath).contains(QStringLiteral("smaller than 10 bytes")));

    const QString zeroPath = directory.filePath(QStringLiteral("zero.bin"));
    writeFile(zeroPath, QByteArray(16, '\0'));
    QVERIFY(filter.verify(zeroPath).contains(QStringLiteral("only zero bytes")));

    const QString validPath = directory.filePath(QStringLiteral("valid.bin"));
    QByteArray validData(1000, '\0');
    validData[999] = '\1';
    writeFile(validPath, validData);
    QCOMPARE(filter.verify(validPath), QString());

    const QString lateDataPath = directory.filePath(QStringLiteral("late-data.bin"));
    QByteArray lateData(1001, '\0');
    lateData[1000] = '\1';
    writeFile(lateDataPath, lateData);
    QVERIFY(filter.verify(lateDataPath).contains(QStringLiteral("only zero bytes")));
}

QTEST_GUILESS_MAIN(KisImportExportFilterTest)

#include "kis_import_export_filter_test.moc"
