/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opengl/KisOpenGLStartupCheck.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

// L2 condition 2 (docs/perf/l2-renderer-choice.md): a renderer whose first
// frame check fails leaves a mark in kritadisplayrc, and the next launch
// skips it and takes the next candidate. The mark disappears when the
// renderer succeeds again. The test forces the failure with the test-only
// environment variable, so it does not need a broken graphics driver.
class KisOpenGLStartupCheckContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        qunsetenv("LIBREPAINT_TEST_FAIL_OPENGL_FIRST_FRAME");
    }

    void forcedFailureWritesTheMarkAndSkipsTheRenderer()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        qputenv("LIBREPAINT_TEST_FAIL_OPENGL_FIRST_FRAME", "1");
        QVERIFY(KisOpenGLStartupCheck::isForcedFailureForTesting());

        QString failureReason;
        QVERIFY(!KisOpenGLStartupCheck::verifyFirstFrameOnCurrentContext(
            QStringLiteral("desktop"), &failureReason, configFilePath));
        QVERIFY2(failureReason.contains(QStringLiteral("LIBREPAINT_TEST_FAIL_OPENGL_FIRST_FRAME")),
                 qPrintable(failureReason));

        QCOMPARE(KisOpenGLStartupCheck::failedRendererNames(configFilePath),
                 QStringList{QStringLiteral("desktop")});

        // The next launch asks for the same candidates and gets the next one.
        const QStringList next =
            KisOpenGLStartupCheck::renderersToTry({QStringLiteral("desktop"),
                                                   QStringLiteral("angle"),
                                                   QStringLiteral("software")},
                                                  true,
                                                  configFilePath);
        QCOMPARE(next, (QStringList{QStringLiteral("angle"), QStringLiteral("software")}));
    }

    void missingContextRecordsTheFailure()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        QString failureReason;
        QVERIFY(!KisOpenGLStartupCheck::verifyFirstFrameOnCurrentContext(
            QStringLiteral("desktop"), &failureReason, configFilePath));
        QVERIFY(!failureReason.isEmpty());
        QVERIFY(KisOpenGLStartupCheck::failedRendererNames(configFilePath)
                    .contains(QStringLiteral("desktop")));
    }

    void aSuccessfulCheckClearsOnlyItsOwnMark()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("desktop"), configFilePath);
        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("angle"), configFilePath);

        KisOpenGLStartupCheck::clearFailedRenderer(QStringLiteral("angle"), configFilePath);

        QCOMPARE(KisOpenGLStartupCheck::failedRendererNames(configFilePath),
                 QStringList{QStringLiteral("desktop")});
    }

    void anExplicitUserChoiceIgnoresTheMarks()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("desktop"), configFilePath);

        // kritadisplayrc OpenGLRenderer=desktop, or the L1 renderer switch,
        // must still be able to try the marked renderer.
        QCOMPARE(KisOpenGLStartupCheck::renderersToTry(
                     {QStringLiteral("desktop"), QStringLiteral("angle")},
                     false,
                     configFilePath),
                 (QStringList{QStringLiteral("desktop"), QStringLiteral("angle")}));
    }

    void autoInTheConfigStillFollowsTheMarks()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("desktop"), configFilePath);

        // The Surface stores OpenGLRenderer=auto. That is not an explicit
        // choice, so the marked renderer stays out of the candidates.
        const bool userForced = KisOpenGLStartupCheck::isUserForcedRenderer(QStringLiteral("auto"), true, QString());
        QVERIFY(!userForced);
        QCOMPARE(KisOpenGLStartupCheck::renderersToTry({QStringLiteral("desktop"), QStringLiteral("angle")},
                                                       !userForced,
                                                       configFilePath),
                 QStringList{QStringLiteral("angle")});
    }

    void anExplicitRendererInTheConfigIgnoresTheMarks()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("desktop"), configFilePath);

        // Explicit angle is the way back even while the desktop mark remains.
        const bool userForced = KisOpenGLStartupCheck::isUserForcedRenderer(QStringLiteral("angle"), true, QString());
        QVERIFY(userForced);
        QCOMPARE(KisOpenGLStartupCheck::renderersToTry({QStringLiteral("desktop"), QStringLiteral("angle")},
                                                       !userForced,
                                                       configFilePath),
                 (QStringList{QStringLiteral("desktop"), QStringLiteral("angle")}));
    }

    void onlyNamedRenderersAreExplicit()
    {
        for (const QString &name :
             {QStringLiteral("desktop"), QStringLiteral("angle"), QStringLiteral("software"), QStringLiteral("none")}) {
            QVERIFY2(KisOpenGLStartupCheck::isExplicitRendererName(name), qPrintable(name));
            QVERIFY2(KisOpenGLStartupCheck::isUserForcedRenderer(QString(), false, name), qPrintable(name));
        }

        for (const QString &name : {QStringLiteral("auto"),
                                    QStringLiteral("Auto"),
                                    QStringLiteral(" angle "),
                                    QStringLiteral("vulkan"),
                                    QString()}) {
            QVERIFY2(!KisOpenGLStartupCheck::isExplicitRendererName(name), qPrintable(name));
        }

        // An absent kritadisplayrc key keeps the platform default (angle on
        // Windows) but must not count as an explicit choice. The L1 switch
        // also treats "auto" like an unset value.
        QVERIFY(!KisOpenGLStartupCheck::isUserForcedRenderer(QStringLiteral("angle"), false, QString()));
        QVERIFY(!KisOpenGLStartupCheck::isUserForcedRenderer(QStringLiteral("auto"), true, QStringLiteral("auto")));
        QVERIFY(KisOpenGLStartupCheck::isUserForcedRenderer(QStringLiteral("auto"), true, QStringLiteral("desktop")));
    }

    void allMarkedCandidatesAreTriedAgain()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("desktop"), configFilePath);
        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("angle"), configFilePath);
        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("software"), configFilePath);

        // Trying them again is the only way a driver update can restore
        // OpenGL. QPainter stays the last resort when they all fail again.
        QCOMPARE(KisOpenGLStartupCheck::renderersToTry(
                     {QStringLiteral("desktop"),
                      QStringLiteral("angle"),
                      QStringLiteral("software")},
                     true,
                     configFilePath),
                 (QStringList{QStringLiteral("desktop"),
                              QStringLiteral("angle"),
                              QStringLiteral("software")}));
    }

    void unknownNamesAreIgnored()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral("vulkan"), configFilePath);
        KisOpenGLStartupCheck::recordFailedRenderer(QStringLiteral(" desktop "), configFilePath);

        QCOMPARE(KisOpenGLStartupCheck::failedRendererNames(configFilePath),
                 QStringList{QStringLiteral("desktop")});
    }

    void aMissingConfigFileHasNoMarks()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString configFilePath = dir.path() + QStringLiteral("/kritadisplayrc");

        QVERIFY(KisOpenGLStartupCheck::failedRendererNames(configFilePath).isEmpty());
        QCOMPARE(KisOpenGLStartupCheck::renderersToTry(
                     {QStringLiteral("desktop"), QStringLiteral("angle")},
                     true,
                     configFilePath),
                 (QStringList{QStringLiteral("desktop"), QStringLiteral("angle")}));
    }

    void selectionLineNamesTheChoiceAndTheSkippedRenderers()
    {
        // A measurement launch does not show the warning UI and initializes
        // the usage logger after the choice, so this one line is what app.err
        // and the latency log keep for the audit.
        QCOMPARE(KisOpenGLStartupCheck::rendererSelectionLogLine(
                     QStringLiteral("desktop"),
                     {QStringLiteral("angle"), QStringLiteral("desktop"), QStringLiteral("software")},
                     {QStringLiteral("angle")}),
                 QStringLiteral("OpenGL renderer selection: chosen=desktop "
                                "candidates=angle,desktop,software skipped=angle"));
        QCOMPARE(KisOpenGLStartupCheck::rendererSelectionLogLine(
                     QStringLiteral("angle"),
                     {QStringLiteral("angle")},
                     {}),
                 QStringLiteral("OpenGL renderer selection: chosen=angle "
                                "candidates=angle skipped=none"));
        QCOMPARE(KisOpenGLStartupCheck::rendererSelectionLogLine(QString(), {}, {}),
                 QStringLiteral("OpenGL renderer selection: chosen=none "
                                "candidates=none skipped=none"));
    }

    void firstFrameLineKeepsTheResultAndTheReason()
    {
        QCOMPARE(KisOpenGLStartupCheck::firstFrameCheckLogLine(
                     QStringLiteral("desktop"), true, QString()),
                 QStringLiteral("OpenGL first frame check: renderer=desktop result=passed"));
        QCOMPARE(KisOpenGLStartupCheck::firstFrameCheckLogLine(
                     QStringLiteral("angle"), false, QStringLiteral("shader did not compile")),
                 QStringLiteral("OpenGL first frame check: renderer=angle result=failed "
                                "reason=shader did not compile"));
    }
};

QTEST_GUILESS_MAIN(KisOpenGLStartupCheckContractTest)

#include "KisOpenGLStartupCheckContractTest.moc"
