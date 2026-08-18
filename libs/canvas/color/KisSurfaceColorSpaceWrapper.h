/*
 * SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_SURFACE_COLOR_SPACE_WRAPPER_H
#define KIS_SURFACE_COLOR_SPACE_WRAPPER_H

#include <QMetaObject>
#include <QtGlobal>
#include <boost/operators.hpp>

#include "kritacanvas_export.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QSurfaceFormat>
#else
#include <QColorSpace>
#endif

class KRITACANVAS_EXPORT KisSurfaceColorSpaceWrapper
    : public boost::equality_comparable<KisSurfaceColorSpaceWrapper>
{
public:
    enum ColorSpace {
        DefaultColorSpace,
        sRGBColorSpace,
        scRGBColorSpace,
        bt2020PQColorSpace
    };
    Q_ENUMS(ColorSpace)

    constexpr KisSurfaceColorSpaceWrapper()
        : KisSurfaceColorSpaceWrapper(DefaultColorSpace)
    {
    }

    constexpr KisSurfaceColorSpaceWrapper(ColorSpace colorSpace)
        : m_colorSpace(colorSpace)
    {
    }

    static constexpr KisSurfaceColorSpaceWrapper makeSRGBColorSpace()
    {
        return {sRGBColorSpace};
    }

    static constexpr KisSurfaceColorSpaceWrapper makeSCRGBColorSpace()
    {
        return {scRGBColorSpace};
    }

    static constexpr KisSurfaceColorSpaceWrapper makeBt2020PQColorSpace()
    {
        return {bt2020PQColorSpace};
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    static constexpr KisSurfaceColorSpaceWrapper fromQtColorSpace(
        QSurfaceFormat::ColorSpace colorSpace)
    {
        return {static_cast<ColorSpace>(colorSpace)};
    }
#else
    static KisSurfaceColorSpaceWrapper fromQtColorSpace(
        const QColorSpace &colorSpace)
    {
        if (colorSpace == QColorSpace()) {
            return {DefaultColorSpace};
        }
        if (colorSpace == QColorSpace::SRgb) {
            return {sRGBColorSpace};
        }
        if (colorSpace == QColorSpace::SRgbLinear) {
            return {scRGBColorSpace};
        }
        if (colorSpace == QColorSpace::Bt2100Pq) {
            return {bt2020PQColorSpace};
        }

        qWarning() << "WARNING: KisSurfaceColorSpaceWrapper: unsupported surface color space"
                   << colorSpace;
        return {DefaultColorSpace};
    }
#endif

    KisSurfaceColorSpaceWrapper(const KisSurfaceColorSpaceWrapper &) = default;
    KisSurfaceColorSpaceWrapper(KisSurfaceColorSpaceWrapper &&) = default;
    KisSurfaceColorSpaceWrapper &operator=(
        const KisSurfaceColorSpaceWrapper &) = default;
    KisSurfaceColorSpaceWrapper &operator=(
        KisSurfaceColorSpaceWrapper &&) = default;

    constexpr bool operator==(
        const KisSurfaceColorSpaceWrapper &other) const
    {
        return m_colorSpace == other.m_colorSpace;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    friend constexpr bool operator==(
        ColorSpace lhs,
        const KisSurfaceColorSpaceWrapper &rhs)
    {
        return lhs == rhs.m_colorSpace;
    }

    friend constexpr bool operator==(
        const KisSurfaceColorSpaceWrapper &lhs,
        ColorSpace rhs)
    {
        return lhs.m_colorSpace == rhs;
    }

    friend constexpr bool operator!=(
        ColorSpace lhs,
        const KisSurfaceColorSpaceWrapper &rhs)
    {
        return !(lhs == rhs);
    }

    friend constexpr bool operator!=(
        const KisSurfaceColorSpaceWrapper &lhs,
        ColorSpace rhs)
    {
        return !(lhs == rhs);
    }

    constexpr operator QSurfaceFormat::ColorSpace() const
    {
        return static_cast<QSurfaceFormat::ColorSpace>(m_colorSpace);
    }
#else
    operator QColorSpace() const
    {
        switch (m_colorSpace) {
        case DefaultColorSpace:
            return QColorSpace();
        case sRGBColorSpace:
            return QColorSpace::SRgb;
        case scRGBColorSpace:
            return QColorSpace::SRgbLinear;
        case bt2020PQColorSpace:
            return QColorSpace::Bt2100Pq;
        }
        Q_UNREACHABLE_RETURN(QColorSpace());
    }
#endif

private:
    ColorSpace m_colorSpace;
};

#endif
