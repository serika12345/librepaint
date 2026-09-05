/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/KisSeExprScript.h"

#include <QTest>

#include <type_traits>

namespace
{
using Script = KisSeExprScript;
using ResourceTypePair = QPair<QString, QString>;

#define ASSERT_SEEXPR_SCRIPT_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Script::method)), signature>)
} // namespace

class KisSeExprScriptSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void seExprScriptTypeLifetimeAndAliasSchemaRemainStable();
    void seExprScriptConstructionAndCloneSchemaRemainStable();
    void seExprScriptPersistenceAndClassificationSignaturesRemainStable();
    void seExprScriptIdentityAndExtensionSignaturesRemainStable();
    void seExprScriptContentSignaturesRemainStable();
};

void KisSeExprScriptSchemaContractTest::seExprScriptTypeLifetimeAndAliasSchemaRemainStable()
{
    static_assert(std::is_same_v<KisSeExprScriptSP, QSharedPointer<Script>>);
    static_assert(std::is_class_v<Script>);
    static_assert(std::is_base_of_v<KoResource, Script>);
    static_assert(std::has_virtual_destructor_v<Script>);
}

void KisSeExprScriptSchemaContractTest::seExprScriptConstructionAndCloneSchemaRemainStable()
{
    static_assert(std::is_constructible_v<Script, Script *>);
    static_assert(std::is_copy_constructible_v<Script>);
    static_assert(std::is_constructible_v<Script, const QImage &, const QString &, const QString &, const QString &>);
    static_assert(std::is_constructible_v<Script, const QString &>);
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(clone, KoResourceSP (Script::*)() const);
}

void KisSeExprScriptSchemaContractTest::seExprScriptPersistenceAndClassificationSignaturesRemainStable()
{
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(loadFromDevice, bool (Script::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(resourceType, ResourceTypePair (Script::*)() const);
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(saveToDevice, bool (Script::*)(QIODevice *) const);
}

void KisSeExprScriptSchemaContractTest::seExprScriptIdentityAndExtensionSignaturesRemainStable()
{
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(defaultFileExtension, QString (Script::*)() const);
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(name, QString (Script::*)() const);
}

void KisSeExprScriptSchemaContractTest::seExprScriptContentSignaturesRemainStable()
{
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(script, QString (Script::*)() const);
    ASSERT_SEEXPR_SCRIPT_SIGNATURE(setScript, void (Script::*)(const QString &));
}

QTEST_GUILESS_MAIN(KisSeExprScriptSchemaContractTest)

#include "KisSeExprScriptSchemaContractTest.moc"
