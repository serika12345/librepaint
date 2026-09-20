/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDebug>
#include <QTest>
#include <QTransform>

#include "KisTransformComponents.h"
#include "kis_algebra_2d.h"

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

QTransform composedTransform(qreal scaleX,
                             qreal scaleY,
                             qreal shear,
                             qreal angle,
                             qreal dx,
                             qreal dy,
                             const QTransform &projective = QTransform())
{
    return QTransform::fromScale(scaleX, scaleY) * shearTransform(shear) * rotationTransform(angle)
        * QTransform::fromTranslate(dx, dy) * projective;
}

void compareReal(qreal actual, qreal expected, qreal tolerance = 1e-9)
{
    QVERIFY2(qAbs(actual - expected) < tolerance,
             qPrintable(QStringLiteral("actual=%1 expected=%2").arg(actual, 0, 'g', 16).arg(expected, 0, 'g', 16)));
}

void compareTransform(const QTransform &actual, const QTransform &expected, qreal tolerance = 1e-9)
{
    compareReal(actual.m11(), expected.m11(), tolerance);
    compareReal(actual.m12(), expected.m12(), tolerance);
    compareReal(actual.m13(), expected.m13(), tolerance);
    compareReal(actual.m21(), expected.m21(), tolerance);
    compareReal(actual.m22(), expected.m22(), tolerance);
    compareReal(actual.m23(), expected.m23(), tolerance);
    compareReal(actual.m31(), expected.m31(), tolerance);
    compareReal(actual.m32(), expected.m32(), tolerance);
    compareReal(actual.m33(), expected.m33(), tolerance);
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
    void decomposedMatrixDefaultsToValidIdentity();
    void affineDecompositionExposesEveryComponent();
    void partialTransformsRecomposeTheAffineInput();
    void projectiveDecompositionPreservesPerspective();
    void identityAndDegenerateInputsDefineValidityBoundary();
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

void KisTransformComponentsContractTest::decomposedMatrixDefaultsToValidIdentity()
{
    const KisAlgebra2D::DecomposedMatrix matrix;

    QVERIFY(matrix.isValid());
    compareReal(matrix.scaleX, 1.0);
    compareReal(matrix.scaleY, 1.0);
    compareReal(matrix.shearXY, 0.0);
    compareReal(matrix.angle, 0.0);
    compareReal(matrix.dx, 0.0);
    compareReal(matrix.dy, 0.0);
    compareReal(matrix.proj[0], 0.0);
    compareReal(matrix.proj[1], 0.0);
    compareReal(matrix.proj[2], 1.0);

    compareTransform(matrix.scaleTransform(), QTransform());
    compareTransform(matrix.shearTransform(), QTransform());
    compareTransform(matrix.rotateTransform(), QTransform());
    compareTransform(matrix.translateTransform(), QTransform());
    compareTransform(matrix.projectTransform(), QTransform());
    compareTransform(matrix.transform(), QTransform());
}

void KisTransformComponentsContractTest::affineDecompositionExposesEveryComponent()
{
    const QTransform input = composedTransform(2.0, 3.0, 0.25, 30.0, 7.0, -11.0);
    const KisAlgebra2D::DecomposedMatrix matrix(input);

    QVERIFY(matrix.isValid());
    compareReal(matrix.scaleX, 2.0);
    compareReal(matrix.scaleY, 3.0);
    compareReal(matrix.shearXY, 0.25);
    compareReal(matrix.angle, 30.0);
    compareReal(matrix.dx, 7.0);
    compareReal(matrix.dy, -11.0);
    compareReal(matrix.proj[0], 0.0);
    compareReal(matrix.proj[1], 0.0);
    compareReal(matrix.proj[2], 1.0);
}

void KisTransformComponentsContractTest::partialTransformsRecomposeTheAffineInput()
{
    const QTransform input = composedTransform(2.0, 3.0, 0.25, 30.0, 7.0, -11.0);
    const KisAlgebra2D::DecomposedMatrix matrix(input);

    compareTransform(matrix.scaleTransform(), QTransform::fromScale(2.0, 3.0));
    compareTransform(matrix.shearTransform(), shearTransform(0.25));
    compareTransform(matrix.rotateTransform(), rotationTransform(30.0));
    compareTransform(matrix.translateTransform(), QTransform::fromTranslate(7.0, -11.0));
    compareTransform(matrix.projectTransform(), QTransform());
    compareTransform(matrix.transform(), input);
}

void KisTransformComponentsContractTest::projectiveDecompositionPreservesPerspective()
{
    const QTransform perspective = projectiveTransform(0.01, -0.02);
    const QTransform input = composedTransform(2.0, 3.0, 0.25, 30.0, 7.0, -11.0, perspective);
    const KisAlgebra2D::DecomposedMatrix matrix(input);

    QVERIFY(matrix.isValid());
    compareReal(matrix.scaleX, 2.0);
    compareReal(matrix.scaleY, 3.0);
    compareReal(matrix.shearXY, 0.25);
    compareReal(matrix.angle, 30.0);
    compareReal(matrix.dx, 7.0);
    compareReal(matrix.dy, -11.0);
    compareReal(matrix.proj[0], 0.01);
    compareReal(matrix.proj[1], -0.02);
    compareReal(matrix.proj[2], 1.0);
    compareTransform(matrix.projectTransform(), perspective);
    compareTransform(matrix.transform(), input);
}

void KisTransformComponentsContractTest::identityAndDegenerateInputsDefineValidityBoundary()
{
    const KisAlgebra2D::DecomposedMatrix identity{QTransform()};
    QVERIFY(identity.isValid());
    compareTransform(identity.transform(), QTransform());

    const KisAlgebra2D::DecomposedMatrix zeroDeterminant(QTransform::fromScale(0.0, 1.0));
    QVERIFY(!zeroDeterminant.isValid());

    const KisAlgebra2D::DecomposedMatrix zeroProjectiveDenominator(
        QTransform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0));
    QVERIFY(!zeroProjectiveDenominator.isValid());
}

QTEST_GUILESS_MAIN(KisTransformComponentsContractTest)

#include "KisTransformComponentsContractTest.moc"
