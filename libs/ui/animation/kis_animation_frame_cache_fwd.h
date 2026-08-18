/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_ANIMATION_FRAME_CACHE_FWD_H
#define KIS_ANIMATION_FRAME_CACHE_FWD_H

template<class T>
class KisSharedPtr;

template<class T>
class KisWeakSharedPtr;

class KisAnimationFrameCache;
using KisAnimationFrameCacheSP = KisSharedPtr<KisAnimationFrameCache>;
using KisAnimationFrameCacheWSP = KisWeakSharedPtr<KisAnimationFrameCache>;

#endif
