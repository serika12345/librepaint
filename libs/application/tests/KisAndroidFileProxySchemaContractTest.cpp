/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "platform-adapters/KisAndroidFileProxy.h"

#include <QTest>

#include <type_traits>

class KisAndroidFileProxySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndContentUriEntryPointRemainStable();
};

void KisAndroidFileProxySchemaContractTest::typeAndContentUriEntryPointRemainStable()
{
    using ContentUriEntryPoint = QString (*)(QString);

    static_assert(std::is_class_v<KisAndroidFileProxy>);
    static_assert(std::is_same_v<decltype(&KisAndroidFileProxy::getFileFromContentUri), ContentUriEntryPoint>);
}

QTEST_GUILESS_MAIN(KisAndroidFileProxySchemaContractTest)

#include "KisAndroidFileProxySchemaContractTest.moc"
