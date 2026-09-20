/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooser_p.h"

#include <utility>

KisResourceItemChooser::KisResourceItemChooser(
    KisResourceUiDescriptor descriptor,
    QWidget *parent)
    : QWidget(parent)
    , d(new Private(std::move(descriptor)))
{
    constructPresentation();
}

KisResourceItemChooser::~KisResourceItemChooser()
{
    disconnect();
    delete d;
}

const KisResourceUiDescriptor &KisResourceItemChooser::descriptor() const
{
    return d->descriptor;
}
