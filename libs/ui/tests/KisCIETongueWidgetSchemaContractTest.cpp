/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/kis_cie_tongue_widget.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisCIETongueWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cieTongueWidgetTypeLifetimeAndModelSchemaRemainStable();
    void cieTongueWidgetProfileAndGamutSignaturesRemainStable();
    void cieTongueWidgetRgbCmykAndXyzDataSignaturesRemainStable();
    void cieTongueWidgetGrayLabAndYCbCrDataSignaturesRemainStable();
    void cieTongueWidgetLegacyStatusDeclarationSignaturesRemainStable();
};

void KisCIETongueWidgetSchemaContractTest::cieTongueWidgetTypeLifetimeAndModelSchemaRemainStable()
{
    using Widget = KisCIETongueWidget;

    static_assert(std::is_class_v<Widget>);
    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *>);
    static_assert(std::is_default_constructible_v<Widget>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(std::is_enum_v<Widget::model>);
    static_assert(Widget::RGBA == 0);
    static_assert(Widget::CMYKA == 1);
    static_assert(Widget::XYZA == 2);
    static_assert(Widget::LABA == 3);
    static_assert(Widget::GRAYA == 4);
    static_assert(Widget::YCbCrA == 5);
}

void KisCIETongueWidgetSchemaContractTest::cieTongueWidgetProfileAndGamutSignaturesRemainStable()
{
    using Widget = KisCIETongueWidget;
    using Chromaticity = KoColorimetryUtils::xyY;
    using SetProfileDataSignature = void (Widget::*)(QVector<Chromaticity>, Chromaticity, bool);
    using SetGamutSignature = void (Widget::*)(QPolygonF);
    using SetProfileDataAvailableSignature = void (Widget::*)(bool);

    static_assert(std::is_same_v<decltype(static_cast<SetProfileDataSignature>(&Widget::setProfileData)),
                                 SetProfileDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetGamutSignature>(&Widget::setGamut)), SetGamutSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetProfileDataAvailableSignature>(&Widget::setProfileDataAvailable)),
                       SetProfileDataAvailableSignature>);
    static_assert(std::is_same_v<decltype(std::declval<Widget &>().setProfileData(std::declval<QVector<Chromaticity>>(),
                                                                                  std::declval<Chromaticity>())),
                                 void>);
}

void KisCIETongueWidgetSchemaContractTest::cieTongueWidgetRgbCmykAndXyzDataSignaturesRemainStable()
{
    using Widget = KisCIETongueWidget;
    using Chromaticity = KoColorimetryUtils::xyY;
    using SetDataSignature = void (Widget::*)(Chromaticity);
    using SetRgbDataSignature = void (Widget::*)(Chromaticity, QVector<Chromaticity>);

    static_assert(std::is_same_v<decltype(static_cast<SetDataSignature>(&Widget::setCMYKData)), SetDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetRgbDataSignature>(&Widget::setRGBData)), SetRgbDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetDataSignature>(&Widget::setXYZData)), SetDataSignature>);
}

void KisCIETongueWidgetSchemaContractTest::cieTongueWidgetGrayLabAndYCbCrDataSignaturesRemainStable()
{
    using Widget = KisCIETongueWidget;
    using Chromaticity = KoColorimetryUtils::xyY;
    using SetDataSignature = void (Widget::*)(Chromaticity);

    static_assert(std::is_same_v<decltype(static_cast<SetDataSignature>(&Widget::setGrayData)), SetDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetDataSignature>(&Widget::setLABData)), SetDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetDataSignature>(&Widget::setYCbCrData)), SetDataSignature>);
}

void KisCIETongueWidgetSchemaContractTest::cieTongueWidgetLegacyStatusDeclarationSignaturesRemainStable()
{
    using Widget = KisCIETongueWidget;
    using StatusDeclarationSignature = void (Widget::*)();

    static_assert(std::is_same_v<decltype(static_cast<StatusDeclarationSignature>(&Widget::loadingFailed)),
                                 StatusDeclarationSignature>);
    static_assert(std::is_same_v<decltype(static_cast<StatusDeclarationSignature>(&Widget::loadingStarted)),
                                 StatusDeclarationSignature>);
    static_assert(std::is_same_v<decltype(static_cast<StatusDeclarationSignature>(&Widget::uncalibratedColor)),
                                 StatusDeclarationSignature>);
}

QTEST_GUILESS_MAIN(KisCIETongueWidgetSchemaContractTest)

#include "KisCIETongueWidgetSchemaContractTest.moc"
