/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2006, 2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoSubpathRemoveCommand.h"

#include "KoPathPoint.h"
#include "KoPathShape.h"
#include <klocalizedstring.h>

namespace
{
using PathUpdater = void (*)(KoPathShape *pathShape);
using SubpathRemover = KoSubpath *(*)(KoPathShape * pathShape, int subpathIndex);
using PathNormalizer = QPointF (*)(KoPathShape *pathShape);
using PointMapper = void (*)(KoPathPoint *point, const QTransform &matrix);
using SubpathAdder = bool (*)(KoPathShape *pathShape, KoSubpath *subpath, int subpathIndex);
using SubpathDeleter = void (*)(KoSubpath *subpath);

void updatePath(KoPathShape *pathShape)
{
    pathShape->update();
}

KoSubpath *removeSubpath(KoPathShape *pathShape, int subpathIndex)
{
    return pathShape->removeSubpath(subpathIndex);
}

QPointF normalizePath(KoPathShape *pathShape)
{
    return pathShape->normalize();
}

void mapPoint(KoPathPoint *point, const QTransform &matrix)
{
    point->map(matrix);
}

bool addSubpath(KoPathShape *pathShape, KoSubpath *subpath, int subpathIndex)
{
    return pathShape->addSubpath(subpath, subpathIndex);
}

void deleteSubpath(KoSubpath *subpath)
{
    qDeleteAll(*subpath);
    delete subpath;
}

struct PathAccess {
    PathUpdater updater;
    SubpathRemover remover;
    PathNormalizer normalizer;
    PointMapper mapper;
    SubpathAdder adder;
    SubpathDeleter deleter;
};

const PathAccess defaultPathAccess{updatePath, removeSubpath, normalizePath, mapPoint, addSubpath, deleteSubpath};
PathAccess activePathAccess = defaultPathAccess;
} // namespace

#if defined(KRITAFLAKE_SUBPATH_REMOVE_COMMAND_CONTRACT_TESTING)
namespace KoSubpathRemoveCommandTesting
{
Q_DECL_HIDDEN void setPathAccessForTesting(PathUpdater updater,
                                           SubpathRemover remover,
                                           PathNormalizer normalizer,
                                           PointMapper mapper,
                                           SubpathAdder adder,
                                           SubpathDeleter deleter)
{
    activePathAccess = {updater, remover, normalizer, mapper, adder, deleter};
}

Q_DECL_HIDDEN void resetPathAccessForTesting()
{
    activePathAccess = defaultPathAccess;
}
} // namespace KoSubpathRemoveCommandTesting
#endif

KoSubpathRemoveCommand::KoSubpathRemoveCommand(KoPathShape *pathShape, int subpathIndex, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_pathShape(pathShape)
    , m_subpathIndex(subpathIndex)
    , m_subpath(0)
{
    setText(kundo2_i18n("Remove subpath"));
}

KoSubpathRemoveCommand::~KoSubpathRemoveCommand()
{
    if (m_subpath) {
        activePathAccess.deleter(m_subpath);
    }
}

void KoSubpathRemoveCommand::redo()
{
    KUndo2Command::redo();
    activePathAccess.updater(m_pathShape);
    m_subpath = activePathAccess.remover(m_pathShape, m_subpathIndex);
    if (m_subpath) {
        QPointF offset = activePathAccess.normalizer(m_pathShape);

        QTransform matrix;
        matrix.translate(-offset.x(), -offset.y());
        Q_FOREACH (KoPathPoint *point, *m_subpath) {
            activePathAccess.mapper(point, matrix);
        }
        activePathAccess.updater(m_pathShape);
    }
}

void KoSubpathRemoveCommand::undo()
{
    KUndo2Command::undo();
    if (m_subpath) {
        activePathAccess.adder(m_pathShape, m_subpath, m_subpathIndex);
        activePathAccess.normalizer(m_pathShape);
        activePathAccess.updater(m_pathShape);
        m_subpath = 0;
    }
}
