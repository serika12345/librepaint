/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPathFillRuleCommand.h>
#include <KoPathShape.h>

#include <QHash>
#include <QTest>
#include <QTime>

#include <cstddef>

namespace
{
QHash<const KUndo2Command *, KUndo2MagicString> commandTexts;
}

KUndo2MagicString::KUndo2MagicString() = default;

KUndo2MagicString::KUndo2MagicString(const QString &text)
    : m_text(text)
{
}

QString KUndo2MagicString::toString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.left(separator) : m_text;
}

QString KUndo2MagicString::toSecondaryString() const
{
    const int separator = m_text.indexOf(QLatin1Char('\n'));
    return separator > 0 ? m_text.mid(separator + 1) : m_text;
}

bool KUndo2MagicString::isEmpty() const
{
    return m_text.isEmpty();
}

bool KUndo2MagicString::operator==(const KUndo2MagicString &rhs) const
{
    return m_text == rhs.m_text;
}

KUndo2Command::KUndo2Command(KUndo2Command *parent)
{
    Q_UNUSED(parent);
}

KUndo2Command::KUndo2Command(const KUndo2MagicString &text, KUndo2Command *parent)
{
    Q_UNUSED(parent);
    setText(text);
}

KUndo2Command::~KUndo2Command()
{
    commandTexts.remove(this);
}

void KUndo2Command::undo()
{
}

void KUndo2Command::redo()
{
}

QString KUndo2Command::actionText() const
{
    return commandTexts.value(this).toSecondaryString();
}

void KUndo2Command::setText(const KUndo2MagicString &text)
{
    commandTexts.insert(this, text);
}

int KUndo2Command::id() const
{
    return -1;
}

int KUndo2Command::timedId() const
{
    return m_timedID;
}

void KUndo2Command::setTimedID(int timedID)
{
    m_timedID = timedID;
}

bool KUndo2Command::mergeWith(const KUndo2Command *other)
{
    Q_UNUSED(other);
    return false;
}

bool KUndo2Command::timedMergeWith(KUndo2Command *other)
{
    Q_UNUSED(other);
    return false;
}

bool KUndo2Command::canAnnihilateWith(const KUndo2Command *other) const
{
    Q_UNUSED(other);
    return false;
}

void KUndo2Command::setTime(const QTime &time)
{
    m_timeOfCreation = time;
}

QTime KUndo2Command::time() const
{
    return m_timeOfCreation;
}

void KUndo2Command::setEndTime(const QTime &time)
{
    m_endOfCommand = time;
}

QTime KUndo2Command::endTime() const
{
    return m_endOfCommand;
}

QVector<KUndo2Command *> KUndo2Command::mergeCommandsVector() const
{
    return m_mergeCommandsVector;
}

bool KUndo2Command::isMerged() const
{
    return !m_mergeCommandsVector.isEmpty();
}

void KUndo2Command::undoMergedCommands()
{
}

void KUndo2Command::redoMergedCommands()
{
}

namespace KoPathFillRuleCommandTesting
{
using FillRuleReader = Qt::FillRule (*)(const KoPathShape *shape);
using FillRuleWriter = void (*)(KoPathShape *shape, Qt::FillRule fillRule);
using UpdateRequester = void (*)(KoPathShape *shape);

void setShapeAccessForTesting(FillRuleReader reader, FillRuleWriter writer, UpdateRequester updater);
void resetShapeAccessForTesting();
} // namespace KoPathFillRuleCommandTesting

namespace
{
struct ShapeObservation {
    Qt::FillRule fillRule{Qt::OddEvenFill};
    int writeCount{0};
    int updateCount{0};
};

struct alignas(std::max_align_t) ShapeToken {
    unsigned char value{0};
};

QHash<const KoPathShape *, ShapeObservation> shapeObservations;

Qt::FillRule readFillRule(const KoPathShape *shape)
{
    return shapeObservations.value(shape).fillRule;
}

void writeFillRule(KoPathShape *shape, Qt::FillRule fillRule)
{
    ShapeObservation &observation = shapeObservations[shape];
    observation.fillRule = fillRule;
    ++observation.writeCount;
}

void requestUpdate(KoPathShape *shape)
{
    ++shapeObservations[shape].updateCount;
}
} // namespace

Qt::FillRule KoPathShape::fillRule() const
{
    return readFillRule(this);
}

void KoPathShape::setFillRule(Qt::FillRule fillRule)
{
    writeFillRule(this, fillRule);
}

namespace
{
class ShapeAccessScope
{
public:
    ShapeAccessScope()
    {
        KoPathFillRuleCommandTesting::setShapeAccessForTesting(readFillRule, writeFillRule, requestUpdate);
    }

    ~ShapeAccessScope()
    {
        KoPathFillRuleCommandTesting::resetShapeAccessForTesting();
    }
};
} // namespace

class KoPathFillRuleCommandContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesPerShapeRulesAcrossUndoRedoCycles();
};

void KoPathFillRuleCommandContractTest::preservesPerShapeRulesAcrossUndoRedoCycles()
{
    ShapeAccessScope accessScope;
    ShapeToken firstStorage;
    ShapeToken secondStorage;
    auto *firstShape = reinterpret_cast<KoPathShape *>(&firstStorage);
    auto *secondShape = reinterpret_cast<KoPathShape *>(&secondStorage);
    shapeObservations = {
        {firstShape, {Qt::OddEvenFill, 0, 0}},
        {secondShape, {Qt::WindingFill, 0, 0}},
    };

    KoPathFillRuleCommand command({firstShape, secondShape}, Qt::WindingFill);
    QCOMPARE(command.actionText(), QStringLiteral("Set fill rule"));

    shapeObservations[firstShape].fillRule = Qt::WindingFill;
    shapeObservations[secondShape].fillRule = Qt::OddEvenFill;

    command.redo();
    QCOMPARE(shapeObservations[firstShape].fillRule, Qt::WindingFill);
    QCOMPARE(shapeObservations[secondShape].fillRule, Qt::WindingFill);
    QCOMPARE(shapeObservations[firstShape].updateCount, 1);
    QCOMPARE(shapeObservations[secondShape].updateCount, 1);

    command.undo();
    QCOMPARE(shapeObservations[firstShape].fillRule, Qt::OddEvenFill);
    QCOMPARE(shapeObservations[secondShape].fillRule, Qt::WindingFill);
    QCOMPARE(shapeObservations[firstShape].updateCount, 2);
    QCOMPARE(shapeObservations[secondShape].updateCount, 2);

    command.redo();
    QCOMPARE(shapeObservations[firstShape].fillRule, Qt::WindingFill);
    QCOMPARE(shapeObservations[secondShape].fillRule, Qt::WindingFill);
    QCOMPARE(shapeObservations[firstShape].writeCount, 3);
    QCOMPARE(shapeObservations[secondShape].writeCount, 3);
    QCOMPARE(shapeObservations[firstShape].updateCount, 3);
    QCOMPARE(shapeObservations[secondShape].updateCount, 3);
}

QTEST_GUILESS_MAIN(KoPathFillRuleCommandContractTest)

#include "KoPathFillRuleCommandContractTest.moc"
