/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <TagActions.h>
#include <TagActionsSource_p.h>

#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

namespace
{
QPointer<QLineEdit> createdLineEdit;
QStringList loadedIconNames;

KisTagSP markerTag(quintptr value)
{
    return KisTagSP(reinterpret_cast<KisTag *>(value), [](KisTag *) {});
}

KoResourceSP markerResource(quintptr value)
{
    return KoResourceSP(reinterpret_cast<KoResource *>(value),
                        [](KoResource *) {});
}

QLineEdit *lineEditFor(LineEditAction *action)
{
    return action->defaultWidget()->findChild<QLineEdit *>();
}

QPushButton *buttonFor(LineEditAction *action)
{
    return action->defaultWidget()->findChild<QPushButton *>();
}

class TestLineEditAction : public LineEditAction
{
public:
    explicit TestLineEditAction(QObject *parent)
        : LineEditAction(parent)
    {
    }

    int triggerCount = 0;
    QString triggeredText;

protected:
    void onTriggered() override
    {
        ++triggerCount;
        triggeredText = userText();
    }
};
}

namespace TagActionsSource
{
QLineEdit *createLineEdit(QWidget *parent)
{
    createdLineEdit = new QLineEdit(parent);
    return createdLineEdit;
}

QIcon loadIcon(const QString &name)
{
    loadedIconNames.append(name);
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::red);
    return QIcon(pixmap);
}
}

QString KisTag::name(bool) const
{
    switch (reinterpret_cast<quintptr>(this)) {
    case 0x10:
        return QStringLiteral("First");
    case 0x20:
        return QStringLiteral("Same URL");
    case 0x30:
        return QStringLiteral("Different");
    default:
        return QStringLiteral("Tag");
    }
}

QString KisTag::url() const
{
    switch (reinterpret_cast<quintptr>(this)) {
    case 0x10:
    case 0x20:
        return QStringLiteral("same");
    case 0x30:
        return QStringLiteral("different");
    default:
        return QStringLiteral("tag");
    }
}

QStringList KisTag::defaultResources() const
{
    return {};
}

QString KisTag::comment(bool) const
{
    return {};
}

QString KoResource::storageLocation() const
{
    return {};
}

bool KoResource::valid() const
{
    return false;
}

QString KoResource::md5Sum(bool) const
{
    return {};
}

int KoResource::version() const
{
    return 0;
}

QString KoResource::filename() const
{
    return {};
}

class TagActionsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void existingTagActionOwnsStateAndEmitsPayload();
    void lineEditActionControlsPresentationAndTriggerPolicy();
    void userInputActionEmitsAndClearsText();
    void resourceActionTracksCurrentResource();
    void tagComparerUsesReferenceUrl();
};

void TagActionsContractTest::existingTagActionOwnsStateAndEmitsPayload()
{
    QObject parent;
    const KisTagSP tag = markerTag(0x10);
    const KoResourceSP resource = markerResource(0x40);
    auto *action = new SimpleExistingTagAction(resource, tag, &parent);
    QPointer<SimpleExistingTagAction> guardedAction = action;
    QSignalSpy triggeredSpy(
        action,
        static_cast<void (SimpleExistingTagAction::*)(KisTagSP, KoResourceSP)>(
            &SimpleExistingTagAction::triggered));

    QCOMPARE(action->parent(), &parent);
    QCOMPARE(action->text(), QStringLiteral("First"));
    action->trigger();
    QCOMPARE(triggeredSpy.count(), 1);
    const QList<QVariant> arguments = triggeredSpy.takeFirst();
    QVERIFY(qvariant_cast<KisTagSP>(arguments.at(0)) == tag);
    QVERIFY(qvariant_cast<KoResourceSP>(arguments.at(1)) == resource);

    delete action;
    QVERIFY(guardedAction.isNull());

    SimpleExistingTagAction emptyAction(resource, KisTagSP(), &parent);
    QSignalSpy emptySpy(
        &emptyAction,
        static_cast<void (SimpleExistingTagAction::*)(KisTagSP, KoResourceSP)>(
            &SimpleExistingTagAction::triggered));
    QVERIFY(emptyAction.text().isEmpty());
    emptyAction.trigger();
    QCOMPARE(emptySpy.count(), 0);
}

