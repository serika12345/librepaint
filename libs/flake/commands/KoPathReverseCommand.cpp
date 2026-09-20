/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathReverseCommand.h"
#include "KoPathShape.h"
#include <klocalizedstring.h>

namespace
{
using SubpathCountReader = int (*)(const KoPathShape *path);
using SubpathReverser = bool (*)(KoPathShape *path, int subpathIndex);

int readSubpathCount(const KoPathShape *path)
{
    return path->subpathCount();
}

bool reverseSubpath(KoPathShape *path, int subpathIndex)
{
    return path->reverseSubpath(subpathIndex);
}

struct PathAccess {
    SubpathCountReader countReader;
    SubpathReverser reverser;
};

const PathAccess defaultPathAccess{readSubpathCount, reverseSubpath};
PathAccess activePathAccess = defaultPathAccess;
} // namespace

#if defined(KRITAFLAKE_PATH_REVERSE_COMMAND_CONTRACT_TESTING)
namespace KoPathReverseCommandTesting
{
Q_DECL_HIDDEN void setPathAccessForTesting(SubpathCountReader countReader, SubpathReverser reverser)
{
    activePathAccess = {countReader, reverser};
}

Q_DECL_HIDDEN void resetPathAccessForTesting()
{
    activePathAccess = defaultPathAccess;
}
} // namespace KoPathReverseCommandTesting
#endif

class Q_DECL_HIDDEN KoPathReverseCommand::Private
{
public:
    Private(const QList<KoPathShape *> &p)
        : paths(p)
    {
    }
    ~Private()
    {
    }

    void reverse()
    {
        if (!paths.size())
            return;

        Q_FOREACH (KoPathShape *shape, paths) {
            int subpathCount = activePathAccess.countReader(shape);
            for (int i = 0; i < subpathCount; ++i)
                activePathAccess.reverser(shape, i);
        }
    }

    QList<KoPathShape *> paths;
};

KoPathReverseCommand::KoPathReverseCommand(const QList<KoPathShape *> &paths, KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new Private(paths))
{
    setText(kundo2_i18n("Reverse paths"));
}

KoPathReverseCommand::~KoPathReverseCommand()
{
    delete d;
}

void KoPathReverseCommand::redo()
{
    KUndo2Command::redo();

    d->reverse();
}

void KoPathReverseCommand::undo()
{
    KUndo2Command::undo();

    d->reverse();
}
