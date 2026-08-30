/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TESTLIBRARYRESOURCEUTILS_H
#define TESTLIBRARYRESOURCEUTILS_H

#include <QObject>

class TestFontLibraryResourceUtils : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nullPointersDoNotInvokeTheDeleter();
    void copyAssignmentAndMoveReleaseAtTheFinalOwner();
    void resetReleasesThePreviousAndCurrentResources();
    void sanityCheckedDeleterReportsOnlyFailures();
    void fontconfigAliasesKeepTheirDestroyFunctions();
    void freeTypeAliasesKeepTheirCheckedDestroyFunctions();
    void harfBuzzAliasesKeepTheirDestroyFunctions();
};

#endif // TESTLIBRARYRESOURCEUTILS_H
