/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <psd_resource_section.h>

#include <QTest>

#include <type_traits>

class PSDImageResourceSectionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceSectionTypeStateAndIoSignaturesRemainStable();
};

void PSDImageResourceSectionSchemaContractTest::resourceSectionTypeStateAndIoSignaturesRemainStable()
{
    using Section = PSDImageResourceSection;
    using ResourceMap = QMap<Section::PSDResourceID, PSDResourceBlock *>;

    static_assert(std::is_class_v<Section>);
    static_assert(std::is_default_constructible_v<Section>);
    static_assert(std::is_destructible_v<Section>);
    static_assert(std::is_same_v<decltype(&Section::resources), ResourceMap Section::*>);
    static_assert(std::is_same_v<decltype(&Section::error), QString Section::*>);
    static_assert(std::is_same_v<decltype(&Section::read), bool (Section::*)(QIODevice &)>);
    static_assert(std::is_same_v<decltype(&Section::write), bool (Section::*)(QIODevice &)>);
    static_assert(std::is_same_v<decltype(&Section::valid), bool (Section::*)()>);
    static_assert(std::is_same_v<decltype(&Section::idToString), QString (*)(Section::PSDResourceID)>);
}

QTEST_APPLESS_MAIN(PSDImageResourceSectionSchemaContractTest)

#include "PSDImageResourceSectionSchemaContractTest.moc"
