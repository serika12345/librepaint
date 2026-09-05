/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPaletteEditor.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using Editor = KisPaletteEditor;

#define ASSERT_PALETTE_EDITOR_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Editor::method)), signature>)
} // namespace

class KisPaletteEditorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paletteEditorTypeLifetimeAndContextSchemaRemainStable();
    void paletteEditorPaletteResourceSignaturesRemainStable();
    void paletteEditorGroupIdentityAndLayoutSignaturesRemainStable();
    void paletteEditorEntryMutationSignaturesRemainStable();
    void paletteEditorEditingLifecycleSignaturesRemainStable();
};

void KisPaletteEditorSchemaContractTest::paletteEditorTypeLifetimeAndContextSchemaRemainStable()
{
    static_assert(std::is_class_v<Editor>);
    static_assert(std::is_base_of_v<QObject, Editor>);
    static_assert(std::is_default_constructible_v<Editor>);
    static_assert(std::is_constructible_v<Editor, QObject *>);
    static_assert(std::has_virtual_destructor_v<Editor>);
    ASSERT_PALETTE_EDITOR_SIGNATURE(clearDocumentContext, void (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(setDocumentContext, void (Editor::*)(QWidget *, const QString &));
    ASSERT_PALETTE_EDITOR_SIGNATURE(setPaletteModel, void (Editor::*)(KisPaletteModel *));
    ASSERT_PALETTE_EDITOR_SIGNATURE(setStorageLocation, void (Editor::*)(QString));
}

void KisPaletteEditorSchemaContractTest::paletteEditorPaletteResourceSignaturesRemainStable()
{
    ASSERT_PALETTE_EDITOR_SIGNATURE(addPalette, KoColorSetSP (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(documentResourceModified, void (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(importPalette, KoColorSetSP (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(removePalette, void (Editor::*)(KoColorSetSP));
    ASSERT_PALETTE_EDITOR_SIGNATURE(saveNewPaletteVersion, void (Editor::*)());
}

void KisPaletteEditorSchemaContractTest::paletteEditorGroupIdentityAndLayoutSignaturesRemainStable()
{
    ASSERT_PALETTE_EDITOR_SIGNATURE(addGroup, QString (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(changeColumnCount, void (Editor::*)(int));
    ASSERT_PALETTE_EDITOR_SIGNATURE(changeGroupRowCount, void (Editor::*)(const QString &, int));
    ASSERT_PALETTE_EDITOR_SIGNATURE(oldNameFromNewName, QString (Editor::*)(const QString &) const);
    ASSERT_PALETTE_EDITOR_SIGNATURE(removeGroup, bool (Editor::*)(const QString &));
    ASSERT_PALETTE_EDITOR_SIGNATURE(rename, void (Editor::*)(const QString &));
    ASSERT_PALETTE_EDITOR_SIGNATURE(renameGroup, QString (Editor::*)(const QString &));
    ASSERT_PALETTE_EDITOR_SIGNATURE(rowCountOfGroup, int (Editor::*)(const QString &) const);
}

void KisPaletteEditorSchemaContractTest::paletteEditorEntryMutationSignaturesRemainStable()
{
    ASSERT_PALETTE_EDITOR_SIGNATURE(removeEntry, void (Editor::*)(const QModelIndex &));
    ASSERT_PALETTE_EDITOR_SIGNATURE(setEntry, void (Editor::*)(const KoColor &, const QModelIndex &));
}

void KisPaletteEditorSchemaContractTest::paletteEditorEditingLifecycleSignaturesRemainStable()
{
    ASSERT_PALETTE_EDITOR_SIGNATURE(clearStagedChanges, void (Editor::*)());
    ASSERT_PALETTE_EDITOR_SIGNATURE(endEditing, void (Editor::*)(bool));
    ASSERT_PALETTE_EDITOR_SIGNATURE(isModified, bool (Editor::*)() const);
    ASSERT_PALETTE_EDITOR_SIGNATURE(startEditing, void (Editor::*)());
    static_assert(std::is_same_v<decltype(std::declval<Editor &>().endEditing()), void>);
}

QTEST_GUILESS_MAIN(KisPaletteEditorSchemaContractTest)

#include "KisPaletteEditorSchemaContractTest.moc"
