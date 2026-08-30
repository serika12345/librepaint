/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMAGE_COMMANDS_TEST_H
#define KIS_IMAGE_COMMANDS_TEST_H

#include <QObject>

class KisImageCommandsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initializingStateRoutesDirtyRequestCalls();
    void finalizingStateRoutesDirtyRequestCalls();
    void publicPartsUseABorrowedFacade();
};

#endif
