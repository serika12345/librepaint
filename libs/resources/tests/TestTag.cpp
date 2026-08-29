/*
 * SPDX-FileCopyrightText: 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestTag.h"

#include <KisTag.h>

#include <QBuffer>
#include <QDebug>
#include <QLocale>
#include <QRegularExpression>
#include <QTest>

#include <KLocalizedString>

#include <memory>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected tag assertion: %s at %s:%d", assertion, file, line);
}

namespace
{
class TagProbe final : public KisTag
{
public:
    explicit TagProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~TagProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class LocaleStateGuard
{
public:
    LocaleStateGuard()
        : m_languages(KLocalizedString::languages())
        , m_locale(QLocale())
    {
    }

    ~LocaleStateGuard()
    {
        KLocalizedString::setLanguages(m_languages);
        QLocale::setDefault(m_locale);
    }

private:
    QStringList m_languages;
    QLocale m_locale;
};

class OpenRecordingBuffer final : public QBuffer
{
public:
    bool open(QIODeviceBase::OpenMode mode) override
    {
        ++openCallCount;
        requestedMode = mode;
        return QBuffer::open(mode);
    }

    int openCallCount{0};
    QIODeviceBase::OpenMode requestedMode{QIODeviceBase::NotOpen};
};

void populateTag(KisTag &tag)
{
    tag.setFilename(QStringLiteral("source.tag"));
    tag.setUrl(QStringLiteral("favorites"));
    tag.setName(QStringLiteral("Favorites"));
    tag.setNames({{QStringLiteral("ja_JP"), QStringLiteral("お気に入り")},
                  {QStringLiteral("nl"), QStringLiteral("Favorieten")}});
    tag.setComment(QStringLiteral("Favorite presets"));
    tag.setComments({{QStringLiteral("ja_JP"), QStringLiteral("お気に入りのプリセット")},
                     {QStringLiteral("nl"), QStringLiteral("Favoriete voorinstellingen")}});
    tag.setResourceType(QStringLiteral("paintoppresets"));
    tag.setDefaultResources({QStringLiteral("ink.kpp"), QStringLiteral("鉛筆.kpp")});
}

void comparePublicValues(KisTag &actual, KisTag &expected)
{
    QCOMPARE(actual.valid(), expected.valid());
    QCOMPARE(actual.id(), expected.id());
    QCOMPARE(actual.active(), expected.active());
    QCOMPARE(actual.filename(), expected.filename());
    QCOMPARE(actual.url(), expected.url());
    QCOMPARE(actual.name(false), expected.name(false));
    QCOMPARE(actual.names(), expected.names());
    QCOMPARE(actual.comment(false), expected.comment(false));
    QCOMPARE(actual.comments(), expected.comments());
    QCOMPARE(actual.resourceType(), expected.resourceType());
    QCOMPARE(actual.defaultResources(), expected.defaultResources());
}
} // namespace

void TestTag::defaultStateAndValueProperties()
{
    bool destroyed = false;
    {
        std::unique_ptr<KisTag> tag(new TagProbe(&destroyed));
        QVERIFY(!tag->valid());
        QCOMPARE(tag->id(), -1);
        QVERIFY(tag->active());

        populateTag(*tag);
        QCOMPARE(tag->filename(), QStringLiteral("source.tag"));
        QCOMPARE(tag->url(), QStringLiteral("favorites"));
        QCOMPARE(tag->name(false), QStringLiteral("Favorites"));
        QCOMPARE(tag->names().size(), 2);
        QCOMPARE(tag->comment(false), QStringLiteral("Favorite presets"));
        QCOMPARE(tag->comments().size(), 2);
        QCOMPARE(tag->resourceType(), QStringLiteral("paintoppresets"));
        QCOMPARE(tag->defaultResources(), QStringList({QStringLiteral("ink.kpp"), QStringLiteral("鉛筆.kpp")}));
        QVERIFY(!destroyed);
    }
    QVERIFY(destroyed);
}

void TestTag::localizedNamesCommentsAndLocaleSelection()
{
    LocaleStateGuard localeState;
    KisTag tag;
    populateTag(tag);

    KLocalizedString::setLanguages({QStringLiteral("nl"), QStringLiteral("ja_JP")});
    QCOMPARE(KisTag::currentLocale(), QStringLiteral("nl"));
    QCOMPARE(tag.name(), QStringLiteral("Favorieten"));
    QCOMPARE(tag.comment(), QStringLiteral("Favoriete voorinstellingen"));
    QCOMPARE(tag.name(false), QStringLiteral("Favorites"));
    QCOMPARE(tag.comment(false), QStringLiteral("Favorite presets"));

    KLocalizedString::setLanguages({QStringLiteral("de")});
    QCOMPARE(tag.name(), QStringLiteral("Favorites"));
    QCOMPARE(tag.comment(), QStringLiteral("Favorite presets"));

    KLocalizedString::setLanguages({});
    QLocale::setDefault(QLocale(QStringLiteral("ja_JP")));
    QCOMPARE(KisTag::currentLocale(), QLocale().name());
    QCOMPARE(tag.name(), QStringLiteral("お気に入り"));
    QCOMPARE(tag.comment(), QStringLiteral("お気に入りのプリセット"));
}

void TestTag::copyAssignmentAndCloneOwnIndependentValues()
{
    KisTag source;
    populateTag(source);

    KisTag copy(source);
    KisTag assigned;
    assigned.setName(QStringLiteral("old"));
    assigned = source;
    assigned.operator=(assigned);
    KisTagSP cloned = source.clone();

    QVERIFY(cloned);
    comparePublicValues(copy, source);
    comparePublicValues(assigned, source);
    comparePublicValues(*cloned, source);

    copy.setName(QStringLiteral("Copy"));
    assigned.setComments({{QStringLiteral("de"), QStringLiteral("Zuweisung")}});
    cloned->setUrl(QStringLiteral("cloned"));

    QCOMPARE(source.name(false), QStringLiteral("Favorites"));
    QCOMPARE(source.comments().size(), 2);
    QCOMPARE(source.url(), QStringLiteral("favorites"));
    QCOMPARE(copy.name(false), QStringLiteral("Copy"));
    const QMap<QString, QString> assignedComments{{QStringLiteral("de"), QStringLiteral("Zuweisung")}};
    QCOMPARE(assigned.comments(), assignedComments);
    QCOMPARE(cloned->url(), QStringLiteral("cloned"));
}

void TestTag::saveLoadUtf8AndAutomaticOpen()
{
    KisTag source;
    populateTag(source);
    source.setName(QStringLiteral("お気に入り"));
    source.setNames({});
    source.setComment(QStringLiteral("Préréglages 🖌"));
    source.setComments({});

    OpenRecordingBuffer output;
    QVERIFY(source.save(output));
    QCOMPARE(output.openCallCount, 1);
    QCOMPARE(output.requestedMode, QIODeviceBase::OpenMode(QIODeviceBase::WriteOnly | QIODeviceBase::Text));
    QVERIFY(output.isWritable());

    const QByteArray serialized = output.data();
    QVERIFY(serialized.startsWith("[Desktop Entry]\nType=Tag\n"));
    QVERIFY(serialized.contains("URL=favorites\n"));
    QVERIFY(serialized.contains("ResourceType=paintoppresets\n"));
    QVERIFY(serialized.contains(QStringLiteral("Name=お気に入り\n").toUtf8()));
    QVERIFY(serialized.contains(QStringLiteral("Comment=Préréglages 🖌\n").toUtf8()));
    QVERIFY(serialized.contains(QStringLiteral("Default Resources=ink.kpp,鉛筆.kpp\n").toUtf8()));

    OpenRecordingBuffer input;
    input.setData(serialized);
    KisTag loaded;
    QVERIFY(loaded.load(input));
    QCOMPARE(input.openCallCount, 1);
    QCOMPARE(input.requestedMode, QIODeviceBase::OpenMode(QIODeviceBase::ReadOnly));
    QVERIFY(input.isReadable());
    QVERIFY(loaded.valid());
    QCOMPARE(loaded.id(), -1);
    QVERIFY(loaded.active());
    QCOMPARE(loaded.url(), source.url());
    QCOMPARE(loaded.name(false), source.name(false));
    QCOMPARE(loaded.names(), source.names());
    QCOMPARE(loaded.comment(false), source.comment(false));
    QCOMPARE(loaded.comments(), source.comments());
    QCOMPARE(loaded.resourceType(), source.resourceType());
    QCOMPARE(loaded.defaultResources(), source.defaultResources());
}

void TestTag::localizedValuesAreNotPreservedBySerialization()
{
    KisTag source;
    populateTag(source);

    QBuffer output;
    QVERIFY(source.save(output));
    const QByteArray serialized = output.data();
    QVERIFY(!serialized.contains(QStringLiteral("Name[ja_JP]=お気に入り\n").toUtf8()));
    QVERIFY(!serialized.contains(QStringLiteral("Name[nl]=Favorieten\n").toUtf8()));
    QVERIFY(!serialized.contains(QStringLiteral("Comment[ja_JP]=お気に入りのプリセット\n").toUtf8()));
    QVERIFY(!serialized.contains(QStringLiteral("Comment[nl]=Favoriete voorinstellingen\n").toUtf8()));
    QVERIFY(serialized.contains(QStringLiteral("Name[お気に入り]=\n").toUtf8()));
    QVERIFY(serialized.contains("Name[Favorieten]=\n"));
    QVERIFY(serialized.contains(QStringLiteral("Comment[お気に入りのプリセット]=\n").toUtf8()));
    QVERIFY(serialized.contains("Comment[Favoriete voorinstellingen]=\n"));

    QBuffer input;
    input.setData(serialized);
    KisTag loaded;
    QVERIFY(loaded.load(input));

    const QMap<QString, QString> loadedNames{{QStringLiteral("Favorieten"), QString()},
                                             {QStringLiteral("お気に入り"), QString()}};
    const QMap<QString, QString> loadedComments{{QStringLiteral("Favoriete voorinstellingen"), QString()},
                                                {QStringLiteral("お気に入りのプリセット"), QString()}};
    QCOMPARE(loaded.names(), loadedNames);
    QCOMPARE(loaded.comments(), loadedComments);
    QCOMPARE(loaded.name(false), QStringLiteral("Favorites"));
    QCOMPARE(loaded.comment(false), QStringLiteral("Favorite presets"));
}

void TestTag::incompleteAndInvalidInputReportCurrentResults()
{
    KisTag incomplete;
    incomplete.setFilename(QStringLiteral("incomplete.tag"));
    QBuffer incompleteInput;
    incompleteInput.setData("[Desktop Entry]\nName=Only two lines\n");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*Incomplete tag file 2.*")));
    QVERIFY(!incomplete.load(incompleteInput));
    QVERIFY(!incomplete.valid());

    KisTag wrongHeader;
    wrongHeader.setFilename(QStringLiteral("wrong-header.tag"));
    QBuffer wrongHeaderInput;
    wrongHeaderInput.setData("[Wrong Entry]\nType=Tag\nURL=wrong\nResourceType=brushes\nName=Wrong\nComment=Wrong\n");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*Invalid tag file.*Wrong Entry.*")));
    QVERIFY(!wrongHeader.load(wrongHeaderInput));
    QVERIFY(!wrongHeader.valid());

    KisTag skippedLine;
    QBuffer skippedLineInput;
    skippedLineInput.setData(
        "[Desktop Entry]\nType=Tag\nURL=usable\nResourceType=brushes\nName=Usable\n"
        "Comment=Still usable\nline without equals\nDefault Resources=one.kpp\n");
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("Found invalid line:.*line without equals.*")));
    QVERIFY(skippedLine.load(skippedLineInput));
    QVERIFY(skippedLine.valid());
    QCOMPARE(skippedLine.url(), QStringLiteral("usable"));
    QCOMPARE(skippedLine.defaultResources(), QStringList({QStringLiteral("one.kpp")}));
}

void TestTag::diagnosticStreamingDescribesTagsAndNullPointers()
{
    KisTagSP tag(new KisTag);
    populateTag(*tag);
    tag->setNames({});
    tag->setComments({});

    QString tagDiagnostic;
    {
        QDebug debug(&tagDiagnostic);
        debug << tag;
    }
    QCOMPARE(tagDiagnostic.trimmed(),
             QStringLiteral("[TAG] Name \"Favorites\" Url \"favorites\" Comment \"Favorite presets\" "
                            "Default resources \"ink.kpp, 鉛筆.kpp\""));

    QString nullDiagnostic;
    {
        QDebug debug(&nullDiagnostic);
        debug << KisTagSP();
    }
    QCOMPARE(nullDiagnostic.trimmed(), QStringLiteral("[TAG] NULL"));
}

QTEST_GUILESS_MAIN(TestTag)
