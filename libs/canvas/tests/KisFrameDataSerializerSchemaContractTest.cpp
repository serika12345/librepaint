/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/kis_frame_data_serializer.h>

#include <QTest>

#include <type_traits>
#include <vector>

class KisFrameDataSerializerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frameTileMemberSchemaRemainsStable();
    void frameTileLifecycleAndBufferSignaturesRemainStable();
    void frameMemberSchemaRemainsStable();
    void frameLifecycleAndValiditySignaturesRemainStable();
    void frameSerializerStorageAndDifferenceSignaturesRemainStable();
};

void KisFrameDataSerializerSchemaContractTest::frameTileMemberSchemaRemainsStable()
{
    using FrameTile = KisFrameDataSerializer::FrameTile;

    static_assert(std::is_class_v<FrameTile>);
    static_assert(std::is_same_v<decltype(FrameTile::col), int>);
    static_assert(std::is_same_v<decltype(FrameTile::row), int>);
    static_assert(std::is_same_v<decltype(FrameTile::rect), QRect>);
    static_assert(std::is_same_v<decltype(FrameTile::data), KisTileDataBuffer>);
}

void KisFrameDataSerializerSchemaContractTest::frameTileLifecycleAndBufferSignaturesRemainStable()
{
    using FrameTile = KisFrameDataSerializer::FrameTile;

    static_assert(std::is_constructible_v<FrameTile, KisTileDataPoolSP>);
    static_assert(!std::is_convertible_v<KisTileDataPoolSP, FrameTile>);
    static_assert(std::is_move_constructible_v<FrameTile>);
    static_assert(std::is_move_assignable_v<FrameTile>);
    static_assert(!std::is_copy_constructible_v<FrameTile>);
    static_assert(!std::is_assignable_v<FrameTile &, FrameTile &>);
    static_assert(std::is_same_v<decltype(&FrameTile::clone), FrameTile (FrameTile::*)() const>);
    static_assert(std::is_same_v<decltype(&FrameTile::isValid), bool (FrameTile::*)() const>);
}

void KisFrameDataSerializerSchemaContractTest::frameMemberSchemaRemainsStable()
{
    using Frame = KisFrameDataSerializer::Frame;
    using FrameTile = KisFrameDataSerializer::FrameTile;

    static_assert(std::is_class_v<Frame>);
    static_assert(std::is_same_v<decltype(Frame::pixelSize), int>);
    static_assert(std::is_same_v<decltype(Frame::frameTiles), std::vector<FrameTile>>);
}

void KisFrameDataSerializerSchemaContractTest::frameLifecycleAndValiditySignaturesRemainStable()
{
    using Frame = KisFrameDataSerializer::Frame;

    static_assert(std::is_default_constructible_v<Frame>);
    static_assert(std::is_move_constructible_v<Frame>);
    static_assert(std::is_move_assignable_v<Frame>);
    static_assert(!std::is_copy_constructible_v<Frame>);
    static_assert(!std::is_assignable_v<Frame &, Frame &>);
    static_assert(std::is_same_v<decltype(&Frame::clone), Frame (Frame::*)() const>);
    static_assert(std::is_same_v<decltype(&Frame::isValid), bool (Frame::*)() const>);
}

void KisFrameDataSerializerSchemaContractTest::frameSerializerStorageAndDifferenceSignaturesRemainStable()
{
    using Serializer = KisFrameDataSerializer;
    using Frame = Serializer::Frame;

    static_assert(std::is_class_v<Serializer>);
    static_assert(std::is_default_constructible_v<Serializer>);
    static_assert(std::is_constructible_v<Serializer, const QString &>);
    static_assert(std::is_destructible_v<Serializer>);
    static_assert(std::is_same_v<decltype(&Serializer::saveFrame), int (Serializer::*)(const Frame &)>);
    static_assert(std::is_same_v<decltype(&Serializer::loadFrame), Frame (Serializer::*)(int, KisTileDataPoolSP)>);
    static_assert(std::is_same_v<decltype(&Serializer::hasFrame), bool (Serializer::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Serializer::forgetFrame), void (Serializer::*)(int)>);
    static_assert(std::is_same_v<decltype(&Serializer::estimateFrameUniqueness),
                                 boost::optional<qreal> (*)(const Frame &, const Frame &, qreal)>);
    static_assert(std::is_same_v<decltype(&Serializer::subtractFrames), bool (*)(Frame &, const Frame &)>);
    static_assert(std::is_same_v<decltype(&Serializer::addFrames), void (*)(Frame &, const Frame &)>);
}

QTEST_APPLESS_MAIN(KisFrameDataSerializerSchemaContractTest)

#include "KisFrameDataSerializerSchemaContractTest.moc"
