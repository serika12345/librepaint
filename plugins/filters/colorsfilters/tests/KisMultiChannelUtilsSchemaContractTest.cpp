/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_multichannel_utils.h"

#include <QTest>

#include <type_traits>

class KisMultiChannelUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void findChannelSignatureRemainsStable();
};

void KisMultiChannelUtilsSchemaContractTest::findChannelSignatureRemainsStable()
{
    using FindChannel = int (*)(const QVector<VirtualChannelInfo> &, const VirtualChannelInfo::Type &);

    static_assert(std::is_same_v<decltype(&KisMultiChannelUtils::findChannel), FindChannel>);
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisMultiChannelUtilsSchemaContractTest)

#include "KisMultiChannelUtilsSchemaContractTest.moc"
