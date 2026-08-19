/*
 * SPDX-FileCopyrightText: 2022 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSvgTextShape.h"
#include "KoSvgTextShape_p.h"

#include "KoSvgTextProperties.h"

int KoSvgTextShape::wordLeft(int pos, bool visual)
{
    if (pos < 0 || pos > d->cursorPos.size()-1 || d->result.isEmpty() || d->cursorPos.isEmpty()) {
        return pos;
    }
    if (writingMode() != KoSvgText::HorizontalTB) {
        return posLeft(pos, visual);
    }

    KoSvgText::Direction direction = KoSvgText::Direction(this->textProperties().propertyOrDefault(KoSvgTextProperties::DirectionId).toInt());
    return direction == KoSvgText::DirectionRightToLeft ? wordEnd(pos) : wordStart(pos);
}

int KoSvgTextShape::wordRight(int pos, bool visual)
{
    if (pos < 0 || pos > d->cursorPos.size()-1 || d->result.isEmpty() || d->cursorPos.isEmpty()) {
        return pos;
    }
    if (writingMode() != KoSvgText::HorizontalTB) {
        return posRight(pos, visual);
    }

    KoSvgText::Direction direction = KoSvgText::Direction(this->textProperties().propertyOrDefault(KoSvgTextProperties::DirectionId).toInt());
    if (direction == KoSvgText::DirectionRightToLeft) {
        const int start = wordStart(pos);
        return start < pos ? qMin(start + 1, d->cursorPos.size() - 1) : start;
    }
    return wordEnd(pos);
}
