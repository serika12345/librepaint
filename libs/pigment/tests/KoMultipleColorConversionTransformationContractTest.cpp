/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <KoMultipleColorConversionTransformation.h>
#include <QHash>
#include <QTest>

KoColorTransformation::~KoColorTransformation() = default;
QList<QString> KoColorTransformation::parameters() const
{
    return {};
}
int KoColorTransformation::parameterId(const QString &) const
{
    qFatal("unused parameter lookup");
    return -1;
}
void KoColorTransformation::setParameter(int, const QVariant &)
{
    qFatal("unused parameter setter");
}
void KoColorTransformation::setParameters(const QHash<QString, QVariant> &)
{
    qFatal("unused parameter batch setter");
}

namespace KoMultipleColorConversionTransformationTesting
{
void setPixelSizeReader(quint32 (*)(const KoColorSpace *));
void resetPixelSizeReader();
} // namespace KoMultipleColorConversionTransformationTesting

namespace
{
QHash<const KoColorSpace *, quint32> widths;
QStringList events;
struct Call {
    const quint8 *src;
    quint8 *dst;
    qint32 pixels;
};
QList<Call> calls;
struct Token {
    int value;
};
const KoColorSpace *space(Token &token)
{
    return reinterpret_cast<const KoColorSpace *>(&token);
}
quint32 width(const KoColorSpace *space)
{
    return widths.value(space);
}

class Step final : public KoColorConversionTransformation
{
public:
    Step(const KoColorSpace *src, const KoColorSpace *dst, quint8 delta, QString name)
        : KoColorConversionTransformation(src, dst, IntentPerceptual, Empty)
        , m_delta(delta)
        , m_name(std::move(name))
    {
    }
    ~Step() override
    {
        events << "delete-" + m_name;
    }
    void transform(const quint8 *src, quint8 *dst, qint32 pixels) const override
    {
        events << m_name;
        calls << Call{src, dst, pixels};
        for (qint32 i = 0; i < pixels; ++i)
            dst[i] = quint8(src[i] + m_delta);
    }

private:
    quint8 m_delta;
    QString m_name;
};
struct Scope {
    Scope()
    {
        widths.clear();
        events.clear();
        calls.clear();
        KoMultipleColorConversionTransformationTesting::setPixelSizeReader(width);
    }
    ~Scope()
    {
        KoMultipleColorConversionTransformationTesting::resetPixelSizeReader();
    }
};
} // namespace

class KoMultipleColorConversionTransformationContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void constructionPreservesConversionIdentity();
    void appendOwnsTransformationsInInputOrder();
    void twoStageConversionUsesOneDistinctIntermediateBuffer();
    void threeStageConversionAlternatesDistinctIntermediateBuffers();
};

void KoMultipleColorConversionTransformationContractTest::constructionPreservesConversionIdentity()
{
    Scope scope;
    Token a{}, b{};
    widths[space(a)] = 1;
    widths[space(b)] = 2;
    KoMultipleColorConversionTransformation t(space(a),
                                              space(b),
                                              KoColorConversionTransformation::IntentSaturation,
                                              KoColorConversionTransformation::GamutCheck
                                                  | KoColorConversionTransformation::HighQuality);
    QCOMPARE(t.srcColorSpace(), space(a));
    QCOMPARE(t.dstColorSpace(), space(b));
    QCOMPARE(t.renderingIntent(), KoColorConversionTransformation::IntentSaturation);
    QCOMPARE(t.conversionFlags(),
             KoColorConversionTransformation::ConversionFlags(KoColorConversionTransformation::GamutCheck
                                                              | KoColorConversionTransformation::HighQuality));
}
void KoMultipleColorConversionTransformationContractTest::appendOwnsTransformationsInInputOrder()
{
    Scope scope;
    Token a{}, b{}, c{};
    widths[space(a)] = widths[space(b)] = widths[space(c)] = 1;
    {
        KoMultipleColorConversionTransformation t(space(a),
                                                  space(c),
                                                  KoColorConversionTransformation::IntentPerceptual,
                                                  {});
        t.appendTransfo(new Step(space(a), space(b), 1, "first"));
        t.appendTransfo(new Step(space(b), space(c), 1, "second"));
    }
    QCOMPARE(events, QStringList({"delete-first", "delete-second"}));
}
void KoMultipleColorConversionTransformationContractTest::twoStageConversionUsesOneDistinctIntermediateBuffer()
{
    Scope scope;
    Token a{}, b{}, c{};
    widths[space(a)] = 1;
    widths[space(b)] = 4;
    widths[space(c)] = 1;
    KoMultipleColorConversionTransformation t(space(a),
                                              space(c),
                                              KoColorConversionTransformation::IntentPerceptual,
                                              {});
    t.appendTransfo(new Step(space(a), space(b), 2, "first"));
    t.appendTransfo(new Step(space(b), space(c), 3, "second"));
    quint8 src[]{1, 2, 3};
    quint8 output[3]{};
    t.transform(src, output, 3);
    QCOMPARE(QList<quint8>({output[0], output[1], output[2]}), QList<quint8>({6, 7, 8}));
    QCOMPARE(events, QStringList({"first", "second"}));
    QCOMPARE(calls[0].pixels, 3);
    QCOMPARE(calls[1].pixels, 3);
    QVERIFY(calls[0].dst != src);
    QCOMPARE(calls[1].src, calls[0].dst);
    QCOMPARE(calls[1].dst, output);
}
void KoMultipleColorConversionTransformationContractTest::threeStageConversionAlternatesDistinctIntermediateBuffers()
{
    Scope scope;
    Token a{}, b{}, c{}, d{};
    widths[space(a)] = 1;
    widths[space(b)] = 2;
    widths[space(c)] = 3;
    widths[space(d)] = 1;
    KoMultipleColorConversionTransformation t(space(a),
                                              space(d),
                                              KoColorConversionTransformation::IntentPerceptual,
                                              {});
    t.appendTransfo(new Step(space(a), space(b), 1, "first"));
    t.appendTransfo(new Step(space(b), space(c), 2, "second"));
    t.appendTransfo(new Step(space(c), space(d), 4, "third"));
    quint8 src[]{5, 6}, output[2]{};
    t.transform(src, output, 2);
    QCOMPARE(QList<quint8>({output[0], output[1]}), QList<quint8>({12, 13}));
    QCOMPARE(events, QStringList({"first", "second", "third"}));
    QVERIFY(calls[0].dst != calls[1].dst);
    QCOMPARE(calls[1].src, calls[0].dst);
    QCOMPARE(calls[2].src, calls[1].dst);
    QCOMPARE(calls[2].dst, output);
}
QTEST_GUILESS_MAIN(KoMultipleColorConversionTransformationContractTest)
#include "KoMultipleColorConversionTransformationContractTest.moc"
