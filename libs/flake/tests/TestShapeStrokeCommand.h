/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef TESTSHAPESTROKECOMMAND_H
#define TESTSHAPESTROKECOMMAND_H

#include <QObject>

class TestShapeStrokeCommand : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void uniformAndSingleConstructorsCapturePreviousStrokes();
    void perShapeConstructorPreservesStrokePairing();
    void redoAndUndoDispatchBaseBeforeStrokeBatch();
    void idAndMergeAcceptOnlyIdenticalShapeSequences();
    void emptyInputsAndSharedOwnershipRemainSafe();
};

#endif // TESTSHAPESTROKECOMMAND_H
