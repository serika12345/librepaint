/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisExtendedModifiersMapperPluginInterface.h>
#include <kis_extended_modifiers_mapper.h>

#include <QApplication>
#include <QKeyEvent>
#include <QTest>

class FixedExtendedModifiersPlugin final : public KisExtendedModifiersMapperPluginInterface
{
public:
    ExtendedModifiers queryExtendedModifiers() override
    {
        ++queryCount;
        return modifiers;
    }

    ExtendedModifiers modifiers;
    int queryCount {0};
};

class KisExtendedModifiersMapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void convertsStandardModifiersInStableOrder();
    void appliesShiftMetaCompatibilityRule();
    void delegatesExtendedModifiersWithoutOwningPlugin();
    void reportsApplicationModifierState();
#ifdef Q_OS_MACOS
    void keepsPluginQueriesStableAcrossLocalMonitorChanges();
#endif
};

void KisExtendedModifiersMapperContractTest::init()
{
    KisExtendedModifiersMapper::setPluginInterface(nullptr);
}

void KisExtendedModifiersMapperContractTest::cleanup()
{
    KisExtendedModifiersMapper::setPluginInterface(nullptr);
#ifdef Q_OS_MACOS
    KisExtendedModifiersMapper::setLocalMonitor(false);
#endif
}

void KisExtendedModifiersMapperContractTest::convertsStandardModifiersInStableOrder()
{
    using Mapper = KisExtendedModifiersMapper;

    QCOMPARE(Mapper::qtModifiersToQtKeys(Qt::NoModifier), Mapper::ExtendedModifiers());
    QCOMPARE(
        Mapper::qtModifiersToQtKeys(
            Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier),
        Mapper::ExtendedModifiers({Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt, Qt::Key_Meta}));
}

void KisExtendedModifiersMapperContractTest::appliesShiftMetaCompatibilityRule()
{
    QKeyEvent shiftedMeta(
        QEvent::KeyPress,
        Qt::Key_Meta,
        Qt::ShiftModifier | Qt::MetaModifier);
    QKeyEvent plainMeta(QEvent::KeyPress, Qt::Key_Meta, Qt::MetaModifier);
    QKeyEvent shiftedControl(
        QEvent::KeyPress,
        Qt::Key_Control,
        Qt::ShiftModifier | Qt::ControlModifier);

    QCOMPARE(KisExtendedModifiersMapper::workaroundShiftAltMetaHell(&shiftedMeta), Qt::Key_Alt);
    QCOMPARE(KisExtendedModifiersMapper::workaroundShiftAltMetaHell(&plainMeta), Qt::Key_Meta);
    QCOMPARE(KisExtendedModifiersMapper::workaroundShiftAltMetaHell(&shiftedControl), Qt::Key_Control);
}

void KisExtendedModifiersMapperContractTest::delegatesExtendedModifiersWithoutOwningPlugin()
{
    FixedExtendedModifiersPlugin plugin;
    plugin.modifiers = {Qt::Key_Space, Qt::Key_R, Qt::Key_Shift};
    KisExtendedModifiersMapper::setPluginInterface(&plugin);

    {
        KisExtendedModifiersMapper mapper;
        QCOMPARE(mapper.queryExtendedModifiers(), plugin.modifiers);
        QCOMPARE(plugin.queryCount, 1);
    }

    QCOMPARE(plugin.queryCount, 1);
}

void KisExtendedModifiersMapperContractTest::reportsApplicationModifierState()
{
    KisExtendedModifiersMapper mapper;

    QCOMPARE(mapper.queryStandardModifiers(), QApplication::queryKeyboardModifiers());
}

#ifdef Q_OS_MACOS
void KisExtendedModifiersMapperContractTest::keepsPluginQueriesStableAcrossLocalMonitorChanges()
{
    FixedExtendedModifiersPlugin plugin;
    plugin.modifiers = {Qt::Key_V, Qt::Key_Alt};
    KisExtendedModifiersMapper::setPluginInterface(&plugin);
    KisExtendedModifiersMapper mapper;

    KisExtendedModifiersMapper::setLocalMonitor(true);
    QCOMPARE(mapper.queryExtendedModifiers(), plugin.modifiers);

    KisExtendedModifiersMapper::setLocalMonitor(false);
    QCOMPARE(mapper.queryExtendedModifiers(), plugin.modifiers);
    QCOMPARE(plugin.queryCount, 2);
}
#endif

QTEST_MAIN(KisExtendedModifiersMapperContractTest)

#include "KisExtendedModifiersMapperContractTest.moc"
