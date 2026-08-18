/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISFRAMEDATASERIALIZER_H
#define KISFRAMEDATASERIALIZER_H

#include <kritacanvas_export.h>
#include <QRect>
#include <QScopedPointer>
#include <tiles/kis_tile_data_buffer.h>

#include <utility>
#include <vector>
#include <boost/optional.hpp>

class QString;


/**
 * KisFrameDataSerializer is the lowest level class for storing frame
 * data on disk. Its responsibilities are simple:
 *
 * 1) Accept low-level frame data object (KisFrameDataSerializer::Frame),
 *    which contains raw data in it (the data may be not a pixel data,
 *    but a preprocessed pixel differences)
 *
 * 2) Compress this data and save it on disk
 */

class KRITACANVAS_EXPORT KisFrameDataSerializer
{
public:
    struct FrameTile
    {
        explicit FrameTile(KisTileDataPoolSP pool) : data(std::move(pool)) {}

        FrameTile(FrameTile &&rhs) = default;
        FrameTile& operator=(FrameTile &&rhs) = default;

        FrameTile(const FrameTile &rhs) = delete;
        FrameTile& operator=(FrameTile &rhs) = delete;

        bool isValid() const {
            return data.data();
        }

        FrameTile clone() const{
            FrameTile tile(data.pool());
            tile.col = col;
            tile.row = row;
            tile.rect = rect;
            tile.data.allocate(data.pixelSize());

            const int bufferSize = data.pixelSize() * rect.width() * rect.height();
            memcpy(tile.data.data(), data.data(), bufferSize);

            return tile;
        }

        int col = -1;
        int row = -1;
        QRect rect;
        KisTileDataBuffer data;
    };

    struct Frame
    {
        Frame() = default;

        Frame(Frame&&rhs) = default;
        Frame& operator=(Frame &&rhs) = default;

        Frame(const Frame &rhs) = delete;
        Frame& operator=(Frame &rhs) = delete;

        Frame clone() const {
            Frame frame;
            frame.pixelSize = pixelSize;
            for (auto it = frameTiles.begin(); it != frameTiles.end(); ++it) {
                frame.frameTiles.push_back(it->clone());
            }
            return frame;
        }

        int pixelSize = 0;
        std::vector<FrameTile> frameTiles;

        bool isValid() const {
            return pixelSize > 0;
        }
    };

public:
    KisFrameDataSerializer();
    KisFrameDataSerializer(const QString &frameCachePath);
    ~KisFrameDataSerializer();

    int saveFrame(const Frame &frame);
    Frame loadFrame(int frameId, KisTileDataPoolSP pool);

    bool hasFrame(int frameId) const;
    void forgetFrame(int frameId);

    static boost::optional<qreal> estimateFrameUniqueness(const Frame &lhs, const Frame &rhs, qreal portion);
    static bool subtractFrames(Frame &dst, const Frame &src);
    static void addFrames(Frame &dst, const Frame &src);

private:
    template<template <typename U> class OpPolicy>
    static bool processFrames(KisFrameDataSerializer::Frame &dst, const KisFrameDataSerializer::Frame &src);

private:
    Q_DISABLE_COPY(KisFrameDataSerializer)

    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISFRAMEDATASERIALIZER_H
