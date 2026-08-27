/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <KisImportExportColorSpaceDialog.h>
#include <KisImportExportUtils.h>
#include <KisImportUserFeedbackInterface.h>
#include <kis_image.h>

namespace
{
int colorSpaceDialogCallCount = 0;

class RecordingFeedback final : public KisImportUserFeedbackInterface
{
public:
    Result askUser(AskCallback) override
    {
        ++callCount;
        return Success;
    }

    int callCount{0};
};
} // namespace

bool KisImportExportColorSpaceDialog::selectEditableColorSpace(QWidget *,
                                                               const KoColorSpace *,
                                                               const KoColorSpace **,
                                                               KoColorConversionTransformation::Intent *,
                                                               KoColorConversionTransformation::ConversionFlags *)
{
    ++colorSpaceDialogCallCount;
    return false;
}

class KisImportExportUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void editableImageNeedsNoFeedbackOrConversion();
};

void KisImportExportUtilsTest::editableImageNeedsNoFeedbackOrConversion()
{
    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(colorSpace);
    QVERIFY(colorSpace->profile());
    QVERIFY(colorSpace->profile()->isSuitableForOutput());

    KisImageSP image = new KisImage(nullptr, 8, 8, colorSpace, "import contract");
    RecordingFeedback feedback;
    KisImageBarrierLock lock(image);
    QVERIFY(lock.owns_lock());
    colorSpaceDialogCallCount = 0;

    const KisImportExportErrorCode result = KritaUtils::workaroundUnsuitableImageColorSpace(image, &feedback, lock);

    QVERIFY(result.isOk());
    QCOMPARE(image->colorSpace(), colorSpace);
    QVERIFY(lock.owns_lock());
    QCOMPARE(feedback.callCount, 0);
    QCOMPARE(colorSpaceDialogCallCount, 0);
}

QTEST_GUILESS_MAIN(KisImportExportUtilsTest)

#include "kis_import_export_utils_test.moc"
