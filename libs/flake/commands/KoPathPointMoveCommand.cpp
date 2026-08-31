/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006, 2008-2009 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2006, 2007 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathPointMoveCommand.h"
#include "KoPathPoint.h"
#include "kis_command_ids.h"
#include "krita_container_utils.h"
#include <KoShapeBulkActionLock.h>
#include <klocalizedstring.h>

#include <algorithm>

namespace
{
using OffsetMap = QMap<KoPathPointData, QPointF>;
using OffsetBatchApplier = void (*)(const OffsetMap &points, const QSet<KoPathShape *> &paths, qreal factor);

void applyOffsetBatch(const OffsetMap &points, const QSet<KoPathShape *> &paths, qreal factor)
{
    QList<KoShape *> shapes;
    std::copy(paths.begin(), paths.end(), std::back_inserter(shapes));

    KoShapeBulkActionLock lock(shapes);

    for (auto it = points.cbegin(); it != points.cend(); ++it) {
        KoPathShape *path = it.key().pathShape;
        // transform offset from document to shape coordinate system
        QPointF shapeOffset = path->documentToShape(factor * it.value()) - path->documentToShape(QPointF());
        QTransform matrix;
        matrix.translate(shapeOffset.x(), shapeOffset.y());

        KoPathPoint *point = path->pointByIndex(it.key().pointIndex);
        if (point) {
            point->map(matrix);
        }
    }

    for (KoPathShape *path : paths) {
        path->normalize();
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

OffsetBatchApplier activeOffsetBatchApplier = applyOffsetBatch;
} // namespace

#ifdef KRITAFLAKE_PATH_POINT_MOVE_COMMAND_CONTRACT_TESTING
namespace KoPathPointMoveCommandTesting
{
Q_DECL_HIDDEN void setOffsetBatchApplierForTesting(OffsetBatchApplier applier)
{
    activeOffsetBatchApplier = applier;
}

Q_DECL_HIDDEN void resetOffsetBatchApplierForTesting()
{
    activeOffsetBatchApplier = applyOffsetBatch;
}
} // namespace KoPathPointMoveCommandTesting
#endif

class KoPathPointMoveCommandPrivate
{
public:
    KoPathPointMoveCommandPrivate()
    {
    }
    void applyOffset(qreal factor);

    OffsetMap points;
    QSet<KoPathShape *> paths;
};

KoPathPointMoveCommand::KoPathPointMoveCommand(const QList<KoPathPointData> &pointData,
                                               const QPointF &offset,
                                               KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new KoPathPointMoveCommandPrivate())
{
    setText(kundo2_i18n("Move points"));

    foreach (const KoPathPointData &data, pointData) {
        if (!d->points.contains(data)) {
            d->points[data] = offset;
            d->paths.insert(data.pathShape);
        }
    }
}

KoPathPointMoveCommand::KoPathPointMoveCommand(const QList<KoPathPointData> &pointData,
                                               const QList<QPointF> &offsets,
                                               KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new KoPathPointMoveCommandPrivate())
{
    Q_ASSERT(pointData.count() == offsets.count());

    setText(kundo2_i18n("Move points"));

    const qsizetype dataCount = pointData.count();
    for (qsizetype i = 0; i < dataCount; ++i) {
        const KoPathPointData &data = pointData[i];
        if (!d->points.contains(data)) {
            d->points[data] = offsets[i];
            d->paths.insert(data.pathShape);
        }
    }
}

KoPathPointMoveCommand::~KoPathPointMoveCommand()
{
    delete d;
}

void KoPathPointMoveCommand::redo()
{
    KUndo2Command::redo();
    d->applyOffset(1.0);
}

void KoPathPointMoveCommand::undo()
{
    KUndo2Command::undo();
    d->applyOffset(-1.0);
}

int KoPathPointMoveCommand::id() const
{
    return KisCommandUtils::ChangePathShapePointId;
}

bool KoPathPointMoveCommand::mergeWith(const KUndo2Command *command)
{
    const KoPathPointMoveCommand *other = dynamic_cast<const KoPathPointMoveCommand *>(command);

    if (!other || other->d->paths != d->paths
        || !KritaUtils::compareListsUnordered(other->d->points.keys(), d->points.keys())) {
        return false;
    }

    auto it = d->points.begin();
    while (it != d->points.end()) {
        it.value() += other->d->points[it.key()];
        ++it;
    }

    return true;
}

void KoPathPointMoveCommandPrivate::applyOffset(qreal factor)
{
    activeOffsetBatchApplier(points, paths, factor);
}
