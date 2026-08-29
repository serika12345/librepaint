/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_parser.h"
#include "kis_meta_data_value.h"

#include <QTest>
#include <QVariant>

#include <memory>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{

class RecordingParser final : public KisMetaData::Parser
{
public:
    RecordingParser(int &parseCount, int &destructionCount)
        : m_parseCount(parseCount)
        , m_destructionCount(destructionCount)
    {
    }

    ~RecordingParser() override
    {
        ++m_destructionCount;
    }

    KisMetaData::Value parse(const QString &text) const override
    {
        ++m_parseCount;
        return KisMetaData::Value(QVariant(text.size()));
    }

private:
    int &m_parseCount;
    int &m_destructionCount;
};

} // namespace

class KisMetaDataParserContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicBaseDispatchesParsingAndDestruction();
};

void KisMetaDataParserContractTest::publicBaseDispatchesParsingAndDestruction()
{
    int parseCount = 0;
    int destructionCount = 0;
    std::unique_ptr<KisMetaData::Parser> parser = std::make_unique<RecordingParser>(parseCount, destructionCount);

    const KisMetaData::Value result = parser->parse(QStringLiteral("metadata"));
    QCOMPARE(parseCount, 1);
    QCOMPARE(result.type(), KisMetaData::Value::Variant);
    QCOMPARE(result.asVariant().toInt(), 8);

    parser.reset();
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisMetaDataParserContractTest)

#include "KisMetaDataParserContractTest.moc"
