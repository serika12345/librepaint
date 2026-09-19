/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include "KisImageAnimSettingCommand.h"
#include "KisImageAnimSettingCommandAnimationAccess_p.h"
#include "kis_command_ids.h"
#include "kis_time_span.h"

namespace
{
struct AnimationToken {
    int fps = 0;
    KisTimeSpan range;
};

AnimationToken *token(KisImageAnimationInterface *interface)
{
    return reinterpret_cast<AnimationToken *>(interface);
}

const AnimationToken *token(const KisImageAnimationInterface *interface)
{
    return reinterpret_cast<const AnimationToken *>(interface);
}

KisImageAnimationInterface *asInterface(AnimationToken *value)
{
    return reinterpret_cast<KisImageAnimationInterface *>(value);
}

class OtherCommand : public KUndo2Command
{
};
} // namespace

KisAsynchronouslyMergeableCommandInterface::~KisAsynchronouslyMergeableCommandInterface() = default;

void kis_assert_exception(const char *, const char *, int)
{
    qFatal("unexpected assertion");
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("unexpected recoverable assertion");
}

int kisImageAnimSettingCommandFramerate(const KisImageAnimationInterface *interface)
{
    const AnimationToken *value = token(interface);
    return value->fps;
}

KisTimeSpan kisImageAnimSettingCommandDocumentRange(const KisImageAnimationInterface *interface)
{
    const AnimationToken *value = token(interface);
    return value->range;
}

void kisImageAnimSettingCommandSetFramerate(KisImageAnimationInterface *interface, int fps)
{
    AnimationToken *value = token(interface);
    value->fps = fps;
}

void kisImageAnimSettingCommandSetDocumentRange(KisImageAnimationInterface *interface, const KisTimeSpan &range)
{
    AnimationToken *value = token(interface);
    value->range = range;
}

class KisImageAnimSettingCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void settingsDefaults();
    void constructionCapturesBeforeValuesAndParent();
    void redoAndUndoRestoreAnimationSettings();
    void idAndMergeCompatibilityAreStable();
    void mergeRetainsFirstBeforeAndUsesLastAfter();
};

void KisImageAnimSettingCommandContractTest::settingsDefaults()
{
    KisImageAnimSettingCommand::Settings settings;
    QCOMPARE(settings.FPS, 0);
    QCOMPARE(settings.startFrame, 0);
    QCOMPARE(settings.endFrame, 24);

    settings = {30, -4, 92};
    QCOMPARE(settings.FPS, 30);
    QCOMPARE(settings.startFrame, -4);
    QCOMPARE(settings.endFrame, 92);
}

void KisImageAnimSettingCommandContractTest::constructionCapturesBeforeValuesAndParent()
{
    AnimationToken animation{12, KisTimeSpan::fromTimeToTime(3, 27)};
    KUndo2Command parent;
    auto *command = new KisImageAnimSettingCommand(asInterface(&animation), {24, 7, 41}, &parent);

    QCOMPARE(parent.childCount(), 1);
    QCOMPARE(parent.child(0), command);

    animation.fps = 60;
    animation.range = KisTimeSpan::fromTimeToTime(100, 120);
    command->redo();
    QCOMPARE(animation.fps, 24);
    QCOMPARE(animation.range.start(), 7);
    QCOMPARE(animation.range.end(), 41);
}

void KisImageAnimSettingCommandContractTest::redoAndUndoRestoreAnimationSettings()
{
    AnimationToken animation{12, KisTimeSpan::fromTimeToTime(3, 27)};
    KisImageAnimSettingCommand command(asInterface(&animation), {24, 7, 41});

    command.redo();
    QCOMPARE(animation.fps, 24);
    QCOMPARE(animation.range.start(), 7);
    QCOMPARE(animation.range.end(), 41);
    command.undo();
    QCOMPARE(animation.fps, 12);
    QCOMPARE(animation.range.start(), 3);
    QCOMPARE(animation.range.end(), 27);
}

void KisImageAnimSettingCommandContractTest::idAndMergeCompatibilityAreStable()
{
    AnimationToken animation{12, KisTimeSpan::fromTimeToTime(3, 27)};
    KisImageAnimSettingCommand first(asInterface(&animation), {24, 7, 41});
    KisImageAnimSettingCommand second(asInterface(&animation), {48, 9, 50});
    OtherCommand other;

    QCOMPARE(first.id(), int(KisCommandUtils::ImageAnimSettingCommandId));
    QVERIFY(first.canMergeWith(&second));
    QVERIFY(!first.canMergeWith(&other));
    QVERIFY(!first.mergeWith(&other));
}

void KisImageAnimSettingCommandContractTest::mergeRetainsFirstBeforeAndUsesLastAfter()
{
    AnimationToken animation{12, KisTimeSpan::fromTimeToTime(3, 27)};
    KisImageAnimSettingCommand first(asInterface(&animation), {24, 7, 41});
    KisImageAnimSettingCommand second(asInterface(&animation), {48, 9, 50});

    QVERIFY(first.mergeWith(&second));
    first.redo();
    QCOMPARE(animation.fps, 48);
    QCOMPARE(animation.range.start(), 9);
    QCOMPARE(animation.range.end(), 50);
    first.undo();
    QCOMPARE(animation.fps, 12);
    QCOMPARE(animation.range.start(), 3);
    QCOMPARE(animation.range.end(), 27);
}

QTEST_MAIN(KisImageAnimSettingCommandContractTest)

#include "KisImageAnimSettingCommandContractTest.moc"
