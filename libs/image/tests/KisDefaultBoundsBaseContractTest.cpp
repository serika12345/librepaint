/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_default_bounds_base.h"

#include <QTest>

#include <type_traits>

namespace
{

class DefaultBoundsProbe final : public KisDefaultBoundsBase
{
public:
    explicit DefaultBoundsProbe(void *cookie = nullptr, int *destructionCount = nullptr)
        : m_cookie(cookie)
        , m_destructionCount(destructionCount)
    {
    }

    ~DefaultBoundsProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QRect bounds() const override
    {
        ++boundsCalls;
        return QRect(-11, 13, 89, 97);
    }

    bool wrapAroundMode() const override
    {
        ++wrapAroundModeCalls;
        return true;
    }

    WrapAroundAxis wrapAroundModeAxis() const override
    {
        ++wrapAroundAxisCalls;
        return WRAPAROUND_VERTICAL;
    }

    int currentLevelOfDetail() const override
    {
        ++levelOfDetailCalls;
        return 3;
    }

    int currentTime() const override
    {
        ++currentTimeCalls;
        return 37;
    }

    bool externalFrameActive() const override
    {
        ++externalFrameCalls;
        return true;
    }

    void *sourceCookie() const override
    {
        ++sourceCookieCalls;
        return m_cookie;
    }

    mutable int boundsCalls{0};
    mutable int wrapAroundModeCalls{0};
    mutable int wrapAroundAxisCalls{0};
    mutable int levelOfDetailCalls{0};
    mutable int currentTimeCalls{0};
    mutable int externalFrameCalls{0};
    mutable int sourceCookieCalls{0};

private:
    void *m_cookie{nullptr};
    int *m_destructionCount{nullptr};
};

} // namespace

class KisDefaultBoundsBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sharedBaseOwnershipDestroysDerivedOnce();
    void imageBorderDelegatesToBoundsOnce();
    void dispatchesWrapTimeAndSourceState();
};

void KisDefaultBoundsBaseContractTest::sharedBaseOwnershipDestroysDerivedOnce()
{
    static_assert(std::is_same_v<KisDefaultBoundsBaseSP, KisSharedPtr<KisDefaultBoundsBase>>);

    int destructionCount = 0;
    {
        KisDefaultBoundsBaseSP first = new DefaultBoundsProbe(nullptr, &destructionCount);
        QCOMPARE(destructionCount, 0);

        {
            const KisDefaultBoundsBaseSP second = first;
            QCOMPARE(second.data(), first.data());
            QCOMPARE(destructionCount, 0);
        }

        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KisDefaultBoundsBaseContractTest::imageBorderDelegatesToBoundsOnce()
{
    DefaultBoundsProbe probe;
    KisDefaultBoundsBase *bounds = &probe;

    QCOMPARE(bounds->imageBorderRect(), QRect(-11, 13, 89, 97));
    QCOMPARE(probe.boundsCalls, 1);
}

void KisDefaultBoundsBaseContractTest::dispatchesWrapTimeAndSourceState()
{
    int source = 0;
    DefaultBoundsProbe probe(&source);
    const KisDefaultBoundsBase *bounds = &probe;

    QVERIFY(bounds->wrapAroundMode());
    QCOMPARE(probe.wrapAroundModeCalls, 1);
    QCOMPARE(bounds->wrapAroundModeAxis(), WRAPAROUND_VERTICAL);
    QCOMPARE(probe.wrapAroundAxisCalls, 1);
    QCOMPARE(bounds->currentLevelOfDetail(), 3);
    QCOMPARE(probe.levelOfDetailCalls, 1);
    QCOMPARE(bounds->currentTime(), 37);
    QCOMPARE(probe.currentTimeCalls, 1);
    QVERIFY(bounds->externalFrameActive());
    QCOMPARE(probe.externalFrameCalls, 1);
    QCOMPARE(bounds->sourceCookie(), static_cast<void *>(&source));
    QCOMPARE(probe.sourceCookieCalls, 1);
}

QTEST_GUILESS_MAIN(KisDefaultBoundsBaseContractTest)

#include "KisDefaultBoundsBaseContractTest.moc"
