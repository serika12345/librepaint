/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Jeremy Lugagne <lugagne.jeremy@gmail.com>
 * SPDX-FileCopyrightText: 2011 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathShapeMarkerCommand.h"
#include "KoMarker.h"
#include "KoPathShape.h"
#include <KoShapeBulkActionLock.h>
#include <QExplicitlySharedDataPointer>
#include <kis_pointer_utils.h>

#include "kis_command_ids.h"

#include <klocalizedstring.h>

#include <utility>

namespace
{
using MarkerReader = KoMarker *(*)(const KoPathShape *shape, KoFlake::MarkerPosition position);
using AutoFillMarkerReader = bool (*)(const KoPathShape *shape);
using MarkerBatchApplier = void (*)(const QList<KoPathShape *> &shapes,
                                    const QList<KoMarker *> &markers,
                                    KoFlake::MarkerPosition position,
                                    const QList<bool> &autoFillMarkers);

KoMarker *readMarker(const KoPathShape *shape, KoFlake::MarkerPosition position)
{
    return shape->marker(position);
}

bool readAutoFillMarkers(const KoPathShape *shape)
{
    return shape->autoFillMarkers();
}

void applyMarkerBatch(const QList<KoPathShape *> &shapes,
                      const QList<KoMarker *> &markers,
                      KoFlake::MarkerPosition position,
                      const QList<bool> &autoFillMarkers)
{
    KoShapeBulkActionLock lock(implicitCastList<KoShape *>(shapes));

    auto markerIt = markers.cbegin();
    auto autoFillIt = autoFillMarkers.cbegin();
    for (KoPathShape *shape : shapes) {
        shape->setMarker(*markerIt, position);
        shape->setAutoFillMarkers(*autoFillIt);
        ++markerIt;
        ++autoFillIt;
    }

    KoShapeBulkActionLock::bulkShapesUpdate(lock.unlock());
}

MarkerReader activeMarkerReader = readMarker;
AutoFillMarkerReader activeAutoFillMarkerReader = readAutoFillMarkers;
MarkerBatchApplier activeMarkerBatchApplier = applyMarkerBatch;
} // namespace

#ifdef KRITAFLAKE_PATH_SHAPE_MARKER_COMMAND_CONTRACT_TESTING
namespace KoPathShapeMarkerCommandTesting
{
void setShapeAccessForTesting(MarkerReader markerReader,
                              AutoFillMarkerReader autoFillMarkerReader,
                              MarkerBatchApplier markerBatchApplier)
{
    activeMarkerReader = markerReader;
    activeAutoFillMarkerReader = autoFillMarkerReader;
    activeMarkerBatchApplier = markerBatchApplier;
}

void resetShapeAccessForTesting()
{
    activeMarkerReader = readMarker;
    activeAutoFillMarkerReader = readAutoFillMarkers;
    activeMarkerBatchApplier = applyMarkerBatch;
}
} // namespace KoPathShapeMarkerCommandTesting
#endif

struct Q_DECL_HIDDEN KoPathShapeMarkerCommand::Private {
    QList<KoPathShape *> shapes; ///< the shapes to set marker for
    QList<QExplicitlySharedDataPointer<KoMarker>> oldMarkers; ///< the old markers, one for each shape
    QExplicitlySharedDataPointer<KoMarker> marker; ///< the new marker to set
    KoFlake::MarkerPosition position;
    QList<bool> oldAutoFillMarkers;
};

KoPathShapeMarkerCommand::KoPathShapeMarkerCommand(const QList<KoPathShape *> &shapes,
                                                   KoMarker *marker,
                                                   KoFlake::MarkerPosition position,
                                                   KUndo2Command *parent)
    : KUndo2Command(kundo2_i18n("Set marker"), parent)
    , m_d(new Private)
{
    m_d->shapes = shapes;
    m_d->marker = marker;
    m_d->position = position;

    // save old markers
    Q_FOREACH (KoPathShape *shape, m_d->shapes) {
        m_d->oldMarkers.append(QExplicitlySharedDataPointer<KoMarker>(activeMarkerReader(shape, position)));
        m_d->oldAutoFillMarkers.append(activeAutoFillMarkerReader(shape));
    }
}

KoPathShapeMarkerCommand::~KoPathShapeMarkerCommand()
{
}

void KoPathShapeMarkerCommand::redo()
{
    KUndo2Command::redo();

    QList<KoMarker *> markers;
    QList<bool> autoFillMarkers;
    for (qsizetype i = 0; i < m_d->shapes.size(); ++i) {
        markers.append(m_d->marker.data());
        autoFillMarkers.append(true);
    }

    activeMarkerBatchApplier(m_d->shapes, markers, m_d->position, autoFillMarkers);
}

void KoPathShapeMarkerCommand::undo()
{
    KUndo2Command::undo();

    QList<KoMarker *> markers;
    markers.reserve(m_d->oldMarkers.size());
    for (const QExplicitlySharedDataPointer<KoMarker> &marker : std::as_const(m_d->oldMarkers)) {
        markers.append(marker.data());
    }

    activeMarkerBatchApplier(m_d->shapes, markers, m_d->position, m_d->oldAutoFillMarkers);
}

int KoPathShapeMarkerCommand::id() const
{
    return KisCommandUtils::ChangeShapeMarkersId;
}

bool KoPathShapeMarkerCommand::mergeWith(const KUndo2Command *command)
{
    const KoPathShapeMarkerCommand *other = dynamic_cast<const KoPathShapeMarkerCommand *>(command);

    if (!other || other->m_d->shapes != m_d->shapes || other->m_d->position != m_d->position) {
        return false;
    }

    m_d->marker = other->m_d->marker;
    return true;
}
