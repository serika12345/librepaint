/*
 * SPDX-FileCopyrightText: 2026 The Krita iPadOS Port Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KRITA_IOS_TOUCH_UI_H
#define KRITA_IOS_TOUCH_UI_H

#include <application/ui/orchestration/KisActionPlugin.h>

#include <memory>

class KisIOSTouchUI final : public KisActionPlugin
{
    Q_OBJECT

public:
    KisIOSTouchUI(QObject *parent, const QVariantList &args);
    ~KisIOSTouchUI() override;

private:
    class Private;
    const std::unique_ptr<Private> d;
};

#endif // KRITA_IOS_TOUCH_UI_H
