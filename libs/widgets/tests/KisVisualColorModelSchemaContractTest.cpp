/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisVisualColorModel.h"

#include <QTest>

#include <type_traits>

class KisVisualColorModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visualColorModelTypeAndEnumerationSchemaRemainsStable();
    void visualColorModelLifetimeAndStateSchemaRemainsStable();
    void visualColorModelPolicySchemaRemainsStable();
    void visualColorModelConversionAndMutationSchemaRemainsStable();
    void visualColorModelNotificationSchemaRemainsStable();
};

void KisVisualColorModelSchemaContractTest::visualColorModelTypeAndEnumerationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_same_v<KisVisualColorModelSP, QSharedPointer<Model>>);
    static_assert(std::is_enum_v<Model::ColorModel>);
    static_assert(int(Model::None) == 0);
    static_assert(int(Model::Channel) == 1);
    static_assert(int(Model::HSV) == 2);
    static_assert(int(Model::HSL) == 3);
    static_assert(int(Model::HSI) == 4);
    static_assert(int(Model::HSY) == 5);
    static_assert(int(Model::YUV) == 6);
}

void KisVisualColorModelSchemaContractTest::visualColorModelLifetimeAndStateSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_default_constructible_v<Model>);
    static_assert(std::is_destructible_v<Model>);
    static_assert(std::is_same_v<decltype(&Model::currentColor), KoColor (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::channelValues), QVector4D (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::colorChannelCount), int (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::colorModel), Model::ColorModel (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::maxChannelValues), QVector4D (Model::*)() const>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelPolicySchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_same_v<decltype(&Model::setMaxChannelValues), void (Model::*)(const QVector4D &)>);
    static_assert(std::is_same_v<decltype(&Model::copyState), void (Model::*)(const Model &)>);
    static_assert(std::is_same_v<decltype(&Model::setRGBColorModel), void (Model::*)(Model::ColorModel)>);
    static_assert(std::is_same_v<decltype(&Model::colorSpace), const KoColorSpace *(Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::isHSXModel), bool (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::supportsExposure), bool (Model::*)() const>);
    static_assert(std::is_same_v<decltype(&Model::slotLoadACSConfig), void (Model::*)()>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelConversionAndMutationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(
        std::is_same_v<decltype(&Model::convertChannelValuesToKoColor), KoColor (Model::*)(const QVector4D &) const>);
    static_assert(std::is_same_v<decltype(&Model::convertKoColorToChannelValues), QVector4D (Model::*)(KoColor) const>);
    static_assert(std::is_same_v<decltype(&Model::slotSetColor), void (Model::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Model::slotSetColorSpace), void (Model::*)(const KoColorSpace *)>);
    static_assert(std::is_same_v<decltype(&Model::slotSetChannelValues), void (Model::*)(const QVector4D &)>);
}

void KisVisualColorModelSchemaContractTest::visualColorModelNotificationSchemaRemainsStable()
{
    using Model = KisVisualColorModel;

    static_assert(std::is_same_v<decltype(&Model::sigColorModelChanged), void (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::sigColorSpaceChanged), void (Model::*)()>);
    static_assert(
        std::is_same_v<decltype(&Model::sigChannelValuesChanged), void (Model::*)(const QVector4D &, quint32)>);
    static_assert(std::is_same_v<decltype(&Model::sigNewColor), void (Model::*)(const KoColor &)>);
}

QTEST_GUILESS_MAIN(KisVisualColorModelSchemaContractTest)

#include "KisVisualColorModelSchemaContractTest.moc"
