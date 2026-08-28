/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceUserOperations.h>
#include <KisResourceUserOperationsNameSource_p.h>

#include <QAbstractButton>
#include <QApplication>
#include <QHash>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QTest>

namespace
{
QHash<QString, QVector<int>> resourceIdsByName;
QStringList queriedNames;
QString capturedQuestionText;
QMessageBox::StandardButtons capturedQuestionButtons;
QMessageBox::StandardButton capturedDefaultButton = QMessageBox::NoButton;
QMessageBox::StandardButton nextAnswer = QMessageBox::Cancel;
bool answerButtonFound = false;

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

QTEST_MAIN(KisResourceUserOperationsContractTest)

#include "KisResourceUserOperationsContractTest.moc"
