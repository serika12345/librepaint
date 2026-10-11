/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_RGB8_DISPLAY_LOOKUP_H
#define KIS_RGB8_DISPLAY_LOOKUP_H
#include <KoColorConversionTransformation.h>
#include <QByteArray>
#include <atomic>
#include <memory>
class KoColorSpace;

/** Exact RGB8 display values generated with one thread-local color converter.
 * Borrowed BGRA8 color spaces outlive this call. Cancellation is checked per blue
 * plane. Ready with a null table represents identity without a GPU allocation.
 * Allocation failure propagates std::bad_alloc; partial tables remain private.
 */
namespace KisRgb8DisplayLookup {
enum class Status { Ready, Cancelled, InvalidColorSpace, ConversionFailed };
struct Result {
    Status status = Status::ConversionFailed;
    std::shared_ptr<const QByteArray> values;
};
Result build(const KoColorSpace *source, const KoColorSpace *destination,
             KoColorConversionTransformation::Intent intent,
             KoColorConversionTransformation::ConversionFlags flags,
             const std::atomic_bool &cancelled);
}
#endif
