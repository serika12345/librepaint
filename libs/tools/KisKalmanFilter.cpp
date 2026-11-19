/*
 *  SPDX-FileCopyrightText: 2018 The Chromium Authors
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "KisKalmanFilter.h"

namespace
{
constexpr quint32 StableIterationCount = 4;

constexpr qreal ProcessNoiseScale = 0.01;
constexpr qreal MeasurementNoiseVariance = 1.0;

using Vector3 = KisKalmanFilter::Vector3;
using Matrix3 = KisKalmanFilter::Matrix3;

const Vector3 &measurementVector()
{
    // H = {1, 0, 0}: only the position is observed.
    static const Vector3 vector{1.0, 0.0, 0.0};
    return vector;
}

Matrix3 identityMatrix()
{
    Matrix3 matrix;
    matrix.values[0][0] = 1.0;
    matrix.values[1][1] = 1.0;
    matrix.values[2][2] = 1.0;
    return matrix;
}

Matrix3 stateTransition(qreal dt)
{
    // new_pos = pre_pos + v * dt + 1/2 * a * dt^2.
    // new_v = v + a * dt.
    // new_a = a.
    Matrix3 matrix;
    matrix.values[0][0] = 1.0;
    matrix.values[0][1] = dt;
    matrix.values[0][2] = 0.5 * dt * dt;
    matrix.values[1][1] = 1.0;
    matrix.values[1][2] = dt;
    matrix.values[2][2] = 1.0;
    return matrix;
}

/// Q = sigma_process^2 * G * G^T, G = {1/2 * dt^2, dt, 1}: the noise is a
/// noisy force on the pointer.
Matrix3 processNoise(qreal dt)
{
    const qreal g[3] = {0.5 * dt * dt, dt, 1.0};

    Matrix3 matrix;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            matrix.values[row][column] = ProcessNoiseScale * g[row] * g[column];
        }
    }
    return matrix;
}

Matrix3 transpose(const Matrix3 &matrix)
{
    Matrix3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.values[row][column] = matrix.values[column][row];
        }
    }
    return result;
}

Matrix3 product(const Matrix3 &left, const Matrix3 &right)
{
    Matrix3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            for (int index = 0; index < 3; ++index) {
                result.values[row][column] +=
                    left.values[row][index] * right.values[index][column];
            }
        }
    }
    return result;
}

Vector3 product(const Matrix3 &matrix, const Vector3 &vector)
{
    const qreal elements[3] = {vector.x, vector.y, vector.z};

    Vector3 result;
    qreal values[3] = {0.0, 0.0, 0.0};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            values[row] += matrix.values[row][column] * elements[column];
        }
    }
    result.x = values[0];
    result.y = values[1];
    result.z = values[2];
    return result;
}

Matrix3 outerProduct(const Vector3 &left, const Vector3 &right)
{
    const qreal leftElements[3] = {left.x, left.y, left.z};
    const qreal rightElements[3] = {right.x, right.y, right.z};

    Matrix3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.values[row][column] = leftElements[row] * rightElements[column];
        }
    }
    return result;
}

Matrix3 sum(const Matrix3 &left, const Matrix3 &right)
{
    Matrix3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.values[row][column] =
                left.values[row][column] + right.values[row][column];
        }
    }
    return result;
}

Matrix3 difference(const Matrix3 &left, const Matrix3 &right)
{
    Matrix3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.values[row][column] =
                left.values[row][column] - right.values[row][column];
        }
    }
    return result;
}

Vector3 scaled(const Vector3 &vector, qreal factor)
{
    Vector3 result;
    result.x = vector.x * factor;
    result.y = vector.y * factor;
    result.z = vector.z * factor;
    return result;
}

Vector3 added(const Vector3 &left, const Vector3 &right)
{
    Vector3 result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    return result;
}

qreal dotProduct(const Vector3 &left, const Vector3 &right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}
} // namespace

KisKalmanFilter::KisKalmanFilter()
    : m_covariance(identityMatrix())
{
}

const KisKalmanFilter::Vector3 &KisKalmanFilter::state() const
{
    return m_state;
}

const KisKalmanFilter::Matrix3 &KisKalmanFilter::covariance() const
{
    return m_covariance;
}

bool KisKalmanFilter::stable() const
{
    return m_iterationCount >= StableIterationCount;
}

void KisKalmanFilter::update(qreal observation, qreal dt)
{
    if (m_iterationCount++ == 0) {
        // The first update only sets the state estimation.
        m_state = Vector3{observation, 0.0, 0.0};
        return;
    }

    predict(dt);

    const Vector3 &h = measurementVector();

    // Y = z - H * X
    const qreal y = observation - dotProduct(h, m_state);
    // S = H * P * H' + R
    const qreal s = dotProduct(h, product(m_covariance, h)) + MeasurementNoiseVariance;
    // K = P * H' * inv(S)
    const Vector3 kalmanGain = scaled(product(m_covariance, h), 1.0 / s);
    // X = X + K * Y
    m_state = added(m_state, scaled(kalmanGain, y));
    // I_HK = eye(P) - K * H
    const Matrix3 iKh = difference(identityMatrix(), outerProduct(kalmanGain, h));

    // P = I_HK * P * I_HK' + K * R * K'
    m_covariance = sum(product(product(iKh, m_covariance), transpose(iKh)),
                       outerProduct(scaled(kalmanGain, MeasurementNoiseVariance),
                                    kalmanGain));
}

void KisKalmanFilter::reset()
{
    m_state = Vector3();
    m_covariance = identityMatrix();
    m_iterationCount = 0;
}

qreal KisKalmanFilter::position() const
{
    return m_state.x;
}

qreal KisKalmanFilter::velocity() const
{
    return m_state.y;
}

qreal KisKalmanFilter::acceleration() const
{
    return m_state.z;
}

void KisKalmanFilter::predict(qreal dt)
{
    const Matrix3 transition = stateTransition(dt);

    // X = F * X
    m_state = product(transition, m_state);
    // P = F * P * F' + Q
    m_covariance = sum(product(product(transition, m_covariance), transpose(transition)),
                       processNoise(dt));
}
