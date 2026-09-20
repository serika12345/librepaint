/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_entry_editor.h"

#include <QLineEdit>
#include <QTest>

#include <kis_meta_data_entry.h>
#include <kis_meta_data_schema.h>
#include <kis_meta_data_schema_registry.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_value.h>

namespace
{

KisMetaData::Value textValue(const QString &text)
{
    return KisMetaData::Value(QVariant(text));
}

} // namespace

class KisEntryEditorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void editingStructureFieldPreservesTheOtherMetadataFields();
};

// Consumer: The Metadata Editor's structure-field widgets.
// Operation: Change one text field in a structured metadata entry.
// Observable result: The edited field changes while the entry remains a structure and its other fields retain their values.
// Failure impact: Editing a caption or contact field can replace the whole metadata record and discard unrelated user-entered data.
void KisEntryEditorContractTest::editingStructureFieldPreservesTheOtherMetadataFields()
{
    const KisMetaData::Schema *schema = KisMetaData::SchemaRegistry::instance()->create(
        QStringLiteral("https://librepaint.org/tests/metadata-editor/"), QStringLiteral("test"));
    QVERIFY(schema);

    KisMetaData::Store store;
    const KisMetaData::Entry entry(schema,
                                   QStringLiteral("contact"),
                                   KisMetaData::Value(QMap<QString, KisMetaData::Value>{
                                       {QStringLiteral("caption"), textValue(QStringLiteral("before"))},
                                       {QStringLiteral("credit"), textValue(QStringLiteral("artist"))},
                                   }));
    QVERIFY(store.addEntry(entry));

    QLineEdit widget;
    KisEntryEditor editor(&widget,
                           &store,
                           entry.qualifiedName(),
                           QStringLiteral("text"),
                           QStringLiteral("caption"),
                           0);
    QCOMPARE(widget.text(), QStringLiteral("before"));

    widget.setText(QStringLiteral("after"));
    editor.valueEdited();

    const KisMetaData::Value &saved = store.getEntry(entry.qualifiedName()).value();
    QCOMPARE(saved.type(), KisMetaData::Value::Structure);
    const QMap<QString, KisMetaData::Value> fields = saved.asStructure();
    QCOMPARE(fields.value(QStringLiteral("caption")).asVariant(), QVariant(QStringLiteral("after")));
    QCOMPARE(fields.value(QStringLiteral("credit")).asVariant(), QVariant(QStringLiteral("artist")));
}

QTEST_MAIN(KisEntryEditorContractTest)

#include "KisEntryEditorContractTest.moc"
