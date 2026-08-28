/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceUserOperations.h>
#include <KisResourceUserOperationsImportSource_p.h>
#include <KisResourceUserOperationsNameSource_p.h>
#include <KisResourceUserOperationsRenameSource_p.h>

#include <QAbstractButton>
#include <QApplication>
#include <QHash>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QTest>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{
QHash<QString, QVector<int>> resourceIdsByName;
QStringList queriedNames;
QString capturedQuestionText;
QMessageBox::StandardButtons capturedQuestionButtons;
QMessageBox::StandardButton capturedDefaultButton = QMessageBox::NoButton;
QMessageBox::StandardButton nextAnswer = QMessageBox::Cancel;
bool answerButtonFound = false;

struct ImportCall
{
    QString resourceType;
    QString resourceFilepath;
    QString storageLocation;
    bool overwrite = false;
};
QList<KisResourceUserOperationsImportSource::ImportAttempt> importAttempts;
QList<ImportCall> importCalls;
int importWarningCount = 0;
QWidget *importWarningParent = nullptr;

bool renameNameUsed = false;
bool renameAllowed = false;
bool renameResult = false;
int renameNameCheckCount = 0;
int renameQuestionCount = 0;
int renameCallCount = 0;
int renameWarningCount = 0;
KoResourceSP capturedRenameResource;
QString capturedRenameName;

KoResourceSP markerResource(quintptr value)
{
    return KoResourceSP(reinterpret_cast<KoResource *>(value), [](KoResource *) {});
}

void answerNextQuestion(QMessageBox::StandardButton answer)
{
    nextAnswer = answer;
    answerButtonFound = false;
    QTimer::singleShot(0, []() {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (!messageBox) {
            return;
        }
        capturedQuestionText = messageBox->text();
        capturedQuestionButtons = messageBox->standardButtons();
        capturedDefaultButton = messageBox->defaultButton()
            ? messageBox->standardButton(messageBox->defaultButton())
            : QMessageBox::NoButton;
        QAbstractButton *button = messageBox->button(nextAnswer);
        answerButtonFound = button != nullptr;
        if (button) {
            button->click();
        } else {
            messageBox->reject();
        }
    });
}
}

namespace KisResourceUserOperationsImportSource
{
ImportAttempt importResourceFile(const QString &resourceType,
                                 const QString &resourceFilepath,
                                 const QString &storageLocation,
                                 bool overwrite)
{
    importCalls.append({resourceType, resourceFilepath, storageLocation, overwrite});
    if (importAttempts.isEmpty()) {
        return {};
    }
    return importAttempts.takeFirst();
}

void warnImportFailed(QWidget *widgetParent)
{
    ++importWarningCount;
    importWarningParent = widgetParent;
}
}

namespace KisResourceUserOperationsRenameSource
{
bool resourceNameIsAlreadyUsed(KoResourceSP resource, const QString &resourceName)
{
    ++renameNameCheckCount;
    capturedRenameResource = resource;
    capturedRenameName = resourceName;
    return renameNameUsed;
}

bool userAllowsDuplicateName(QWidget *)
{
    ++renameQuestionCount;
    return renameAllowed;
}

bool renameResource(KoResourceSP resource, const QString &resourceName)
{
    ++renameCallCount;
    capturedRenameResource = resource;
    capturedRenameName = resourceName;
    return renameResult;
}

void warnRenameFailed(QWidget *)
{
    ++renameWarningCount;
}
}

namespace KisResourceUserOperationsNameSource
{
QVector<int> resourceIdsForName(KisResourceModel *, const QString &name)
{
    queriedNames.append(name);
    return resourceIdsByName.value(name);
}
}

class KisResourceUserOperationsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void overwriteQuestionUsesFileNameAndAnswer();
    void nameUsageChecksExactUnderscoreAndIgnoredId();
    void importUsesOverwriteDecisionAndWarnsOnFailure();
    void renameUsesDuplicateDecisionAndReportsFailure();
};

void KisResourceUserOperationsContractTest::overwriteQuestionUsesFileNameAndAnswer()
{
    capturedQuestionText.clear();
    capturedQuestionButtons = QMessageBox::NoButton;
    capturedDefaultButton = QMessageBox::NoButton;
    answerNextQuestion(QMessageBox::Yes);
    QVERIFY(KisResourceUserOperations::userAllowsOverwrite(
        nullptr, QStringLiteral("/tmp/resources/brush.bundle")));
    QVERIFY(answerButtonFound);
    QVERIFY(capturedQuestionText.contains(QStringLiteral("brush.bundle")));
    QVERIFY(!capturedQuestionText.contains(QStringLiteral("/tmp/resources")));
    QCOMPARE(capturedQuestionButtons,
             QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::Cancel));
    QCOMPARE(capturedDefaultButton, QMessageBox::Cancel);

    answerNextQuestion(QMessageBox::Cancel);
    QVERIFY(!KisResourceUserOperations::userAllowsOverwrite(
        nullptr, QStringLiteral("/tmp/resources/brush.bundle")));
    QVERIFY(answerButtonFound);
}

