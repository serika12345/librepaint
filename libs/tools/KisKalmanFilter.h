/*
 *  SPDX-FileCopyrightText: 2018 The Chromium Authors
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KIS_KALMAN_FILTER_H
#define KIS_KALMAN_FILTER_H

#include <QtGlobal>

#include <kritatools_export.h>

/**
 * A Kalman filter of one axis with three states: position, velocity and
 * acceleration. The equations, the constants, the initial covariance and the
 * number of updates that make the filter stable are the ones of Chromium's
 * ui/base/prediction/kalman_filter, of which this class is a port. Chromium
 * keeps the state in gfx::Vector3dF and gfx::Matrix3F; the small vector and
 * matrix in this file replace them.
 */
class KRITATOOLS_EXPORT KisKalmanFilter
{
public:
    struct Vector3 {
        qreal x = 0.0;
        qreal y = 0.0;
        qreal z = 0.0;
    };

    struct Matrix3 {
        qreal values[3][3] = {};
    };

    KisKalmanFilter();

    const Vector3 &state() const;
    const Matrix3 &covariance() const;

    /// True once the filter has seen four updates.
    bool stable() const;

    /// Move to the observation, dt ms after the previous one. The first
    /// update only sets the position.
    void update(qreal observation, qreal dt);

    /// Back to the initial state and covariance, and to zero updates.
    void reset();

    qreal position() const;
    qreal velocity() const;
    qreal acceleration() const;

private:
    void predict(qreal dt);

    Vector3 m_state;
    Matrix3 m_covariance;
    quint32 m_iterationCount = 0;
};

#endif // KIS_KALMAN_FILTER_H
