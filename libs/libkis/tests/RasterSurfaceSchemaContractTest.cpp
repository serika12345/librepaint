/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Channel.h>
#include <Document.h>

#include <QColor>
#include <QImage>
#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_CHANNEL_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Channel::method)), signature>)

#define ASSERT_DOCUMENT_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Document::method)), signature>)

} // namespace

class RasterSurfaceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rasterChannelTypeAndValueSchemaRemainStable();
    void rasterChannelIdentityVisibilityAndBoundsSchemaRemainStable();
    void rasterChannelPixelSignaturesRemainStable();
    void rasterDocumentColorSpaceAndBackgroundSignaturesRemainStable();
    void rasterDocumentPixelProjectionAndThumbnailSignaturesRemainStable();
};

void RasterSurfaceSchemaContractTest::rasterChannelTypeAndValueSchemaRemainStable()
{
    using ComparisonSignature = bool (Channel::*)(const Channel &) const;

    static_assert(std::is_class_v<Channel>);
    static_assert(std::is_base_of_v<QObject, Channel>);
    static_assert(std::is_constructible_v<Channel, KisNodeSP, KoChannelInfo *, QObject *>);
    static_assert(std::is_constructible_v<Channel, KisNodeSP, KoChannelInfo *>);
    static_assert(std::has_virtual_destructor_v<Channel>);
    ASSERT_CHANNEL_SIGNATURE(operator==, ComparisonSignature);
    ASSERT_CHANNEL_SIGNATURE(operator!=, ComparisonSignature);
}

void RasterSurfaceSchemaContractTest::rasterChannelIdentityVisibilityAndBoundsSchemaRemainStable()
{
    ASSERT_CHANNEL_SIGNATURE(name, QString (Channel::*)() const);
    ASSERT_CHANNEL_SIGNATURE(position, int (Channel::*)() const);
    ASSERT_CHANNEL_SIGNATURE(visible, bool (Channel::*)() const);
    ASSERT_CHANNEL_SIGNATURE(setVisible, void (Channel::*)(bool));
    ASSERT_CHANNEL_SIGNATURE(bounds, QRect (Channel::*)() const);
    ASSERT_CHANNEL_SIGNATURE(channelSize, int (Channel::*)() const);
}

void RasterSurfaceSchemaContractTest::rasterChannelPixelSignaturesRemainStable()
{
    ASSERT_CHANNEL_SIGNATURE(pixelData, QByteArray (Channel::*)(const QRect &) const);
    ASSERT_CHANNEL_SIGNATURE(setPixelData, void (Channel::*)(QByteArray, const QRect &));
}

void RasterSurfaceSchemaContractTest::rasterDocumentColorSpaceAndBackgroundSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(backgroundColor, QColor (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(colorDepth, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(colorModel, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(colorProfile, QString (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setBackgroundColor, bool (Document::*)(const QColor &));
    ASSERT_DOCUMENT_SIGNATURE(setColorProfile, bool (Document::*)(const QString &));
    ASSERT_DOCUMENT_SIGNATURE(setColorSpace, bool (Document::*)(const QString &, const QString &, const QString &));
}

void RasterSurfaceSchemaContractTest::rasterDocumentPixelProjectionAndThumbnailSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(pixelData, QByteArray (Document::*)(int, int, int, int) const);
    ASSERT_DOCUMENT_SIGNATURE(projection, QImage (Document::*)(int, int, int, int) const);
    ASSERT_DOCUMENT_SIGNATURE(thumbnail, QImage (Document::*)(int, int) const);

    using ProjectionWithDefaults = decltype(std::declval<const Document &>().projection());
    static_assert(std::is_same_v<ProjectionWithDefaults, QImage>);
}

QTEST_APPLESS_MAIN(RasterSurfaceSchemaContractTest)

#include "RasterSurfaceSchemaContractTest.moc"
