/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Peter Simonsson <peter.simonsson@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeKeepAspectRatioCommand.h"

#include <klocalizedstring.h>

#include <KoShape.h>

namespace
{
using KeepAspectRatioReader = bool (*)(const KoShape *shape);
using KeepAspectRatioWriter = void (*)(KoShape *shape, bool keepAspectRatio);

bool readKeepAspectRatio(const KoShape *shape)
{
    return shape->keepAspectRatio();
}

void writeKeepAspectRatio(KoShape *shape, bool keepAspectRatio)
{
    shape->setKeepAspectRatio(keepAspectRatio);
}

struct ShapeAccess {
    KeepAspectRatioReader reader;
    KeepAspectRatioWriter writer;
};

const ShapeAccess defaultShapeAccess{readKeepAspectRatio, writeKeepAspectRatio};
ShapeAccess activeShapeAccess = defaultShapeAccess;
} // namespace

#if defined(KRITAFLAKE_SHAPE_KEEP_ASPECT_RATIO_COMMAND_CONTRACT_TESTING)
namespace KoShapeKeepAspectRatioCommandTesting
{
Q_DECL_HIDDEN void setShapeAccessForTesting(KeepAspectRatioReader reader, KeepAspectRatioWriter writer)
{
    activeShapeAccess = {reader, writer};
}

Q_DECL_HIDDEN void resetShapeAccessForTesting()
{
    activeShapeAccess = defaultShapeAccess;
}
} // namespace KoShapeKeepAspectRatioCommandTesting
#endif

KoShapeKeepAspectRatioCommand::KoShapeKeepAspectRatioCommand(const QList<KoShape *> &shapes,
                                                             bool newKeepAspectRatio,
                                                             KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Keep Aspect Ratio"), parent)
    , m_shapes(shapes)
{
    Q_FOREACH (KoShape *shape, shapes) {
        m_oldKeepAspectRatio << activeShapeAccess.reader(shape);
        m_newKeepAspectRatio << newKeepAspectRatio;
    }
}

KoShapeKeepAspectRatioCommand::~KoShapeKeepAspectRatioCommand()
{
}

void KoShapeKeepAspectRatioCommand::redo()
{
    KUndo2Command::redo();
    for (int i = 0; i < m_shapes.count(); ++i) {
        activeShapeAccess.writer(m_shapes[i], m_newKeepAspectRatio[i]);
    }
}

void KoShapeKeepAspectRatioCommand::undo()
{
    KUndo2Command::undo();
    for (int i = 0; i < m_shapes.count(); ++i) {
        activeShapeAccess.writer(m_shapes[i], m_oldKeepAspectRatio[i]);
    }
}
