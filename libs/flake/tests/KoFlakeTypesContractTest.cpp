/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoFlakeTypes.h"

#include <QSharedPointer>
#include <QTest>

#include <type_traits>

class KoShapeStroke
{
public:
    explicit KoShapeStroke(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KoShapeStroke()
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class KoShapeStrokeModel
{
public:
    explicit KoShapeStrokeModel(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KoShapeStrokeModel()
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

class KoFlakeTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sharedStrokeAliasesRetainTheirObjects();
};

void KoFlakeTypesContractTest::sharedStrokeAliasesRetainTheirObjects()
{
    static_assert(std::is_same_v<KoShapeStrokeSP, QSharedPointer<KoShapeStroke>>);
    static_assert(std::is_same_v<KoShapeStrokeModelSP, QSharedPointer<KoShapeStrokeModel>>);

    int strokeDestructionCount = 0;
    int modelDestructionCount = 0;
    KoShapeStrokeSP stroke(new KoShapeStroke(&strokeDestructionCount));
    KoShapeStrokeModelSP model(new KoShapeStrokeModel(&modelDestructionCount));

    {
        const KoShapeStrokeSP strokeCopy = stroke;
        const KoShapeStrokeModelSP modelCopy = model;
        QCOMPARE(strokeCopy.data(), stroke.data());
        QCOMPARE(modelCopy.data(), model.data());
    }

    QCOMPARE(strokeDestructionCount, 0);
    QCOMPARE(modelDestructionCount, 0);
    stroke.clear();
    model.clear();
    QCOMPARE(strokeDestructionCount, 1);
    QCOMPARE(modelDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KoFlakeTypesContractTest)

#include "KoFlakeTypesContractTest.moc"
