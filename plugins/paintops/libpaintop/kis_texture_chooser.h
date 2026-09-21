/*
  *  SPDX-FileCopyrightText: 2017 Scott Petrovic <scottpetrovic@gmail.com>
  *
  *  SPDX-License-Identifier: GPL-2.0-or-later
  */

#ifndef _KIS_TEXTURE_CHOOSER_H_
#define _KIS_TEXTURE_CHOOSER_H_

#include <QWidget>
#include <QtCore/qtmetamacros.h>

#include "KisTextureOptionData.h"
#include "ui_wdgtexturechooser.h"

class KisTextureChooser : public QWidget, public Ui::KisWdgTextureChooser
{
    Q_OBJECT

public:
    KisTextureChooser(KisBrushTextureFlags flags, QWidget *parent = 0);
    ~KisTextureChooser();

    bool selectTexturingMode(KisTextureOptionData::TexturingMode mode);
    KisTextureOptionData::TexturingMode texturingMode() const;
};

#endif
