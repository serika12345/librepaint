/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisNodeSelectionRecipe.h"

#include <QTest>

#include <type_traits>

class KisNodeSelectionRecipeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndSelectionModeSchemaRemainStable();
    void constructionSchemaRemainsStable();
    void selectionStateMemberSchemaRemainsStable();
    void processingSelectionSignatureRemainsStable();
};

void KisNodeSelectionRecipeSchemaContractTest::typeAndSelectionModeSchemaRemainStable()
{
    using Recipe = KisNodeSelectionRecipe;

    static_assert(std::is_class_v<Recipe>);
    static_assert(std::is_enum_v<Recipe::SelectionMode>);
    static_assert(Recipe::SelectedLayer == 0);
    static_assert(Recipe::FirstLayer == 1);
    static_assert(Recipe::Group == 2);
}

void KisNodeSelectionRecipeSchemaContractTest::constructionSchemaRemainsStable()
{
    using Recipe = KisNodeSelectionRecipe;

    static_assert(std::is_constructible_v<Recipe, KisNodeList>);
    static_assert(std::is_constructible_v<Recipe, KisNodeList, Recipe::SelectionMode, QPoint>);
    static_assert(std::is_copy_constructible_v<Recipe>);
    static_assert(std::is_constructible_v<Recipe, const Recipe &, int>);
}

void KisNodeSelectionRecipeSchemaContractTest::selectionStateMemberSchemaRemainsStable()
{
    using Recipe = KisNodeSelectionRecipe;

    static_assert(std::is_same_v<decltype(&Recipe::mode), Recipe::SelectionMode Recipe::*>);
    static_assert(std::is_same_v<decltype(&Recipe::pickPoint), QPoint Recipe::*>);
    static_assert(std::is_same_v<decltype(&Recipe::selectedNodes), KisNodeList Recipe::*>);
}

void KisNodeSelectionRecipeSchemaContractTest::processingSelectionSignatureRemainsStable()
{
    using Recipe = KisNodeSelectionRecipe;

    static_assert(std::is_same_v<decltype(&Recipe::selectNodesToProcess), KisNodeList (Recipe::*)() const>);
}

QTEST_APPLESS_MAIN(KisNodeSelectionRecipeSchemaContractTest)

#include "KisNodeSelectionRecipeSchemaContractTest.moc"
