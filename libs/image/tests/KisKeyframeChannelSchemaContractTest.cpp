/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_keyframe_channel.h"
#include "kis_scalar_keyframe_channel.h"

#include <QTest>

#include <utility>

namespace KisKeyframeChannelContractTypes
{

class KeyframeChannelConstructionProbe final : public KisKeyframeChannel
{
public:
    using KisKeyframeChannel::KisKeyframeChannel;

    explicit KeyframeChannelConstructionProbe(const KisKeyframeChannel &rhs)
        : KisKeyframeChannel(rhs)
    {
    }

    QRect affectedRect(int) const override;

private:
    KisKeyframeSP createKeyframe() override;
    QPair<int, KisKeyframeSP> loadKeyframe(const QDomElement &) override;
    void saveKeyframe(KisKeyframeSP, QDomElement, const QString &) override;
};

} // namespace KisKeyframeChannelContractTypes

class KisKeyframeChannelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scalarKeyframeLimitsValueSchemaRemainsStable();
};

void KisKeyframeChannelSchemaContractTest::scalarKeyframeLimitsValueSchemaRemainsStable()
{

    ScalarKeyframeLimits limits(5.0, 1.0);
    QCOMPARE(limits.lower, 1.0);
    QCOMPARE(limits.upper, 5.0);
    QCOMPARE(limits.clamp(-1.0), 1.0);
    QCOMPARE(limits.clamp(3.0), 3.0);
    QCOMPARE(limits.clamp(8.0), 5.0);
}

QTEST_GUILESS_MAIN(KisKeyframeChannelSchemaContractTest)

#include "KisKeyframeChannelSchemaContractTest.moc"
