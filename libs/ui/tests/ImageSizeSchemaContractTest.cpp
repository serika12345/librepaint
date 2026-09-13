/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "imagesize/imagesize.h"

#include <QTest>

#include <type_traits>

class ImageSizeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionPluginTypeConstructionAndLifetimeSchemaRemainStable();
};

void ImageSizeSchemaContractTest::actionPluginTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_base_of_v<KisActionPlugin, ImageSize>);
    static_assert(std::is_constructible_v<ImageSize, QObject *>);
    static_assert(std::has_virtual_destructor_v<ImageSize>);
}

QTEST_GUILESS_MAIN(ImageSizeSchemaContractTest)

#include "ImageSizeSchemaContractTest.moc"
