/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "text/KoCSSFontInfo.h"

#include <QTest>

#include <cmath>

class KoCSSFontInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsMatchCssAndVariableFontConventions();
    void computedAxesHonorModesAndExplicitOverrides();
    void equalityComparesMaintainedFieldsAndAutomaticSize();
    void equalityOmitsFontSizeAdjustmentAndSlantAutomationState();
};

void KoCSSFontInfoContractTest::defaultsMatchCssAndVariableFontConventions()
{
    const KoCSSFontInfo info;

    QVERIFY(info.families.isEmpty());
    QCOMPARE(info.size, -1.0);
    QVERIFY(info.automaticOpticalSizing);
    QCOMPARE(info.fontSizeAdjust, 0.0);
    QCOMPARE(info.weight, 400.0);
    QCOMPARE(info.width, 100.0);
    QCOMPARE(info.slantMode, QFont::StyleNormal);
    QVERIFY(info.autoSlant);
    QCOMPARE(info.slantValue, 0.0);
    QVERIFY(info.axisSettings.isEmpty());
}

void KoCSSFontInfoContractTest::computedAxesHonorModesAndExplicitOverrides()
{
    KoCSSFontInfo info;
    info.size = 12.0;

    QMap<QString, double> settings = info.computedAxisSettings();
    QCOMPARE(settings.size(), 4);
    QCOMPARE(settings.value(QStringLiteral("wght")), 400.0);
    QCOMPARE(settings.value(QStringLiteral("wdth")), 100.0);
    QCOMPARE(settings.value(QStringLiteral("opsz")), 12.0);
    QCOMPARE(settings.value(QStringLiteral("ital")), 0.0);

    info.automaticOpticalSizing = false;
    info.slantMode = QFont::StyleItalic;
    settings = info.computedAxisSettings();
    QVERIFY(!settings.contains(QStringLiteral("opsz")));
    QCOMPARE(settings.value(QStringLiteral("ital")), 1.0);
    QVERIFY(!settings.contains(QStringLiteral("slnt")));

    info.slantMode = QFont::StyleOblique;
    info.autoSlant = true;
    settings = info.computedAxisSettings();
    QCOMPARE(settings.value(QStringLiteral("slnt")), -14.0);
    QVERIFY(!settings.contains(QStringLiteral("ital")));

    info.autoSlant = false;
    info.slantValue = 8.5;
    settings = info.computedAxisSettings();
    QCOMPARE(settings.value(QStringLiteral("slnt")), -8.5);

    info.axisSettings = {{QStringLiteral("wght"), 725.0},
                         {QStringLiteral("wdth"), 87.5},
                         {QStringLiteral("slnt"), -6.0},
                         {QStringLiteral("GRAD"), 0.25}};
    settings = info.computedAxisSettings();
    QCOMPARE(settings.value(QStringLiteral("wght")), 725.0);
    QCOMPARE(settings.value(QStringLiteral("wdth")), 87.5);
    QCOMPARE(settings.value(QStringLiteral("slnt")), -6.0);
    QCOMPARE(settings.value(QStringLiteral("GRAD")), 0.25);
}

void KoCSSFontInfoContractTest::equalityComparesMaintainedFieldsAndAutomaticSize()
{
    KoCSSFontInfo info;
    info.families = {QStringLiteral("Example Sans"), QStringLiteral("sans-serif")};
    info.size = 12.0;
    info.automaticOpticalSizing = false;
    info.fontSizeAdjust = 0.5;
    info.weight = 450.0;
    info.width = 92.0;
    info.slantMode = QFont::StyleOblique;
    info.autoSlant = false;
    info.slantValue = 9.0;
    info.axisSettings = {{QStringLiteral("GRAD"), 0.25}};

    QCOMPARE(info, KoCSSFontInfo(info));

    KoCSSFontInfo different = info;
    different.families = {QStringLiteral("Other Sans")};
    QVERIFY(!(info == different));

    different = info;
    different.size = 13.0;
    QVERIFY(!(info == different));

    different = info;
    different.size = std::nextafter(info.size, 13.0);
    QVERIFY(info == different);

    different = info;
    different.automaticOpticalSizing = true;
    QVERIFY(!(info == different));

    different = info;
    different.weight = 500.0;
    QVERIFY(!(info == different));

    different = info;
    different.width = 88.0;
    QVERIFY(!(info == different));

    different = info;
    different.slantMode = QFont::StyleItalic;
    QVERIFY(!(info == different));

    different = info;
    different.axisSettings.insert(QStringLiteral("GRAD"), 0.5);
    QVERIFY(!(info == different));

    different = info;
    different.weight = std::nextafter(info.weight, 451.0);
    different.width = std::nextafter(info.width, 93.0);
    QVERIFY(info == different);

    KoCSSFontInfo automaticSize = info;
    automaticSize.automaticOpticalSizing = true;
    KoCSSFontInfo otherAutomaticSize = automaticSize;
    otherAutomaticSize.size = 72.0;
    QVERIFY(automaticSize == otherAutomaticSize);
}

void KoCSSFontInfoContractTest::equalityOmitsFontSizeAdjustmentAndSlantAutomationState()
{
    KoCSSFontInfo info;
    info.fontSizeAdjust = 0.4;
    info.slantMode = QFont::StyleOblique;
    info.autoSlant = false;
    info.slantValue = 8.0;

    KoCSSFontInfo differentFontSizeAdjustment = info;
    differentFontSizeAdjustment.fontSizeAdjust = 0.9;
    QVERIFY(info == differentFontSizeAdjustment);

    KoCSSFontInfo differentManualAngle = info;
    differentManualAngle.slantValue = 18.0;
    QVERIFY(info == differentManualAngle);

    KoCSSFontInfo differentAutomation = info;
    differentAutomation.autoSlant = true;
    QVERIFY(info == differentAutomation);

    differentAutomation.slantValue = 18.0;
    QVERIFY(!(info == differentAutomation));
}

QTEST_GUILESS_MAIN(KoCSSFontInfoContractTest)

#include "KoCSSFontInfoContractTest.moc"
