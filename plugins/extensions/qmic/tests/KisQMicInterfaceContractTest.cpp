/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../kis_qmic_interface.h"

#include <QTest>

#include <type_traits>

class KisQMicInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageTypeConstructionAndOwnershipRemainStable();
    void imageStorageValuesRemainStable();
    void imageBufferAndDiagnosticSignaturesRemainStable();
    void interfaceTypeConstructionAndLifetimeSchemaRemainStable();
    void interfaceExchangeSignaturesRemainStable();
};

void KisQMicInterfaceContractTest::imageTypeConstructionAndOwnershipRemainStable()
{
    static_assert(std::is_class_v<KisQMicImage>);
    static_assert(std::is_same_v<KisQMicImageSP, QSharedPointer<KisQMicImage>>);
    static_assert(std::is_constructible_v<KisQMicImage, QString, int, int, int>);
    static_assert(std::is_destructible_v<KisQMicImage>);
    static_assert(!std::is_copy_constructible_v<KisQMicImage>);
}

void KisQMicInterfaceContractTest::imageStorageValuesRemainStable()
{
    using Image = KisQMicImage;
    Image image(QStringLiteral("Layer A"), 2, 3);

    static_assert(std::is_same_v<decltype(&Image::m_mutex), QMutex Image::*>);
    static_assert(std::is_same_v<decltype(&Image::m_layerName), QString Image::*>);
    static_assert(std::is_same_v<decltype(&Image::m_width), int Image::*>);
    static_assert(std::is_same_v<decltype(&Image::m_height), int Image::*>);
    static_assert(std::is_same_v<decltype(&Image::m_spectrum), int Image::*>);
    static_assert(std::is_same_v<decltype(&Image::m_data), float *Image::*>);
    QCOMPARE(image.m_layerName, QStringLiteral("Layer A"));
    QCOMPARE(image.m_width, 2);
    QCOMPARE(image.m_height, 3);
    QCOMPARE(image.m_spectrum, 4);
    QVERIFY(image.m_data);
}

void KisQMicInterfaceContractTest::imageBufferAndDiagnosticSignaturesRemainStable()
{
    KisQMicImage image(QStringLiteral("RGBA"), 2, 3, 4);
    image.m_data[0] = 0.25F;

    static_assert(std::is_same_v<decltype(&KisQMicImage::constData), const float *(KisQMicImage::*)() const>);
    static_assert(std::is_same_v<decltype(&KisQMicImage::size), size_t (KisQMicImage::*)() const>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const KisQMicImage &)>(&operator<<)),
                                 QDebug (*)(QDebug, const KisQMicImage &)>);
    QCOMPARE(image.constData(), image.m_data);
    QCOMPARE(image.constData()[0], 0.25F);
    QCOMPARE(image.size(), size_t(2 * 3 * 4 * sizeof(float)));
}

void KisQMicInterfaceContractTest::interfaceTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Interface = KisImageInterface;

    static_assert(std::is_class_v<Interface>);
    static_assert(std::is_base_of_v<QObject, Interface>);
    static_assert(std::is_constructible_v<Interface, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<Interface>);
}

void KisQMicInterfaceContractTest::interfaceExchangeSignaturesRemainStable()
{
    using Interface = KisImageInterface;

    static_assert(std::is_same_v<decltype(&Interface::gmic_qt_get_image_size), QSize (Interface::*)(int)>);
    static_assert(std::is_same_v<decltype(&Interface::gmic_qt_get_cropped_images),
                                 QVector<KisQMicImageSP> (Interface::*)(int, QRectF &)>);
    static_assert(
        std::is_same_v<decltype(&Interface::gmic_qt_output_images), void (Interface::*)(int, QVector<KisQMicImageSP>)>);
    static_assert(std::is_same_v<decltype(&Interface::gmic_qt_detach), void (Interface::*)()>);
}

QTEST_GUILESS_MAIN(KisQMicInterfaceContractTest)

#include "KisQMicInterfaceContractTest.moc"
