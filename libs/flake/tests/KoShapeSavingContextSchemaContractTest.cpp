/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeSavingContext.h>
#include <svg/SvgSavingContext.h>
#include <svg/SvgWriter.h>

#include <QTest>


namespace
{
using SavedImageMap = QMap<qint64, QString>;



} // namespace

class KoShapeSavingContextSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeSavingOptionValueSchemaRemainsStable();
};

void KoShapeSavingContextSchemaContractTest::shapeSavingOptionValueSchemaRemainsStable()
{
    using Options = KoShapeSavingContext::ShapeSavingOptions;


    QCOMPARE(int(KoShapeSavingContext::PresentationShape), 1);
    QCOMPARE(int(KoShapeSavingContext::DrawId), 2);
    QCOMPARE(int(KoShapeSavingContext::AutoStyleInStyleXml), 4);
    QCOMPARE(int(KoShapeSavingContext::UniqueMasterPages), 8);
    QCOMPARE(int(KoShapeSavingContext::ZIndex), 16);

    const Options combined = KoShapeSavingContext::PresentationShape | KoShapeSavingContext::AutoStyleInStyleXml
        | KoShapeSavingContext::ZIndex;
    QVERIFY(combined.testFlag(KoShapeSavingContext::PresentationShape));
    QVERIFY(combined.testFlag(KoShapeSavingContext::AutoStyleInStyleXml));
    QVERIFY(combined.testFlag(KoShapeSavingContext::ZIndex));
    QVERIFY(!combined.testFlag(KoShapeSavingContext::DrawId));
    QVERIFY(!combined.testFlag(KoShapeSavingContext::UniqueMasterPages));
}

QTEST_APPLESS_MAIN(KoShapeSavingContextSchemaContractTest)

#include "KoShapeSavingContextSchemaContractTest.moc"
