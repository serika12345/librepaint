/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintOpOptionUtils.h"

#include <QTest>

class KisPropertiesConfiguration
{
public:
    int optionValue = 0;
};

namespace
{

struct OptionData {
    const KisPropertiesConfiguration *source = nullptr;
    int value = -1;
    int readCount = 0;

    void read(const KisPropertiesConfiguration *setting)
    {
        source = setting;
        value = setting->optionValue;
        ++readCount;
    }
};

} // namespace

class KisPaintOpOptionUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadOptionDataReadsOnceAndReturnsPopulatedValue();
};

void KisPaintOpOptionUtilsContractTest::loadOptionDataReadsOnceAndReturnsPopulatedValue()
{
    KisPropertiesConfiguration setting;
    setting.optionValue = 37;

    const OptionData data = KisPaintOpOptionUtils::loadOptionData<OptionData>(&setting);

    QCOMPARE(data.source, &setting);
    QCOMPARE(data.value, 37);
    QCOMPARE(data.readCount, 1);
}

QTEST_GUILESS_MAIN(KisPaintOpOptionUtilsContractTest)

#include "KisPaintOpOptionUtilsContractTest.moc"
