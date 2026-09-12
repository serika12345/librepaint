/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisCompositeOpListConnectionHelper.h"

#include <QTest>

#include <type_traits>

class KisCompositeOpListConnectionHelperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compositeOpControlConnectionSignatureRemainsStable();
    void blendModeActionConnectionSignatureRemainsStable();
};

void KisCompositeOpListConnectionHelperSchemaContractTest::compositeOpControlConnectionSignatureRemainsStable()
{
    using ConnectControl = void (*)(KisCompositeOpListWidget *, QObject *, const char *);

    static_assert(std::is_same_v<decltype(static_cast<ConnectControl>(&KisWidgetConnectionUtils::connectControl)),
                                 ConnectControl>);
}

void KisCompositeOpListConnectionHelperSchemaContractTest::blendModeActionConnectionSignatureRemainsStable()
{
    using ConnectBlendModeActions = void (*)(KisCompositeOpComboBox *, KisActionManager *);

    static_assert(
        std::is_same_v<decltype(&KisWidgetConnectionUtils::connectBlendModeActions), ConnectBlendModeActions>);
}

QTEST_APPLESS_MAIN(KisCompositeOpListConnectionHelperSchemaContractTest)

#include "KisCompositeOpListConnectionHelperSchemaContractTest.moc"
