/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisColorSamplerStroke.h"

#include <QTest>

#include <type_traits>

class KisColorSamplerStrokeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void activeStateAndLifecycleSignaturesRemainStable();
    void sampleInputSignatureRemainsStable();
    void colorNotificationSignaturesRemainStable();
};

void KisColorSamplerStrokeSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Stroke = KisColorSamplerStroke;

    static_assert(std::is_class_v<Stroke>);
    static_assert(std::is_constructible_v<Stroke, QObject *>);
    static_assert(std::is_destructible_v<Stroke>);
    static_assert(std::has_virtual_destructor_v<Stroke>);
}

void KisColorSamplerStrokeSchemaContractTest::activeStateAndLifecycleSignaturesRemainStable()
{
    using Stroke = KisColorSamplerStroke;

    static_assert(std::is_same_v<decltype(&Stroke::isActive), bool (Stroke::*)() const>);
    static_assert(std::is_same_v<decltype(&Stroke::start), void (Stroke::*)(KisStrokesFacade *, int, int)>);
    static_assert(std::is_same_v<decltype(&Stroke::finish), void (Stroke::*)()>);
}

void KisColorSamplerStrokeSchemaContractTest::sampleInputSignatureRemainsStable()
{
    using Stroke = KisColorSamplerStroke;

    static_assert(std::is_same_v<decltype(&Stroke::addSample),
                                 void (Stroke::*)(const KisPaintDeviceSP &, const QPoint &, const KoColor &)>);
}

void KisColorSamplerStrokeSchemaContractTest::colorNotificationSignaturesRemainStable()
{
    using Stroke = KisColorSamplerStroke;
    using ColorNotification = void (Stroke::*)(const KoColor &);

    static_assert(std::is_same_v<decltype(&Stroke::sigColorUpdated), ColorNotification>);
    static_assert(std::is_same_v<decltype(&Stroke::sigFinalColorSelected), ColorNotification>);
}

QTEST_APPLESS_MAIN(KisColorSamplerStrokeSchemaContractTest)

#include "KisColorSamplerStrokeSchemaContractTest.moc"