void TagActionsContractTest::lineEditActionControlsPresentationAndTriggerPolicy()
{
    QWidget parent;
    auto *action = new TestLineEditAction(&parent);
    QPointer<TestLineEditAction> guardedAction = action;
    QPointer<QWidget> defaultWidget = action->defaultWidget();
    QPointer<QLineEdit> lineEdit = lineEditFor(action);
    QPushButton *button = buttonFor(action);
    QLabel *label = action->defaultWidget()->findChild<QLabel *>();

    QVERIFY(defaultWidget);
    QVERIFY(lineEdit);
    QVERIFY(button);
    QVERIFY(label);
    QCOMPARE(createdLineEdit.data(), lineEdit.data());
    QCOMPARE(lineEdit->parentWidget(), defaultWidget.data());
    QVERIFY(lineEdit->isClearButtonEnabled());
    QVERIFY(!action->closeParentOnTrigger());

    action->setPlaceholderText(QStringLiteral("Type a tag"));
    action->setText(QStringLiteral("alpha"));
    QCOMPARE(lineEdit->placeholderText(), QStringLiteral("Type a tag"));
    QCOMPARE(lineEdit->text(), QStringLiteral("alpha"));

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::blue);
    action->setIcon(QIcon(pixmap));
    QVERIFY(!label->pixmap().isNull());

    action->setVisible(false);
    QVERIFY(!action->isVisible());
    QVERIFY(defaultWidget->isHidden());
    QVERIFY(lineEdit->isHidden());
    action->setVisible(true);
    QVERIFY(action->isVisible());
    QVERIFY(!defaultWidget->isHidden());
    QVERIFY(!lineEdit->isHidden());

    button->click();
    QCOMPARE(action->triggerCount, 1);
    QCOMPARE(action->triggeredText, QStringLiteral("alpha"));
    QCOMPARE(action->data().toString(), QStringLiteral("alpha"));
    QCOMPARE(lineEdit->text(), QStringLiteral("alpha"));

    parent.show();
    action->setCloseParentOnTrigger(true);
    action->setText(QStringLiteral("beta"));
    button->click();
    QCOMPARE(action->triggerCount, 2);
    QCOMPARE(action->triggeredText, QStringLiteral("beta"));
    QVERIFY(parent.isHidden());
    QVERIFY(lineEdit->text().isEmpty());

    delete action;
    QVERIFY(guardedAction.isNull());
    QVERIFY(defaultWidget.isNull());
    QVERIFY(lineEdit.isNull());
}

void TagActionsContractTest::userInputActionEmitsAndClearsText()
{
    loadedIconNames.clear();
    QWidget parent;
    auto *action = new UserInputTagAction(&parent);
    QPointer<UserInputTagAction> guardedAction = action;
    QPointer<QLineEdit> lineEdit = lineEditFor(action);
    QPushButton *button = buttonFor(action);
    QSignalSpy triggeredSpy(
        action,
        static_cast<void (UserInputTagAction::*)(const QString &)>(
            &UserInputTagAction::triggered));

    QVERIFY(lineEdit);
    QVERIFY(button);
    QVERIFY(action->closeParentOnTrigger());
    QVERIFY(!lineEdit->placeholderText().isEmpty());
    QCOMPARE(loadedIconNames,
             QStringList({QStringLiteral("list-add"),
                          QStringLiteral("document-new")}));

    action->setText(QStringLiteral("Landscape"));
    button->click();
    QCOMPARE(triggeredSpy.count(), 1);
    QCOMPARE(triggeredSpy.takeFirst().at(0).toString(),
             QStringLiteral("Landscape"));
    QVERIFY(lineEdit->text().isEmpty());

    delete action;
    QVERIFY(guardedAction.isNull());
    QVERIFY(lineEdit.isNull());
}

void TagActionsContractTest::resourceActionTracksCurrentResource()
{
    loadedIconNames.clear();
    QWidget parent;
    const KoResourceSP initialResource = markerResource(0x50);
    const KoResourceSP replacementResource = markerResource(0x60);
    auto *action = new NewTagResourceAction(initialResource, &parent);
    QPointer<NewTagResourceAction> guardedAction = action;
    QPointer<QLineEdit> lineEdit = lineEditFor(action);
    QPushButton *button = buttonFor(action);
    QSignalSpy triggeredSpy(
        action,
        static_cast<void (NewTagResourceAction::*)(const QString &, KoResourceSP)>(
            &NewTagResourceAction::triggered));

    QVERIFY(lineEdit);
    QVERIFY(button);
    QCOMPARE(loadedIconNames,
             QStringList({QStringLiteral("list-add"),
                          QStringLiteral("document-new")}));

    action->setText(QStringLiteral("First"));
    button->click();
    QCOMPARE(triggeredSpy.count(), 1);
    QList<QVariant> arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QStringLiteral("First"));
    QVERIFY(qvariant_cast<KoResourceSP>(arguments.at(1)) == initialResource);

    action->setResource(replacementResource);
    action->setText(QStringLiteral("Second"));
    button->click();
    QCOMPARE(triggeredSpy.count(), 1);
    arguments = triggeredSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QStringLiteral("Second"));
    QVERIFY(qvariant_cast<KoResourceSP>(arguments.at(1))
            == replacementResource);
    QVERIFY(lineEdit->text().isEmpty());

    delete action;
    QVERIFY(guardedAction.isNull());
    QVERIFY(lineEdit.isNull());
}

void TagActionsContractTest::tagComparerUsesReferenceUrl()
{
    const KisTagSP reference = markerTag(0x10);
    const KisTagSP sameUrl = markerTag(0x20);
    const KisTagSP differentUrl = markerTag(0x30);
    CompareWithOtherTagFunctor comparer(reference);

    QVERIFY(comparer.referenceTag() == reference);
    QVERIFY(comparer(reference));
    QVERIFY(comparer(sameUrl));
    QVERIFY(!comparer(differentUrl));
    QVERIFY(!comparer(KisTagSP()));

    comparer.setReferenceTag(KisTagSP());
    QVERIFY(comparer.referenceTag().isNull());
    QVERIFY(!comparer(reference));

    comparer.setReferenceTag(differentUrl);
    QVERIFY(comparer.referenceTag() == differentUrl);
    QVERIFY(comparer(differentUrl));
    QVERIFY(!comparer(reference));
}

QTEST_MAIN(TagActionsContractTest)

#include "TagActionsContractTest.moc"
