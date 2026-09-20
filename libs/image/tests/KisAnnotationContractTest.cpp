/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_annotation.h"

#include <QTest>

#include <memory>

namespace
{

struct AnnotationCalls {
    int clone{0};
    int displayText{0};
    int destruction{0};
};

class RoutingAnnotation final : public KisAnnotation
{
public:
    explicit RoutingAnnotation(AnnotationCalls *calls)
        : KisAnnotation(QStringLiteral("routing/type"),
                        QStringLiteral("Routing annotation"),
                        QByteArrayLiteral("payload"))
        , m_calls(calls)
    {
    }

    ~RoutingAnnotation() override
    {
        ++m_calls->destruction;
    }

    KisAnnotation *clone() const override
    {
        ++m_calls->clone;
        return new RoutingAnnotation(*this);
    }

    QString displayText() const override
    {
        ++m_calls->displayText;
        return QStringLiteral("routed: ") + type();
    }

private:
    RoutingAnnotation(const RoutingAnnotation &rhs)
        : KisAnnotation(rhs)
        , m_calls(rhs.m_calls)
    {
    }

    AnnotationCalls *m_calls;
};

} // namespace

class KisAnnotationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesTypeDescriptionAndBinaryPayload();
    void setAnnotationReplacesOnlyPayload();
    void displayTextDecodesUtf8Payload();
    void cloneCreatesIndependentHeapObject();
    void virtualInterfaceDispatchesAndDestroysDerivedObject();
};

void KisAnnotationContractTest::storesTypeDescriptionAndBinaryPayload()
{
    const QString type = QStringLiteral("application/x-librepaint-contract");
    const QString description = QStringLiteral("Binary contract data");
    const QByteArray payload("head\0tail\xff", 10);

    const KisAnnotation annotation(type, description, payload);

    QCOMPARE(annotation.type(), type);
    QCOMPARE(annotation.description(), description);
    QCOMPARE(annotation.annotation(), payload);
    QCOMPARE(annotation.annotation().size(), 10);
    QCOMPARE(annotation.annotation().at(4), '\0');
}

void KisAnnotationContractTest::setAnnotationReplacesOnlyPayload()
{
    const QString type = QStringLiteral("contract/type");
    const QString description = QStringLiteral("Stable description");
    KisAnnotation annotation(type, description, QByteArrayLiteral("before"));
    const QByteArray replacement("after\0binary", 12);

    annotation.setAnnotation(replacement);

    QCOMPARE(annotation.annotation(), replacement);
    QCOMPARE(annotation.type(), type);
    QCOMPARE(annotation.description(), description);
}

void KisAnnotationContractTest::displayTextDecodesUtf8Payload()
{
    const QString expected = QStringLiteral("Résumé — 日本語");
    const KisAnnotation annotation(QStringLiteral("text/type"), QStringLiteral("Text"), expected.toUtf8());

    QCOMPARE(annotation.displayText(), expected);
}

void KisAnnotationContractTest::cloneCreatesIndependentHeapObject()
{
    KisAnnotation original(QStringLiteral("clone/type"),
                           QStringLiteral("Clone description"),
                           QByteArrayLiteral("original payload"));
    std::unique_ptr<KisAnnotation> clone(original.clone());

    QVERIFY(clone);
    QVERIFY(clone.get() != &original);
    QCOMPARE(clone->type(), original.type());
    QCOMPARE(clone->description(), original.description());
    QCOMPARE(clone->annotation(), original.annotation());

    original.setAnnotation(QByteArrayLiteral("changed original"));
    QCOMPARE(clone->annotation(), QByteArrayLiteral("original payload"));

    clone->setAnnotation(QByteArrayLiteral("changed clone"));
    QCOMPARE(original.annotation(), QByteArrayLiteral("changed original"));
}

void KisAnnotationContractTest::virtualInterfaceDispatchesAndDestroysDerivedObject()
{
    AnnotationCalls calls;
    std::unique_ptr<KisAnnotation> annotation = std::make_unique<RoutingAnnotation>(&calls);

    QCOMPARE(annotation->displayText(), QStringLiteral("routed: routing/type"));
    QCOMPARE(calls.displayText, 1);

    std::unique_ptr<KisAnnotation> clone(annotation->clone());
    QCOMPARE(calls.clone, 1);
    QCOMPARE(clone->displayText(), QStringLiteral("routed: routing/type"));
    QCOMPARE(calls.displayText, 2);

    annotation.reset();
    QCOMPARE(calls.destruction, 1);
    clone.reset();
    QCOMPARE(calls.destruction, 2);
}

QTEST_GUILESS_MAIN(KisAnnotationContractTest)

#include "KisAnnotationContractTest.moc"
