/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINTING_INFORMATION_BUILDER_ADAPTERS_H
#define KIS_PAINTING_INFORMATION_BUILDER_ADAPTERS_H

#include <kis_painting_information_builder.h>
#include <kritaui_export.h>

class KisCoordinatesConverter;
class KisToolFreehand;

class KRITAUI_EXPORT KisConverterPaintingInformationBuilder
    : public KisPaintingInformationBuilder
{
    Q_OBJECT

public:
    explicit KisConverterPaintingInformationBuilder(
        const KisCoordinatesConverter *converter);

protected:
    QPointF documentToImage(const QPointF &point) override;
    QPointF imageToDocument(const QPointF &point) override;
    QPointF imageToView(const QPointF &point) override;

    qreal canvasRotation() const override;
    bool canvasMirroredX() const override;
    bool canvasMirroredY() const override;

private:
    const KisCoordinatesConverter *m_converter;
};

class KRITAUI_EXPORT KisToolFreehandPaintingInformationBuilder
    : public KisPaintingInformationBuilder
{
    Q_OBJECT

public:
    explicit KisToolFreehandPaintingInformationBuilder(KisToolFreehand *tool);

protected:
    QPointF documentToImage(const QPointF &point) override;
    QPointF imageToDocument(const QPointF &point) override;
    QPointF imageToView(const QPointF &point) override;
    QPointF adjustDocumentPoint(const QPointF &point,
                                const QPointF &startPoint) override;
    qreal calculatePerspective(const QPointF &documentPoint) override;

    qreal canvasRotation() const override;
    bool canvasMirroredX() const override;
    bool canvasMirroredY() const override;

private:
    KisToolFreehand *m_tool;
};

#endif
