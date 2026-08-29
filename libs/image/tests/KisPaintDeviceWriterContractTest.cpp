/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QByteArray>
#include <QTest>

#include "kis_paint_device_writer.h"

#include <memory>

class RecordingWriter : public KisPaintDeviceWriter
{
public:
    explicit RecordingWriter(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingWriter() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    bool write(const QByteArray &data) override
    {
        ++byteArrayCallCount;
        byteArrayPayload = data;
        return byteArrayResult;
    }

    bool write(const char *data, qint64 length) override
    {
        ++rawCallCount;
        rawLength = length;
        rawPayload = QByteArray(data, static_cast<int>(length));
        return rawResult;
    }

    int byteArrayCallCount = 0;
    int rawCallCount = 0;
    QByteArray byteArrayPayload;
    QByteArray rawPayload;
    qint64 rawLength = -1;
    bool byteArrayResult = false;
    bool rawResult = true;

private:
    int *m_destructionCount;
};

class KisPaintDeviceWriterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void virtualWritesPreservePayloadsLengthsAndResults();
    void baseOwnershipDestroysDerivedExactlyOnce();
};

void KisPaintDeviceWriterContractTest::virtualWritesPreservePayloadsLengthsAndResults()
{
    RecordingWriter writer;
    KisPaintDeviceWriter &interface = writer;

    const QByteArray byteArrayPayload("alpha\0omega", 11);
    QCOMPARE(interface.write(byteArrayPayload), writer.byteArrayResult);
    QCOMPARE(writer.byteArrayCallCount, 1);
    QCOMPARE(writer.byteArrayPayload, byteArrayPayload);

    const char rawPayload[] = {'b', 'e', 't', 'a', '\0', 't', 'a', 'i', 'l'};
    QCOMPARE(interface.write(rawPayload, 5), writer.rawResult);
    QCOMPARE(writer.rawCallCount, 1);
    QCOMPARE(writer.rawLength, 5);
    QCOMPARE(writer.rawPayload, QByteArray(rawPayload, 5));
}

void KisPaintDeviceWriterContractTest::baseOwnershipDestroysDerivedExactlyOnce()
{
    int destructionCount = 0;

    {
        std::unique_ptr<KisPaintDeviceWriter> writer = std::make_unique<RecordingWriter>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisPaintDeviceWriterContractTest)

#include "KisPaintDeviceWriterContractTest.moc"
