/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisImageBarrierLock.h>
#include <KisNodeAdditionFlags.h>
#include <KisRenderedDab.h>
#include <KisRunnableStrokeJobUtils.h>
#include <KisSelectionTags.h>
#include <KisSequentialIteratorProgress.h>
#include <KisWraparoundAxis.h>
#include <brushengine/kis_locked_properties.h>
#include <brushengine/kis_paintop_lod_limitations.h>
#include <brushengine/kis_standard_uniform_properties_factory.h>
#include <commands/kis_change_filter_command.h>
#include <commands/kis_image_commands.h>
#include <commands/kis_node_commands.h>
#include <commands/kis_selection_commands.h>
#include <kis_brush_mask_applicator_base.h>
#include <kis_cached_paint_device.h>
#include <kis_cross_device_color_sampler.h>
#include <kis_datamanager.h>
#include <kis_do_something_command.h>
#include <kis_exif_info_visitor.h>
#include <kis_layer_projection_plane.h>
#include <kis_mask_generator.h>
#include <kis_progress_update_helper.h>
#include <kis_sequential_iterator.h>
#include <kis_threaded_text_rendering_workaround.h>
#include <kis_transaction.h>
#include <kis_types.h>
#include <kis_vec.h>
#include <kis_wrapped_rect.h>

class TestPublicImageHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersCompileAsOnePublicSurface();
};

void TestPublicImageHeaders::headersCompileAsOnePublicSurface()
{
    QVERIFY(true);
}

QTEST_MAIN(TestPublicImageHeaders)

#include "TestPublicImageHeaders.moc"
