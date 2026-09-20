/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPortingUtils.h>

#include <QApplication>
#include <QByteArray>
#include <QGuiApplication>
#include <QScreen>
#include <QTest>
#include <QTextStream>

class KisPortingUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void utf8StreamEncoding();
    void stringEndRemoval();
    void screenNumberFallback();
};

void KisPortingUtilsTest::utf8StreamEncoding()
{
    QByteArray bytes;
    QTextStream stream(&bytes, QIODevice::WriteOnly);
    KisPortingUtils::setUtf8OnStream(stream);

    const QString text = QStringLiteral("LibrePaint-日本語");
    stream << text;
    stream.flush();

    QCOMPARE(bytes, text.toUtf8());
}

void KisPortingUtilsTest::stringEndRemoval()
{
    QCOMPARE(KisPortingUtils::stringRemoveFirst(QStringLiteral("paint")), QStringLiteral("aint"));
    QCOMPARE(KisPortingUtils::stringRemoveFirst(QString()), QString());
    QCOMPARE(KisPortingUtils::stringRemoveLast(QStringLiteral("paint")), QStringLiteral("pain"));
    QCOMPARE(KisPortingUtils::stringRemoveLast(QString()), QString());
}

void KisPortingUtilsTest::screenNumberFallback()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    QVERIFY(!screens.isEmpty());
    const int primaryScreenIndex = screens.indexOf(QGuiApplication::primaryScreen());
    QVERIFY(primaryScreenIndex >= 0);

    QCOMPARE(KisPortingUtils::getScreenNumberForWidget(nullptr), primaryScreenIndex);
}

QTEST_MAIN(KisPortingUtilsTest)

#include "KisPortingUtilsTest.moc"
