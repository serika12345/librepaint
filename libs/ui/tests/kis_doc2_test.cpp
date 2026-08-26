/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_doc2_test.h"

#include <application/ui/workspace/KisMainWindow.h>

#include <simpletest.h>

#include "document/KisDocument.h"
#include "application/ui/orchestration/KisPart.h"
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/kis_config.h>
#include <testui.h>
#include "KritaTransformMaskStubs.h"


void silenceReignsSupreme(QtMsgType /*type*/, const QMessageLogContext &/*context*/, const QString &/*msg*/)
{
}

void KisDocumentTest::initTestCase()
{
    TestUtil::registerTransformMaskStubs();
    qInstallMessageHandler(silenceReignsSupreme);
}

void KisDocumentTest::testOpenImageTwiceInSameDoc()
{
    QString fname2 = QString(FILES_DATA_DIR) + '/' + "load_test.kra";
    QString fname = QString(FILES_DATA_DIR) + '/' + "load_test2.kra";


    Q_ASSERT(!fname.isEmpty());
    Q_ASSERT(!fname2.isEmpty());

    QScopedPointer<KisDocument> doc(KisPart::instance()->createDocument());

    doc->loadNativeFormat(fname);
    doc->loadNativeFormat(fname2);
}


KISTEST_MAIN(KisDocumentTest)

