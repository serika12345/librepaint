/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFontDatabase>
#include <QLocale>
#include <QTest>

#include "text/KoWritingSystemUtils.h"

class KoWritingSystemUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writingSystemMappings();
    void localeScriptMappings();
    void characterScriptMappings();
    void samplesAndLocaleTags();
    void bcp47ValueFormatting();
    void parseBcp47Locale();
    void numericPrimaryLanguageIsAccepted();
    void localeConversions();
};

void KoWritingSystemUtilsContractTest::writingSystemMappings()
{
    QCOMPARE(KoWritingSystemUtils::scriptTagForWritingSystem(QFontDatabase::Any), QStringLiteral("Zyyy"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForWritingSystem(QFontDatabase::Greek), QStringLiteral("Grek"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForWritingSystem(QFontDatabase::Vietnamese), QStringLiteral("Latn"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForWritingSystem(QFontDatabase::Symbol), QString());

    QCOMPARE(KoWritingSystemUtils::writingSystemForScriptTag(QStringLiteral("Grek")), QFontDatabase::Greek);
    QCOMPARE(KoWritingSystemUtils::writingSystemForScriptTag(QStringLiteral("Latn")), QFontDatabase::Latin);
    QCOMPARE(KoWritingSystemUtils::writingSystemForScriptTag(QStringLiteral("missing")), QFontDatabase::Any);
}

void KoWritingSystemUtilsContractTest::localeScriptMappings()
{
    QCOMPARE(KoWritingSystemUtils::scriptTagForQLocaleScript(QLocale::GreekScript), QStringLiteral("Grek"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForQLocaleScript(QLocale::HanWithBopomofoScript), QStringLiteral("Hanb"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForQLocaleScript(QLocale::AnyScript), QString());

    QCOMPARE(KoWritingSystemUtils::scriptForScriptTag(QStringLiteral("Grek")), QLocale::GreekScript);
    QCOMPARE(KoWritingSystemUtils::scriptForScriptTag(QStringLiteral("Hanb")), QLocale::HanWithBopomofoScript);
    QCOMPARE(KoWritingSystemUtils::scriptForScriptTag(QStringLiteral("missing")), QLocale::AnyScript);
}

void KoWritingSystemUtilsContractTest::characterScriptMappings()
{
    QCOMPARE(KoWritingSystemUtils::scriptTagForQCharScript(QChar::Script_Greek), QStringLiteral("Grek"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForQCharScript(QChar::Script_Yezidi), QStringLiteral("Yezi"));
    QCOMPARE(KoWritingSystemUtils::scriptTagForQCharScript(QChar::Script_Unknown), QString());

    QCOMPARE(KoWritingSystemUtils::qCharScriptForScriptTag(QStringLiteral("Grek")), QChar::Script_Greek);
    QCOMPARE(KoWritingSystemUtils::qCharScriptForScriptTag(QStringLiteral("Yezi")), QChar::Script_Yezidi);
    QCOMPARE(KoWritingSystemUtils::qCharScriptForScriptTag(QStringLiteral("missing")), QChar::Script_Unknown);
}

void KoWritingSystemUtilsContractTest::samplesAndLocaleTags()
{
    const QMap<QString, QString> samples = KoWritingSystemUtils::samples();

    QCOMPARE(samples.value(QStringLiteral("AaBbGg")), QStringLiteral("s_Latn"));
    QCOMPARE(samples.value(QStringLiteral("☺❤⚓🌈")), QStringLiteral("s_Zsye"));
    QCOMPARE(samples.value(QStringLiteral("∆∅∞≠")), QStringLiteral("s_Zmth"));
    QCOMPARE(samples.value(QStringLiteral("←↕↝↴")), QStringLiteral("s_Zsym"));

    for (auto it = samples.cbegin(); it != samples.cend(); ++it) {
        QVERIFY2(it.value().startsWith(QLatin1String("s_")) || it.value().startsWith(QLatin1String("l_")),
                 qPrintable(it.value()));
    }

    QCOMPARE(KoWritingSystemUtils::sampleTagForQLocale(QLocale(QLocale::English)), QStringLiteral("s_Latn"));
    QCOMPARE(KoWritingSystemUtils::sampleTagForQLocale(QLocale(QLocale::Japanese)), QStringLiteral("s_Jpan"));
    QCOMPARE(KoWritingSystemUtils::sampleTagForQLocale(
                 QLocale(QLocale::Vietnamese, QLocale::LatinScript, QLocale::AnyCountry)),
             QStringLiteral("l_vi"));
}

void KoWritingSystemUtilsContractTest::bcp47ValueFormatting()
{
    KoWritingSystemUtils::Bcp47Locale locale;
    QVERIFY(!locale.isValid());
    QCOMPARE(locale.toString(), QString());
    QCOMPARE(locale.toPosixLocaleFormat(), QString());

    locale.languageTags = {QStringLiteral("zh"), QStringLiteral("cmn")};
    locale.scriptTag = QStringLiteral("Hant");
    locale.regionTag = QStringLiteral("hk");
    locale.variantTags = {QStringLiteral("variant")};
    locale.extensionTags = {QStringLiteral("u-ca-buddhist")};
    locale.privateUseTags = {QStringLiteral("x-private")};

    QVERIFY(locale.isValid());
    QCOMPARE(locale.toString(), QStringLiteral("zh-cmn-Hant-hk-variant-u-ca-buddhist-x-private"));
    QCOMPARE(locale.toPosixLocaleFormat(), QStringLiteral("zh_Hant_HK"));
}

void KoWritingSystemUtilsContractTest::parseBcp47Locale()
{
    const KoWritingSystemUtils::Bcp47Locale locale =
        KoWritingSystemUtils::parseBcp47Locale(QStringLiteral("ZH-cmn-hant-hk-VARIANT-u-ca-buddhist-x-PRIVATE"));

    QCOMPARE(locale.languageTags, QStringList({QStringLiteral("zh"), QStringLiteral("cmn")}));
    QCOMPARE(locale.scriptTag, QStringLiteral("Hant"));
    QCOMPARE(locale.regionTag, QStringLiteral("HK"));
    QCOMPARE(locale.variantTags, QStringList({QStringLiteral("variant")}));
    QCOMPARE(locale.extensionTags, QStringList({QStringLiteral("u-ca-buddhist")}));
    QCOMPARE(locale.privateUseTags, QStringList({QStringLiteral("x-private")}));
    QCOMPARE(locale.toString(), QStringLiteral("zh-cmn-Hant-HK-variant-u-ca-buddhist-x-private"));

    const KoWritingSystemUtils::Bcp47Locale numericRegion =
        KoWritingSystemUtils::parseBcp47Locale(QStringLiteral("es-419"));
    QCOMPARE(numericRegion.regionTag, QStringLiteral("419"));

    const KoWritingSystemUtils::Bcp47Locale grandfathered =
        KoWritingSystemUtils::parseBcp47Locale(QStringLiteral("art-lojban"));
    QCOMPARE(grandfathered.languageTags, QStringList({QStringLiteral("jbo")}));
}

void KoWritingSystemUtilsContractTest::numericPrimaryLanguageIsAccepted()
{
    const KoWritingSystemUtils::Bcp47Locale locale = KoWritingSystemUtils::parseBcp47Locale(QStringLiteral("1"));

    QCOMPARE(locale.languageTags, QStringList({QStringLiteral("1")}));
    QVERIFY(locale.isValid());
    QCOMPARE(locale.toString(), QStringLiteral("1"));
    QCOMPARE(locale.toPosixLocaleFormat(), QStringLiteral("1"));
}

void KoWritingSystemUtilsContractTest::localeConversions()
{
    KoWritingSystemUtils::Bcp47Locale value;
    value.languageTags = {QStringLiteral("en")};
    value.scriptTag = QStringLiteral("Latn");
    value.regionTag = QStringLiteral("US");

    const QLocale fromValue = KoWritingSystemUtils::localeFromBcp47Locale(value);
    const QLocale fromString = KoWritingSystemUtils::localeFromBcp47Locale(QStringLiteral("en-Latn-US"));

    QCOMPARE(fromValue, fromString);
    QCOMPARE(fromValue.language(), QLocale::English);
    QCOMPARE(fromValue.script(), QLocale::LatinScript);
    QCOMPARE(fromValue.territory(), QLocale::UnitedStates);
}

QTEST_MAIN(KoWritingSystemUtilsContractTest)

#include "KoWritingSystemUtilsContractTest.moc"
