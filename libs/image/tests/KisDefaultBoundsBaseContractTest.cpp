/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_default_bounds_base.h"

#include <QTest>

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
        return QRect(-11, 13, 89, 97);
    }

    bool wrapAroundMode() const override
    {
        return true;
    }

    WrapAroundAxis wrapAroundModeAxis() const override
    {
        return WRAPAROUND_VERTICAL;
    }

    int currentLevelOfDetail() const override
    {
        return 3;
    }

    int currentTime() const override
    {
        return 37;
    }

    bool externalFrameActive() const override
    {
        return true;
    }

    void *sourceCookie() const override
    {
        return m_cookie;
    }

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
    void imageBorderUsesProvidedBounds();
};

void KisDefaultBoundsBaseContractTest::sharedBaseOwnershipDestroysDerivedOnce()
{
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

void KisDefaultBoundsBaseContractTest::imageBorderUsesProvidedBounds()
{
    DefaultBoundsProbe probe;
    KisDefaultBoundsBase *bounds = &probe;

    QCOMPARE(bounds->imageBorderRect(), QRect(-11, 13, 89, 97));
}

QTEST_GUILESS_MAIN(KisDefaultBoundsBaseContractTest)

#include "KisDefaultBoundsBaseContractTest.moc"
