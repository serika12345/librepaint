/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SvgUtil.h"

#include <QRegularExpression>
#include <QStringList>
#include <QTransform>

#include <math.h>

double SvgUtil::fromUserSpace(double value)
{
    return value;
}

double SvgUtil::toUserSpace(double value)
{
    return value;
}

QPointF SvgUtil::toUserSpace(const QPointF &point)
{
    return QPointF(toUserSpace(point.x()), toUserSpace(point.y()));
}

QRectF SvgUtil::toUserSpace(const QRectF &rect)
{
    return QRectF(toUserSpace(rect.topLeft()), toUserSpace(rect.size()));
}

QSizeF SvgUtil::toUserSpace(const QSizeF &size)
{
    return QSizeF(toUserSpace(size.width()), toUserSpace(size.height()));
}

QPointF SvgUtil::objectToUserSpace(const QPointF &position, const QRectF &objectBound)
{
    qreal x = objectBound.left() + position.x() * objectBound.width();
    qreal y = objectBound.top() + position.y() * objectBound.height();
    return QPointF(x, y);
}

QSizeF SvgUtil::objectToUserSpace(const QSizeF &size, const QRectF &objectBound)
{
    qreal w = size.width() * objectBound.width();
    qreal h = size.height() * objectBound.height();
    return QSizeF(w, h);
}

QPointF SvgUtil::userSpaceToObject(const QPointF &position, const QRectF &objectBound)
{
    qreal x = 0.0;
    if (objectBound.width() != 0)
        x = (position.x() - objectBound.x()) / objectBound.width();
    qreal y = 0.0;
    if (objectBound.height() != 0)
        y = (position.y() - objectBound.y()) / objectBound.height();
    return QPointF(x, y);
}

QSizeF SvgUtil::userSpaceToObject(const QSizeF &size, const QRectF &objectBound)
{
    qreal w = objectBound.width() != 0 ? size.width() / objectBound.width() : 0.0;
    qreal h = objectBound.height() != 0 ? size.height() / objectBound.height() : 0.0;
    return QSizeF(w, h);
}

void SvgUtil::parseAspectRatio(const PreserveAspectRatioParser &p, const QRectF &elementBounds, const QRectF &viewBoxRect, QTransform *_viewTransform)
{
    if (p.mode != Qt::IgnoreAspectRatio) {
        QTransform viewBoxTransform = *_viewTransform;

        const qreal tan1 = viewBoxRect.height() / viewBoxRect.width();
        const qreal tan2 = elementBounds.height() / elementBounds.width();

        const qreal uniformScale =
            (p.mode == Qt::KeepAspectRatioByExpanding) ^ (tan1 > tan2) ?
                elementBounds.height() / viewBoxRect.height() :
                elementBounds.width() / viewBoxRect.width();

        viewBoxTransform =
            QTransform::fromTranslate(-viewBoxRect.x(), -viewBoxRect.y()) *
            QTransform::fromScale(uniformScale, uniformScale) *
            QTransform::fromTranslate(elementBounds.x(), elementBounds.y());

        const QPointF viewBoxAnchor = viewBoxTransform.map(p.rectAnchorPoint(viewBoxRect));
        const QPointF elementAnchor = p.rectAnchorPoint(elementBounds);
        const QPointF offset = elementAnchor - viewBoxAnchor;

        viewBoxTransform = viewBoxTransform * QTransform::fromTranslate(offset.x(), offset.y());

        *_viewTransform = viewBoxTransform;
    }
}

const char * SvgUtil::parseNumber(const char *ptr, qreal &number)
{
    int integer, exponent;
    qreal decimal, frac;
    int sign, expsign;

    exponent = 0;
    integer = 0;
    frac = 1.0;
    decimal = 0;
    sign = 1;
    expsign = 1;

    // read the sign
    if (*ptr == '+') {
        ptr++;
    } else if (*ptr == '-') {
        ptr++;
        sign = -1;
    }

    // read the integer part
    while (*ptr != '\0' && *ptr >= '0' && *ptr <= '9')
        integer = (integer * 10) + *(ptr++) - '0';
    if (*ptr == '.') { // read the decimals
        ptr++;
        while (*ptr != '\0' && *ptr >= '0' && *ptr <= '9')
            decimal += (*(ptr++) - '0') * (frac *= 0.1);
    }

    if (*ptr == 'e' || *ptr == 'E') { // read the exponent part
        ptr++;

        // read the sign of the exponent
        if (*ptr == '+') {
            ptr++;
        } else if (*ptr == '-') {
            ptr++;
            expsign = -1;
        }

        exponent = 0;
        while (*ptr != '\0' && *ptr >= '0' && *ptr <= '9') {
            exponent *= 10;
            exponent += *ptr - '0';
            ptr++;
        }
    }
    number = integer + decimal;
    number *= sign * pow((double)10, double(expsign * exponent));

    return ptr;
}

QStringList SvgUtil::simplifyList(const QString &str)
{
    QString attribute = str;
    attribute.replace(',', ' ');
    attribute.remove('\r');
    attribute.remove('\n');
    return attribute.simplified().split(' ', Qt::SkipEmptyParts);
}

SvgUtil::PreserveAspectRatioParser::PreserveAspectRatioParser(const QString &str)
{
    QRegularExpression rexp("(defer)?\\s*(none|(x(Min|Max|Mid)Y(Min|Max|Mid)))\\s*(meet|slice)?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = rexp.match(str.toLower());

    if (match.hasMatch()) {
        if (match.captured(1) == "defer") {
            defer = true;
        }

        if (match.captured(2) != "none") {
            xAlignment = alignmentFromString(match.captured(4));
            yAlignment = alignmentFromString(match.captured(5));
            mode = match.captured(6) == "slice" ?
                Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio;
        }
    }
}

QPointF SvgUtil::PreserveAspectRatioParser::rectAnchorPoint(const QRectF &rc) const
{
    return QPointF(alignedValue(rc.x(), rc.x() + rc.width(), xAlignment),
                   alignedValue(rc.y(), rc.y() + rc.height(), yAlignment));
}

QString SvgUtil::PreserveAspectRatioParser::toString() const
{
    QString result;

    if (!defer &&
        xAlignment == Middle &&
        yAlignment == Middle &&
        mode == Qt::KeepAspectRatio) {

        return result;
    }

    if (defer) {
        result += "defer ";
    }

    if (mode == Qt::IgnoreAspectRatio) {
        result += "none";
    } else {
        result += QString("x%1Y%2")
            .arg(alignmentToString(xAlignment))
            .arg(alignmentToString(yAlignment));

        if (mode == Qt::KeepAspectRatioByExpanding) {
            result += " slice";
        }
    }

    return result;
}

SvgUtil::PreserveAspectRatioParser::Alignment SvgUtil::PreserveAspectRatioParser::alignmentFromString(const QString &str) const {
    return
        str == "max" ? Max :
        str == "mid" ? Middle : Min;
}

QString SvgUtil::PreserveAspectRatioParser::alignmentToString(SvgUtil::PreserveAspectRatioParser::Alignment alignment) const
{
    return
        alignment == Max ? "Max" :
        alignment == Min ? "Min" :
        "Mid";

}

qreal SvgUtil::PreserveAspectRatioParser::alignedValue(qreal min, qreal max, SvgUtil::PreserveAspectRatioParser::Alignment alignment)
{
    qreal result = min;

    switch (alignment) {
    case Min:
        result = min;
        break;
    case Middle:
        result = 0.5 * (min + max);
        break;
    case Max:
        result = max;
        break;
    }

    return result;
}
