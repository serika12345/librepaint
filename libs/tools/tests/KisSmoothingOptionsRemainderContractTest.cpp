/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_smoothing_options.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected recoverable assertion %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

namespace
{

class DerivedSmoothingOptions : public KisSmoothingOptions
{
public:
    explicit DerivedSmoothingOptions(bool *destroyed)
        : KisSmoothingOptions(false)
        , m_destroyed(destroyed)
    {
    }

    ~DerivedSmoothingOptions() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

} // namespace

class KisSmoothingOptionsRemainderContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sharedAliasOwnsObjectLifetime();
    void basePointerDeletionRunsDerivedDestructor();
    void remainingModesRoundTripAndEmitChange();
};

void KisSmoothingOptionsRemainderContractTest::sharedAliasOwnsObjectLifetime()
{
    KisSmoothingOptionsSP options(new KisSmoothingOptions(false));
    QPointer<KisSmoothingOptions> observed(options.data());

    QVERIFY(!observed.isNull());
    options.clear();
    QVERIFY(observed.isNull());
}

void KisSmoothingOptionsRemainderContractTest::basePointerDeletionRunsDerivedDestructor()
{
    bool destroyed = false;
    KisSmoothingOptions *options = new DerivedSmoothingOptions(&destroyed);

    delete options;

    QVERIFY(destroyed);
}

void KisSmoothingOptionsRemainderContractTest::remainingModesRoundTripAndEmitChange()
{
    KisSmoothingOptions options(false);
    QSignalSpy smoothingTypeChangedSpy(&options, &KisSmoothingOptions::sigSmoothingTypeChanged);
    const QList<KisSmoothingOptions::SmoothingType> modes{
        KisSmoothingOptions::NO_SMOOTHING,
        KisSmoothingOptions::STABILIZER,
        KisSmoothingOptions::PIXEL_PERFECT,
    };

    QVERIFY(smoothingTypeChangedSpy.isValid());
    for (const KisSmoothingOptions::SmoothingType mode : modes) {
        const int previousSignalCount = smoothingTypeChangedSpy.count();

        options.setSmoothingType(mode);

        QCOMPARE(options.smoothingType(), mode);
        QCOMPARE(smoothingTypeChangedSpy.count(), previousSignalCount + 1);
    }
}

QTEST_GUILESS_MAIN(KisSmoothingOptionsRemainderContractTest)

#include "KisSmoothingOptionsRemainderContractTest.moc"
