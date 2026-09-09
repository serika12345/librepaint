/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_utils.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisToolUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSamplerTypeAndConstructionSchemaRemainStable();
    void colorSamplerBooleanValueSchemaRemainsStable();
    void colorSamplerNumericAndPersistenceSignaturesRemainStable();
    void toolUtilityFunctionSignaturesRemainStable();
};

void KisToolUtilsSchemaContractTest::colorSamplerTypeAndConstructionSchemaRemainStable()
{
    using Config = KisToolUtils::ColorSamplerConfig;

    static_assert(std::is_class_v<Config>);
    static_assert(std::is_default_constructible_v<Config>);
}

void KisToolUtilsSchemaContractTest::colorSamplerBooleanValueSchemaRemainsStable()
{
    using Config = KisToolUtils::ColorSamplerConfig;
    using BooleanMember = bool Config::*;

    static_assert(std::is_same_v<decltype(&Config::toForegroundColor), BooleanMember>);
    static_assert(std::is_same_v<decltype(&Config::updateColor), BooleanMember>);
    static_assert(std::is_same_v<decltype(&Config::addColorToCurrentPalette), BooleanMember>);
    static_assert(std::is_same_v<decltype(&Config::normaliseValues), BooleanMember>);
    static_assert(std::is_same_v<decltype(&Config::sampleMerged), BooleanMember>);
}

void KisToolUtilsSchemaContractTest::colorSamplerNumericAndPersistenceSignaturesRemainStable()
{
    using Config = KisToolUtils::ColorSamplerConfig;
    using IntegerMember = int Config::*;

    static_assert(std::is_same_v<decltype(&Config::radius), IntegerMember>);
    static_assert(std::is_same_v<decltype(&Config::blend), IntegerMember>);
    static_assert(std::is_same_v<decltype(&Config::save), void (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::load), void (Config::*)()>);
}

void KisToolUtilsSchemaContractTest::toolUtilityFunctionSignaturesRemainStable()
{
    using EditableMessage = QString (*)(KisNodeSP, bool);
    using ClearImage = bool (*)(KisImageSP, KisNodeList, KisSelectionSP);
    using SetCursorPosition = void (*)(const QPoint &);

    static_assert(
        std::is_same_v<decltype(static_cast<EditableMessage>(&KisToolUtils::nodeEditableMessage)), EditableMessage>);
    static_assert(std::is_same_v<decltype(static_cast<ClearImage>(&KisToolUtils::clearImage)), ClearImage>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetCursorPosition>(&KisToolUtils::setCursorPos)), SetCursorPosition>);
    static_assert(std::is_same_v<decltype(KisToolUtils::nodeEditableMessage(std::declval<KisNodeSP>())), QString>);
}

QTEST_GUILESS_MAIN(KisToolUtilsSchemaContractTest)

#include "KisToolUtilsSchemaContractTest.moc"
