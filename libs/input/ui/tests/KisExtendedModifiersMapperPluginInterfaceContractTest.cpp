/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisExtendedModifiersMapperPluginInterface.h>

#include <QPointer>
#include <QTest>

class ExtendedModifiersPlugin final : public KisExtendedModifiersMapperPluginInterface
{
public:
    using KisExtendedModifiersMapperPluginInterface::KisExtendedModifiersMapperPluginInterface;

    ~ExtendedModifiersPlugin() override
    {
        if (destroyed) {
            *destroyed = true;
        }
    }

    ExtendedModifiers queryExtendedModifiers() override
    {
        return modifiers;
    }

    ExtendedModifiers modifiers;
    bool *destroyed {nullptr};
};

class KisExtendedModifiersMapperPluginInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void returnsPluginProvidedModifierSequence();
    void preservesQObjectAndPolymorphicLifetime();
};

void KisExtendedModifiersMapperPluginInterfaceContractTest::returnsPluginProvidedModifierSequence()
{
    ExtendedModifiersPlugin plugin;
    plugin.modifiers = {Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt};
    KisExtendedModifiersMapperPluginInterface *interface = &plugin;

    QCOMPARE(interface->queryExtendedModifiers(), plugin.modifiers);
}

void KisExtendedModifiersMapperPluginInterfaceContractTest::preservesQObjectAndPolymorphicLifetime()
{
    QObject parent;
    bool destroyed = false;
    auto *plugin = new ExtendedModifiersPlugin(&parent);
    plugin->destroyed = &destroyed;
    QPointer<KisExtendedModifiersMapperPluginInterface> guard(plugin);
    KisExtendedModifiersMapperPluginInterface *interface = plugin;

    QCOMPARE(interface->parent(), &parent);

    delete interface;

    QVERIFY(destroyed);
    QVERIFY(guard.isNull());
}

QTEST_GUILESS_MAIN(KisExtendedModifiersMapperPluginInterfaceContractTest)

#include "KisExtendedModifiersMapperPluginInterfaceContractTest.moc"
