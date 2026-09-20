/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISTAGCHOOSERWIDGETCONSTRUCTIONSOURCE_P_H
#define KISTAGCHOOSERWIDGETCONSTRUCTIONSOURCE_P_H

class QAbstractItemModel;
class QObject;
class QString;
class QWidget;
class KisTagChooserWidget;
class KisTagModel;

namespace KisTagChooserWidgetConstructionSource
{
QAbstractItemModel *preparePresentationModel(KisTagModel *model);
QAbstractItemModel *createAllTagsModel(const QString &resourceType,
                                       QObject *parent);
void connectTagToolButton(QWidget *toolButton,
                          KisTagChooserWidget *chooser);
}

#endif // KISTAGCHOOSERWIDGETCONSTRUCTIONSOURCE_P_H