void KisResourceUserOperationsContractTest::nameUsageChecksExactUnderscoreAndIgnoredId()
{
    auto *model = reinterpret_cast<KisResourceModel *>(quintptr(1));

    resourceIdsByName = {{QStringLiteral("Dry Brush"), {12}}};
    queriedNames.clear();
    QVERIFY(KisResourceUserOperations::resourceNameIsAlreadyUsed(
        model, QStringLiteral("Dry Brush")));
    QCOMPARE(queriedNames, QStringList{QStringLiteral("Dry Brush")});

    resourceIdsByName = {
        {QStringLiteral("Dry Brush"), {12}},
        {QStringLiteral("Dry_Brush"), {15}}
    };
    queriedNames.clear();
    QVERIFY(KisResourceUserOperations::resourceNameIsAlreadyUsed(
        model, QStringLiteral("Dry Brush"), 12));
    QCOMPARE(queriedNames,
             QStringList({QStringLiteral("Dry Brush"), QStringLiteral("Dry_Brush")}));

    resourceIdsByName = {{QStringLiteral("Dry Brush"), {12, 15}}};
    queriedNames.clear();
    QVERIFY(KisResourceUserOperations::resourceNameIsAlreadyUsed(
        model, QStringLiteral("Dry Brush"), 12));

    resourceIdsByName.clear();
    queriedNames.clear();
    QVERIFY(!KisResourceUserOperations::resourceNameIsAlreadyUsed(
        model, QStringLiteral("Unused Name")));
    QCOMPARE(queriedNames,
             QStringList({QStringLiteral("Unused Name"), QStringLiteral("Unused_Name")}));
}

void KisResourceUserOperationsContractTest::importUsesOverwriteDecisionAndWarnsOnFailure()
{
    const KoResourceSP imported = markerResource(0x10);
    importAttempts = {{imported, false}};
    importCalls.clear();
    importWarningCount = 0;
    const KoResourceSP directResult =
        KisResourceUserOperations::importResourceFileWithUserInput(
            nullptr,
            QString(),
            QStringLiteral("brushes"),
            QStringLiteral("/tmp/brush.gbr"));
    QVERIFY(directResult == imported);
    QCOMPARE(importCalls.size(), 1);
    QCOMPARE(importCalls.first().resourceType, QStringLiteral("brushes"));
    QCOMPARE(importCalls.first().resourceFilepath, QStringLiteral("/tmp/brush.gbr"));
    QCOMPARE(importCalls.first().storageLocation, QString());
    QVERIFY(!importCalls.first().overwrite);
    QCOMPARE(importWarningCount, 0);

    importAttempts = {{{}, true}};
    importCalls.clear();
    importWarningCount = 0;
    answerNextQuestion(QMessageBox::Cancel);
    QVERIFY(KisResourceUserOperations::importResourceFileWithUserInput(
                nullptr,
                QString(),
                QStringLiteral("brushes"),
                QStringLiteral("/tmp/brush.gbr"))
                .isNull());
    QVERIFY(answerButtonFound);
    QCOMPARE(importCalls.size(), 1);
    QCOMPARE(importWarningCount, 0);

    importAttempts = {{{}, true}, {imported, false}};
    importCalls.clear();
    answerNextQuestion(QMessageBox::Yes);
    const KoResourceSP overwriteResult =
        KisResourceUserOperations::importResourceFileWithUserInput(
            nullptr,
            QString(),
            QStringLiteral("brushes"),
            QStringLiteral("/tmp/brush.gbr"));
    QVERIFY(overwriteResult == imported);
    QVERIFY(answerButtonFound);
    QCOMPARE(importCalls.size(), 2);
    QVERIFY(!importCalls.at(0).overwrite);
    QVERIFY(importCalls.at(1).overwrite);

    QWidget warningParent;
    importAttempts = {{{}, true}};
    importCalls.clear();
    importWarningCount = 0;
    importWarningParent = nullptr;
    QVERIFY(KisResourceUserOperations::importResourceFileWithUserInput(
                &warningParent,
                QStringLiteral("custom-storage"),
                QStringLiteral("brushes"),
                QStringLiteral("/tmp/missing.gbr"))
                .isNull());
    QCOMPARE(importCalls.size(), 1);
    QCOMPARE(importWarningCount, 1);
    QCOMPARE(importWarningParent, &warningParent);
}

void KisResourceUserOperationsContractTest::renameUsesDuplicateDecisionAndReportsFailure()
{
    const KoResourceSP resource = markerResource(0x20);
    renameNameUsed = true;
    renameAllowed = false;
    renameResult = true;
    renameNameCheckCount = 0;
    renameQuestionCount = 0;
    renameCallCount = 0;
    renameWarningCount = 0;
    QVERIFY(!KisResourceUserOperations::renameResourceWithUserInput(
        nullptr, resource, QStringLiteral("Duplicate")));
    QCOMPARE(renameNameCheckCount, 1);
    QCOMPARE(renameQuestionCount, 1);
    QCOMPARE(renameCallCount, 0);
    QCOMPARE(renameWarningCount, 0);

    renameAllowed = true;
    QVERIFY(KisResourceUserOperations::renameResourceWithUserInput(
        nullptr, resource, QStringLiteral("Duplicate")));
    QCOMPARE(renameCallCount, 1);
    QVERIFY(capturedRenameResource == resource);
    QCOMPARE(capturedRenameName, QStringLiteral("Duplicate"));

    renameNameUsed = false;
    renameResult = false;
    renameNameCheckCount = 0;
    renameQuestionCount = 0;
    renameCallCount = 0;
    renameWarningCount = 0;
    QVERIFY(!KisResourceUserOperations::renameResourceWithUserInput(
        nullptr, resource, QStringLiteral("Unique")));
    QCOMPARE(renameNameCheckCount, 1);
    QCOMPARE(renameQuestionCount, 0);
    QCOMPARE(renameCallCount, 1);
    QCOMPARE(renameWarningCount, 1);
}

QTEST_MAIN(KisResourceUserOperationsContractTest)

#include "KisResourceUserOperationsContractTest.moc"
