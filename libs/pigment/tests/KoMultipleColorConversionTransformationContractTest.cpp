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
    Step(const KoColorSpace *src, const KoColorSpace *dst, quint8 delta, QString name, quint8 multiplier = 1)
        : KoColorConversionTransformation(src, dst, IntentPerceptual, Empty)
        , m_delta(delta)
        , m_name(std::move(name))
        , m_multiplier(multiplier)
    {
    }
    ~Step() override
    {
        events << "delete-" + m_name;
    }
    void transform(const quint8 *src, quint8 *dst, qint32 pixels) const override
    {
        for (qint32 i = 0; i < pixels; ++i)
            dst[i * width(dstColorSpace())] = quint8(src[i * width(srcColorSpace())] * m_multiplier + m_delta);
    }

private:
    quint8 m_delta;
    QString m_name;
    quint8 m_multiplier;
};
struct Scope {
    Scope()
    {
        widths.clear();
        events.clear();
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
    void ownsAppendedTransformations();
    void twoStageConversionComposesOperationsWithoutChangingInput();
    void threeStageConversionComposesDifferentPixelFormats();
    void distinctBufferTransformInPlacePreservesInput();
};

void KoMultipleColorConversionTransformationContractTest::constructionPreservesConversionIdentity()
{
    using Transformation = KoColorConversionTransformation;

    QCOMPARE(int(Transformation::IntentPerceptual), 0);
    QCOMPARE(int(Transformation::IntentRelativeColorimetric), 1);
    QCOMPARE(int(Transformation::IntentSaturation), 2);
    QCOMPARE(int(Transformation::IntentAbsoluteColorimetric), 3);
    QCOMPARE(int(Transformation::Empty), 0x0);
    QCOMPARE(int(Transformation::NoOptimization), 0x0100);
    QCOMPARE(int(Transformation::GamutCheck), 0x1000);
    QCOMPARE(int(Transformation::SoftProofing), 0x4000);
    QCOMPARE(int(Transformation::BlackpointCompensation), 0x2000);
    QCOMPARE(int(Transformation::NoWhiteOnWhiteFixup), 0x0004);
    QCOMPARE(int(Transformation::HighQuality), 0x0400);
    QCOMPARE(int(Transformation::LowQuality), 0x0800);
    QCOMPARE(int(Transformation::CopyAlpha), 0x04000000);
    QCOMPARE(int(Transformation::NoAdaptationAbsoluteIntent), 01000000);
    QCOMPARE(Transformation::internalRenderingIntent(), Transformation::IntentPerceptual);
    QCOMPARE(Transformation::internalConversionFlags(),
             Transformation::ConversionFlags(Transformation::BlackpointCompensation));
    QCOMPARE(Transformation::adjustmentRenderingIntent(), Transformation::IntentPerceptual);
    QCOMPARE(
        Transformation::adjustmentConversionFlags(),
        Transformation::ConversionFlags(Transformation::BlackpointCompensation | Transformation::NoWhiteOnWhiteFixup));

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
    QVERIFY(t.isValid());
}
void KoMultipleColorConversionTransformationContractTest::ownsAppendedTransformations()
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
    events.sort();
    QCOMPARE(events, QStringList({"delete-first", "delete-second"}));
}
void KoMultipleColorConversionTransformationContractTest::twoStageConversionComposesOperationsWithoutChangingInput()
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
    t.appendTransfo(new Step(space(b), space(c), 3, "second", 2));
    quint8 src[]{1, 2, 3};
    quint8 output[3]{};
    t.transform(src, output, 3);
    QCOMPARE(QList<quint8>({output[0], output[1], output[2]}), QList<quint8>({9, 11, 13}));
    QCOMPARE(QList<quint8>({src[0], src[1], src[2]}), QList<quint8>({1, 2, 3}));
}
void KoMultipleColorConversionTransformationContractTest::threeStageConversionComposesDifferentPixelFormats()
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
    t.appendTransfo(new Step(space(b), space(c), 2, "second", 2));
    t.appendTransfo(new Step(space(c), space(d), 4, "third", 3));
    quint8 src[]{5, 6}, output[2]{};
    t.transform(src, output, 2);
    QCOMPARE(QList<quint8>({output[0], output[1]}), QList<quint8>({46, 52}));
    QCOMPARE(QList<quint8>({src[0], src[1]}), QList<quint8>({5, 6}));
}
void KoMultipleColorConversionTransformationContractTest::distinctBufferTransformInPlacePreservesInput()
{
    Scope scope;
    Token a{}, b{};
    widths[space(a)] = widths[space(b)] = 1;
    Step step(space(a), space(b), 5, "in-place");
    quint8 src[]{1, 4, 9};
    quint8 output[3]{};

    step.transformInPlace(src, output, 3);

    QCOMPARE(QList<quint8>({output[0], output[1], output[2]}), QList<quint8>({6, 9, 14}));
    QCOMPARE(QList<quint8>({src[0], src[1], src[2]}), QList<quint8>({1, 4, 9}));
}
QTEST_GUILESS_MAIN(KoMultipleColorConversionTransformationContractTest)
#include "KoMultipleColorConversionTransformationContractTest.moc"
