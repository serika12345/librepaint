/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDebug>
#include <QTest>
#include <QTransform>

#include "KisTransformComponents.h"

void kis_assert_exception(const char *, const char *, int)
{
    qFatal("Unexpected decomposition assertion");
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("Unexpected recoverable decomposition assertion");
}

QString __methodName(const char *prettyFunction)
{
    return QString::fromLatin1(prettyFunction);
}

namespace
{

using KisAlgebra2D::KisTransformComponent;
using KisAlgebra2D::KisTransformComponents;

KisTransformComponents allExcept(KisTransformComponent component)
{
    return KisAlgebra2D::makeFullTransformComponents().setFlag(component, false);
}

QTransform shearTransform(qreal shear)
{
    QTransform transform;
    transform.shear(shear, 0.0);
    return transform;
}

QTransform rotationTransform(qreal angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform;
}

QTransform projectiveTransform(qreal x, qreal y)
{
    return QTransform(1.0, 0.0, x, 0.0, 1.0, y, 0.0, 0.0, 1.0);
}

QString debugText(KisTransformComponent component)
{
    QString text;
    {
        QDebug debug(&text);
        debug << component;
    }
    return text.trimmed();
}

QString debugText(KisTransformComponents components)
{
    QString text;
    {
        QDebug debug(&text);
        debug << components;
    }
    return text.trimmed();
}

} // namespace

class KisTransformComponentsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void flagsRepresentEveryTransformComponent();
    void transformClassificationDistinguishesEveryComponent();
    void comparisonReportsUnchangedComponents();
    void debugOutputIsStable();
};

void KisTransformComponentsContractTest::flagsRepresentEveryTransformComponent()
{
    QCOMPARE(int(KisTransformComponent::Translate), 0x1);
    QCOMPARE(int(KisTransformComponent::Scale), 0x2);
    QCOMPARE(int(KisTransformComponent::Rotate), 0x4);
    QCOMPARE(int(KisTransformComponent::Shear), 0x8);
    QCOMPARE(int(KisTransformComponent::Project), 0x10);

    const KisTransformComponents expected = KisTransformComponent::Translate | KisTransformComponent::Scale
        | KisTransformComponent::Rotate | KisTransformComponent::Shear | KisTransformComponent::Project;
    QCOMPARE(KisAlgebra2D::makeFullTransformComponents(), expected);
}

void KisTransformComponentsContractTest::transformClassificationDistinguishesEveryComponent()
{
    QCOMPARE(KisAlgebra2D::componentsForTransform(QTransform()), KisTransformComponents());
    QCOMPARE(KisAlgebra2D::componentsForTransform(QTransform::fromTranslate(7.0, -11.0)),
             KisTransformComponents(KisTransformComponent::Translate));
    QCOMPARE(KisAlgebra2D::componentsForTransform(QTransform::fromScale(2.0, 3.0)),
             KisTransformComponents(KisTransformComponent::Scale));
    QCOMPARE(KisAlgebra2D::componentsForTransform(rotationTransform(30.0)),
             KisTransformComponents(KisTransformComponent::Rotate));
    QCOMPARE(KisAlgebra2D::componentsForTransform(shearTransform(0.25)),
             KisTransformComponents(KisTransformComponent::Shear));
    QCOMPARE(KisAlgebra2D::componentsForTransform(projectiveTransform(0.01, 0.02)),
             KisTransformComponents(KisTransformComponent::Project));

    const QTransform combined = QTransform::fromScale(2.0, 3.0) * shearTransform(0.25) * rotationTransform(30.0)
        * QTransform::fromTranslate(7.0, -11.0) * projectiveTransform(0.01, 0.02);
    QCOMPARE(KisAlgebra2D::componentsForTransform(combined), KisAlgebra2D::makeFullTransformComponents());
}

void KisTransformComponentsContractTest::comparisonReportsUnchangedComponents()
{
    const QTransform identity;

    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, identity), KisAlgebra2D::makeFullTransformComponents());
    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, QTransform::fromTranslate(7.0, -11.0)),
             allExcept(KisTransformComponent::Translate));
    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, QTransform::fromScale(2.0, 3.0)),
             allExcept(KisTransformComponent::Scale));
    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, rotationTransform(30.0)),
             allExcept(KisTransformComponent::Rotate));
    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, shearTransform(0.25)),
             allExcept(KisTransformComponent::Shear));
    QCOMPARE(KisAlgebra2D::compareTransformComponents(identity, projectiveTransform(0.01, 0.02)),
             allExcept(KisTransformComponent::Project));

    const QTransform nonUniformScale = QTransform::fromScale(2.0, 3.0);
    QCOMPARE(KisAlgebra2D::compareTransformComponents(nonUniformScale, nonUniformScale),
             KisAlgebra2D::makeFullTransformComponents());
}

void KisTransformComponentsContractTest::debugOutputIsStable()
{
    QCOMPARE(debugText(KisTransformComponent::Translate), QStringLiteral("KisTransformComponent::Translate"));
    QCOMPARE(debugText(KisTransformComponent::Scale), QStringLiteral("KisTransformComponent::Scale"));
    QCOMPARE(debugText(KisTransformComponent::Rotate), QStringLiteral("KisTransformComponent::Rotate"));
    QCOMPARE(debugText(KisTransformComponent::Shear), QStringLiteral("KisTransformComponent::Shear"));
    QCOMPARE(debugText(KisTransformComponent::Project), QStringLiteral("KisTransformComponent::Project"));

    QCOMPARE(debugText(KisTransformComponents()), QStringLiteral("KisTransformComponents()"));
    QCOMPARE(debugText(KisAlgebra2D::makeFullTransformComponents()),
             QStringLiteral("KisTransformComponents(Translate | Scale | Rotate | Shear | Project)"));
}

QTEST_GUILESS_MAIN(KisTransformComponentsContractTest)

#include "KisTransformComponentsContractTest.moc"
