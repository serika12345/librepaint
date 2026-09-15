/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cos/psd_text_data_converter.h"

#include <QTest>

#include <type_traits>

class PsdTextDataConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void psdTextDataConverterSchemaRemainStable();
};

void PsdTextDataConverterSchemaContractTest::psdTextDataConverterSchemaRemainStable()
{
    using Converter = PsdTextDataConverter;

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_default_constructible_v<Converter>);
    static_assert(std::is_destructible_v<Converter>);
    static_assert(std::is_same_v<decltype(&Converter::convertPSDTextEngineDataToSVG),
                                 bool (Converter::*)(QVariantHash,
                                                     QVariantHash,
                                                     const KoColorSpace *,
                                                     int,
                                                     QString *,
                                                     QString *,
                                                     QPointF &,
                                                     bool &,
                                                     bool &,
                                                     QTransform)>);
    static_assert(std::is_same_v<decltype(&Converter::convertToPSDTextEngineData),
                                 bool (Converter::*)(const QString &,
                                                     QRectF &,
                                                     const QList<KoShape *> &,
                                                     QVariantHash &,
                                                     int &,
                                                     QString &,
                                                     bool &,
                                                     QTransform)>);
    static_assert(std::is_same_v<decltype(&Converter::errors), QStringList (Converter::*)() const>);
    static_assert(std::is_same_v<decltype(&Converter::warnings), QStringList (Converter::*)() const>);
}

QTEST_GUILESS_MAIN(PsdTextDataConverterSchemaContractTest)

#include "PsdTextDataConverterSchemaContractTest.moc"
