#include "KisSafeDocumentLoaderTest.h"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <testui.h>

#include "config-limit-long-tests.h"
#include "kis_safe_document_loader.h"
#include <kis_paint_device.h>

namespace {
void checkLoadedColor(const QSignalSpy &spy, const QColor &expected)
{
    QVERIFY(!spy.isEmpty());
    const auto device = spy.last().first().value<KisPaintDeviceSP>();
    QVERIFY(device);
    QColor actual;
    QVERIFY(device->pixel(0, 0, &actual));
    QCOMPARE(actual, expected);
}

bool writeToFile(QFile &file, const QColor &color)
{
    if (!file.reset() || !file.resize(0)) return false;
    QImage img(10,10,QImage::Format_ARGB32);
    img.fill(color);
    return img.save(&file, "PNG") && file.flush();
}

}

void KisSafeDocumentLoaderTest::test()
{
    QTemporaryFile file("safe_loader_test_XXXXXX.png");

    QVERIFY(file.open());
    QVERIFY(writeToFile(file, Qt::black));

    KisSafeDocumentLoader loader(file.fileName());

    QSignalSpy spy(&loader, &KisSafeDocumentLoader::loadingFinished);
    QSignalSpy spyFailed(&loader, &KisSafeDocumentLoader::loadingFailed);

    // reloadImage() is synchronous
    loader.reloadImage();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spyFailed.size(), 0);
    checkLoadedColor(spy, Qt::black);

    QVERIFY(writeToFile(file, Qt::white));

    QTRY_COMPARE_WITH_TIMEOUT(spy.size(), 2, 6000);
    QCOMPARE(spyFailed.size(), 0);
    checkLoadedColor(spy, Qt::white);

    spy.clear();

    QVERIFY(file.reset());
    QVERIFY(file.resize(0));
    const auto corrupt = QByteArrayLiteral("blah-blah-try-read-me");
    QCOMPARE(file.write(corrupt), corrupt.size());
    QVERIFY(file.flush());

    QTRY_COMPARE_WITH_TIMEOUT(spyFailed.size(), 1, 6000);
    QCOMPARE(spy.size(), 0);

}

void KisSafeDocumentLoaderTest::testFileLost()
{
    QTemporaryFile file("safe_loader_test_XXXXXX.png");

    QVERIFY(file.open());
    QVERIFY(writeToFile(file, Qt::black));

    KisSafeDocumentLoader loader(file.fileName());

    QSignalSpy spy(&loader, &KisSafeDocumentLoader::loadingFinished);
    QSignalSpy spyFailed(&loader, &KisSafeDocumentLoader::loadingFailed);
    QSignalSpy spyExistsState(&loader, &KisSafeDocumentLoader::fileExistsStateChanged);

    // reloadImage() is synchronous
    loader.reloadImage();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spyFailed.size(), 0);
    QCOMPARE(spyExistsState.size(), 0);
    checkLoadedColor(spy, Qt::black);

    spy.clear();

    file.close();
    QVERIFY(file.remove());

    QTRY_COMPARE_WITH_TIMEOUT(spyExistsState.size(), 1, 15000);
    QCOMPARE(spyExistsState[0][0].toBool(), false);
    spyExistsState.clear();

    QVERIFY(file.open());
    QVERIFY(writeToFile(file, Qt::white));

    QTRY_COMPARE_WITH_TIMEOUT(spyExistsState.size(), 1, 3500);
    QCOMPARE(spyFailed.size(), 0);
    QCOMPARE(spyExistsState[0][0].toBool(), true);
    spyExistsState.clear();

    QTRY_COMPARE_WITH_TIMEOUT(spy.size(), 1, 6000);
    QCOMPARE(spyFailed.size(), 0);
    QCOMPARE(spyExistsState.size(), 0);
    checkLoadedColor(spy, Qt::white);
    spy.clear();


    QVERIFY(writeToFile(file, Qt::yellow));
    QTRY_COMPARE_WITH_TIMEOUT(spy.size(), 1, 6000);
    QCOMPARE(spyFailed.size(), 0);
    QCOMPARE(spyExistsState.size(), 0);
    checkLoadedColor(spy, Qt::yellow);
}

KISTEST_MAIN(KisSafeDocumentLoaderTest)
