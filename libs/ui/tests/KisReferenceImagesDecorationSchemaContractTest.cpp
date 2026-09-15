/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisReferenceImagesDecoration.h"

#include <QTest>

#include <type_traits>

class KisReferenceImagesDecorationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void referenceImagesDecorationSchemaRemainStable();
};

void KisReferenceImagesDecorationSchemaContractTest::referenceImagesDecorationSchemaRemainStable()
{
    using Decoration = KisReferenceImagesDecoration;

    static_assert(std::is_same_v<KisReferenceImagesDecorationSP, KisSharedPtr<Decoration>>);
    static_assert(std::is_class_v<Decoration>);
    static_assert(std::is_base_of_v<KisCanvasDecoration, Decoration>);
    static_assert(std::is_constructible_v<Decoration, QPointer<KisView>, KisDocument *>);
    static_assert(std::has_virtual_destructor_v<Decoration>);
    static_assert(std::is_same_v<decltype(&Decoration::addReferenceImage), void (Decoration::*)(KisReferenceImage *)>);
    static_assert(std::is_same_v<decltype(&Decoration::documentHasReferenceImages), bool (Decoration::*)() const>);
}

QTEST_GUILESS_MAIN(KisReferenceImagesDecorationSchemaContractTest)

#include "KisReferenceImagesDecorationSchemaContractTest.moc"
