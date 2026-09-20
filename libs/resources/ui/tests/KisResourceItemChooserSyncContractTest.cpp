/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemChooserSync.h>

#include <QSignalSpy>
#include <QTest>

class KisResourceItemChooserSyncContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsAtDefaultBaseLength();
    void clampsAndEmitsEffectiveBaseLength();
};

void KisResourceItemChooserSyncContractTest::startsAtDefaultBaseLength()
{
    // Consumer: Resource chooser users opening a synchronized thumbnail grid.
    // Operation: A chooser reads the shared preview size before the user changes it.
    // Observable result: The initial preview cells use the standard 50-pixel base length.
    // Failure impact: Newly opened resource choosers have unexpectedly tiny or oversized previews.
    KisResourceItemChooserSync sync;

    QCOMPARE(sync.baseLength(), 50);
}

void KisResourceItemChooserSyncContractTest::clampsAndEmitsEffectiveBaseLength()
{
    // Consumer: Preset, brush, and gamut-mask chooser users resizing synchronized previews.
    // Operation: A chooser requests a preview size below, within, and above the supported range.
    // Observable result: Every synchronized chooser receives the usable clamped size.
    // Failure impact: Preview grids diverge or become too small or large to select resources reliably.
    KisResourceItemChooserSync sync;
    QSignalSpy changedSpy(&sync, &KisResourceItemChooserSync::baseLengthChanged);
    const QList<int> requestedLengths {10, 70, 120};
    const QList<int> effectiveLengths {25, 70, 100};

    for (int i = 0; i < requestedLengths.size(); ++i) {
        sync.setBaseLength(requestedLengths.at(i));

        QCOMPARE(sync.baseLength(), effectiveLengths.at(i));
        QCOMPARE(changedSpy.size(), i + 1);
        QCOMPARE(changedSpy.at(i).at(0).toInt(), effectiveLengths.at(i));
    }
}

QTEST_GUILESS_MAIN(KisResourceItemChooserSyncContractTest)

#include "KisResourceItemChooserSyncContractTest.moc"
