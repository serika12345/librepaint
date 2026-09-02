/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <text/KoSvgTextContentElement.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KoSvgTextContentElementSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void contentIdentityAndLifecycleSignaturesRemainStable();
    void contentPropertyAndPathValueSchemaRemainsStable();
    void contentLayoutAndDecorationValueSchemaRemainsStable();
    void contentSerializationSignaturesRemainStable();
    void contentQueryAndMutationSignaturesRemainStable();
};

void KoSvgTextContentElementSchemaContractTest::contentIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoSvgTextContentElement>);
    static_assert(std::is_default_constructible_v<KoSvgTextContentElement>);
    static_assert(std::is_copy_constructible_v<KoSvgTextContentElement>);
    static_assert(std::is_destructible_v<KoSvgTextContentElement>);
}

void KoSvgTextContentElementSchemaContractTest::contentPropertyAndPathValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::properties), KoSvgTextProperties>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::localTransformations),
                                 QVector<KoSvgText::CharTransformation>>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::textPathInfo), KoSvgText::TextOnPathInfo>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::textPathId), QString>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::textLength), KoSvgText::AutoValue>);
}

void KoSvgTextContentElementSchemaContractTest::contentLayoutAndDecorationValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::lengthAdjust), KoSvgText::LengthAdjust>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::textDecorations),
                                 QMap<KoSvgText::TextDecoration, QPainterPath>>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::text), QString>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::finalResultIndex), int>);
    static_assert(std::is_same_v<decltype(KoSvgTextContentElement::associatedOutline), QPainterPath>);
}

void KoSvgTextContentElementSchemaContractTest::contentSerializationSignaturesRemainStable()
{
    using LoadSvg = bool (KoSvgTextContentElement::*)(const QDomElement &, SvgLoadingContext &, bool);
    using LoadSvgTextNode = bool (KoSvgTextContentElement::*)(const QDomText &, SvgLoadingContext &);
    using SaveSvg =
        bool (KoSvgTextContentElement::*)(SvgSavingContext &, bool, bool, QMap<QString, QString>, KoShape *);

    static_assert(std::is_same_v<decltype(static_cast<LoadSvg>(&KoSvgTextContentElement::loadSvg)), LoadSvg>);
    static_assert(
        std::is_same_v<decltype(std::declval<KoSvgTextContentElement &>().loadSvg(std::declval<const QDomElement &>(),
                                                                                  std::declval<SvgLoadingContext &>())),
                       bool>);
    static_assert(std::is_same_v<decltype(static_cast<LoadSvgTextNode>(&KoSvgTextContentElement::loadSvgTextNode)),
                                 LoadSvgTextNode>);
    static_assert(std::is_same_v<decltype(static_cast<SaveSvg>(&KoSvgTextContentElement::saveSvg)), SaveSvg>);
    static_assert(std::is_same_v<decltype(std::declval<KoSvgTextContentElement &>().saveSvg(
                                     std::declval<SvgSavingContext &>(),
                                     true,
                                     true,
                                     std::declval<QMap<QString, QString>>())),
                                 bool>);
}

void KoSvgTextContentElementSchemaContractTest::contentQueryAndMutationSignaturesRemainStable()
{
    using NumChars = int (KoSvgTextContentElement::*)(bool, KoSvgTextProperties) const;
    using GetTransformedString =
        QString (KoSvgTextContentElement::*)(QVector<QPair<int, int>> &, KoSvgTextProperties) const;
    using InsertText = void (KoSvgTextContentElement::*)(int, QString);
    using RemoveText = void (KoSvgTextContentElement::*)(int &, int);

    static_assert(std::is_same_v<decltype(static_cast<NumChars>(&KoSvgTextContentElement::numChars)), NumChars>);
    static_assert(std::is_same_v<decltype(std::declval<const KoSvgTextContentElement &>().numChars()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KoSvgTextContentElement &>().numChars(true)), int>);
    static_assert(
        std::is_same_v<decltype(static_cast<GetTransformedString>(&KoSvgTextContentElement::getTransformedString)),
                       GetTransformedString>);
    static_assert(std::is_same_v<decltype(std::declval<const KoSvgTextContentElement &>().getTransformedString(
                                     std::declval<QVector<QPair<int, int>> &>())),
                                 QString>);
    static_assert(std::is_same_v<decltype(static_cast<InsertText>(&KoSvgTextContentElement::insertText)), InsertText>);
    static_assert(std::is_same_v<decltype(static_cast<RemoveText>(&KoSvgTextContentElement::removeText)), RemoveText>);
}

QTEST_GUILESS_MAIN(KoSvgTextContentElementSchemaContractTest)

#include "KoSvgTextContentElementSchemaContractTest.moc"
