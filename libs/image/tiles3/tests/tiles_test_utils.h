/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TILES_TEST_UTILS_H
#define TILES_TEST_UTILS_H

#include <kis_debug.h>

bool memoryIsFilled(quint8 c, quint8 *mem, qint32 size)
{
    for(; size > 0; size--)
        if(*(mem++) != c) {
            dbgKrita << "Expected" << c << "but found" << *(mem-1);
            return false;
        }

    return true;
}

#define TILESIZE 64*64


#endif /* TILES_TEST_UTILS_H */
