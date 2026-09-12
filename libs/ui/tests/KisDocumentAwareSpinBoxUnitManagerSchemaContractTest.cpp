/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <utils/kis_document_aware_spin_box_unit_manager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_DOCUMENT_AWARE_UNIT_MANAGER_SIGNATURE(method, signature)                                                \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KisDocumentAwareSpinBoxUnitManager::method)), signature>)
} // namespace

class KisDocumentAwareSpinBoxUnitManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentAwareUnitManagerTypeAndDirectionSchemaRemainStable();
    void unitManagerSetupSchemaRemainStable();
    void unitManagerBuilderAndConversionSchemaRemainStable();
};

void KisDocumentAwareSpinBoxUnitManagerSchemaContractTest::documentAwareUnitManagerTypeAndDirectionSchemaRemainStable()
{
    static_assert(std::is_base_of_v<KisSpinBoxUnitManagerBuilder, KisDocumentAwareSpinBoxUnitManagerBuilder>);
    static_assert(std::is_base_of_v<KisSpinBoxUnitManager, KisDocumentAwareSpinBoxUnitManager>);
    static_assert(std::has_virtual_destructor_v<KisDocumentAwareSpinBoxUnitManager>);
    static_assert(std::is_enum_v<KisDocumentAwareSpinBoxUnitManager::PixDir>);
    static_assert(KisDocumentAwareSpinBoxUnitManager::PIX_DIR_X == 0);
    static_assert(KisDocumentAwareSpinBoxUnitManager::PIX_DIR_Y == 1);

    QVERIFY(true);
}

void KisDocumentAwareSpinBoxUnitManagerSchemaContractTest::unitManagerSetupSchemaRemainStable()
{
    static_assert(std::is_constructible_v<KisDocumentAwareSpinBoxUnitManager, QObject *>);
    static_assert(std::is_constructible_v<KisDocumentAwareSpinBoxUnitManager, QObject *, int>);
    static_assert(
        std::is_same_v<decltype(&KisDocumentAwareSpinBoxUnitManager::setDocumentAwarenessToExistingUnitSpinBox),
                       void (*)(KisDoubleParseUnitSpinBox *, bool)>);
    static_assert(std::is_same_v<decltype(&KisDocumentAwareSpinBoxUnitManager::createUnitSpinBoxWithDocumentAwareness),
                                 KisDoubleParseUnitSpinBox *(*)(QWidget *)>);

    QVERIFY(true);
}

void KisDocumentAwareSpinBoxUnitManagerSchemaContractTest::unitManagerBuilderAndConversionSchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisDocumentAwareSpinBoxUnitManagerBuilder::buildUnitManager),
                                 KisSpinBoxUnitManager *(KisDocumentAwareSpinBoxUnitManagerBuilder::*)(QObject *)>);
    ASSERT_DOCUMENT_AWARE_UNIT_MANAGER_SIGNATURE(getConversionFactor,
                                                 qreal (KisDocumentAwareSpinBoxUnitManager::*)(int, QString) const);
    ASSERT_DOCUMENT_AWARE_UNIT_MANAGER_SIGNATURE(getConversionConstant,
                                                 qreal (KisDocumentAwareSpinBoxUnitManager::*)(int, QString) const);

    QVERIFY(true);
}

#undef ASSERT_DOCUMENT_AWARE_UNIT_MANAGER_SIGNATURE

QTEST_APPLESS_MAIN(KisDocumentAwareSpinBoxUnitManagerSchemaContractTest)

#include "KisDocumentAwareSpinBoxUnitManagerSchemaContractTest.moc"
