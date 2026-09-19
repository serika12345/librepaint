/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeLoadingContext.h>

#include <QTest>
#include <QVariant>

#include <utility>

namespace
{
using ShapeSubItem = QPair<KoShape *, QVariant>;

} // namespace

class KoShapeLoadingContextSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void additionalAttributeValueSchemaRemainsStable();
};

void KoShapeLoadingContextSchemaContractTest::additionalAttributeValueSchemaRemainsStable()
{
    using Attribute = KoShapeLoadingContext::AdditionalAttributeData;


    QString sourceNamespace = QStringLiteral("urn:librepaint:first");
    QString sourceTag = QStringLiteral("first-tag");
    QString sourceName = QStringLiteral("shared-name");
    const Attribute original(sourceNamespace, sourceTag, sourceName);
    const Attribute copied(original);

    sourceNamespace = QStringLiteral("urn:librepaint:changed");
    sourceTag = QStringLiteral("changed-tag");
    sourceName = QStringLiteral("changed-name");

    QCOMPARE(original.ns, QStringLiteral("urn:librepaint:first"));
    QCOMPARE(original.tag, QStringLiteral("first-tag"));
    QCOMPARE(original.name, QStringLiteral("shared-name"));
    QCOMPARE(copied.ns, original.ns);
    QCOMPARE(copied.tag, original.tag);
    QCOMPARE(copied.name, original.name);

    const Attribute sameNameDifferentContext(QStringLiteral("urn:librepaint:second"),
                                             QStringLiteral("second-tag"),
                                             QStringLiteral("shared-name"));
    const Attribute differentName(original.ns, original.tag, QStringLiteral("different-name"));
    QVERIFY(original == sameNameDifferentContext);
    QVERIFY(!(original == differentName));
}

QTEST_APPLESS_MAIN(KoShapeLoadingContextSchemaContractTest)

#include "KoShapeLoadingContextSchemaContractTest.moc"
