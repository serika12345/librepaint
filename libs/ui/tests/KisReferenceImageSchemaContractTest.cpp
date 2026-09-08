/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/KisReferenceImage.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_REFERENCE_IMAGE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisReferenceImage::method)), signature>)
#define ASSERT_SATURATION_COMMAND_SIGNATURE(method, signature)                                                         \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KisReferenceImage::SetSaturationCommand::method)), signature>)
} // namespace

class KisReferenceImageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void referenceImageTypeLifetimeAndCloneSchemaRemainStable();
    void referenceImageFactorySignaturesRemainStable();
    void referenceImagePresentationAndSaturationSignaturesRemainStable();
    void referenceImagePersistenceAndIdentitySignaturesRemainStable();
    void saturationCommandSchemaRemainStable();
};

void KisReferenceImageSchemaContractTest::referenceImageTypeLifetimeAndCloneSchemaRemainStable()
{
    using Image = KisReferenceImage;

    static_assert(std::is_class_v<Image>);
    static_assert(std::is_default_constructible_v<Image>);
    static_assert(std::is_copy_constructible_v<Image>);
    static_assert(std::has_virtual_destructor_v<Image>);
    ASSERT_REFERENCE_IMAGE_SIGNATURE(cloneShape, KoShape * (Image::*)() const);
}

void KisReferenceImageSchemaContractTest::referenceImageFactorySignaturesRemainStable()
{
    using Image = KisReferenceImage;

    ASSERT_REFERENCE_IMAGE_SIGNATURE(fromFile,
                                     Image * (*)(const QString &, const KisCoordinatesConverter &, QWidget *));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(fromClipboard, Image * (*)(const KisCoordinatesConverter &));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(fromQImage, Image * (*)(const KisCoordinatesConverter &, const QImage &));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(fromPaintDevice,
                                     Image * (*)(KisPaintDeviceSP, const KisCoordinatesConverter &, QWidget *));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(fromXml, Image * (*)(const QDomElement &));
}

void KisReferenceImageSchemaContractTest::referenceImagePresentationAndSaturationSignaturesRemainStable()
{
    using Image = KisReferenceImage;

    ASSERT_REFERENCE_IMAGE_SIGNATURE(paint, void (Image::*)(QPainter &) const);
    ASSERT_REFERENCE_IMAGE_SIGNATURE(getPixel, KoColor (Image::*)(QPointF));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(getImage, QImage (Image::*)());
    ASSERT_REFERENCE_IMAGE_SIGNATURE(setSaturation, void (Image::*)(qreal));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(saturation, qreal (Image::*)() const);
}

void KisReferenceImageSchemaContractTest::referenceImagePersistenceAndIdentitySignaturesRemainStable()
{
    using Image = KisReferenceImage;

    ASSERT_REFERENCE_IMAGE_SIGNATURE(setEmbed, void (Image::*)(bool));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(embed, bool (Image::*)());
    ASSERT_REFERENCE_IMAGE_SIGNATURE(hasLocalFile, bool (Image::*)());
    ASSERT_REFERENCE_IMAGE_SIGNATURE(setFilename, void (Image::*)(const QString &));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(filename, QString (Image::*)() const);
    ASSERT_REFERENCE_IMAGE_SIGNATURE(internalFile, QString (Image::*)() const);
    ASSERT_REFERENCE_IMAGE_SIGNATURE(saveXml, void (Image::*)(QDomDocument &, QDomElement &, int));
    ASSERT_REFERENCE_IMAGE_SIGNATURE(saveImage, bool (Image::*)(KoStore *) const);
    ASSERT_REFERENCE_IMAGE_SIGNATURE(loadImage, bool (Image::*)(KoStore *));
}

void KisReferenceImageSchemaContractTest::saturationCommandSchemaRemainStable()
{
    using Image = KisReferenceImage;
    using Command = Image::SetSaturationCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_same_v<decltype(Command::images), QVector<Image *>>);
    static_assert(std::is_same_v<decltype(Command::oldSaturations), QVector<qreal>>);
    static_assert(std::is_same_v<decltype(Command::newSaturation), qreal>);
    static_assert(std::is_constructible_v<Command, const QList<KoShape *> &, qreal>);
    static_assert(std::is_constructible_v<Command, const QList<KoShape *> &, qreal, KUndo2Command *>);
    ASSERT_SATURATION_COMMAND_SIGNATURE(undo, void (Command::*)());
    ASSERT_SATURATION_COMMAND_SIGNATURE(redo, void (Command::*)());
}

#undef ASSERT_REFERENCE_IMAGE_SIGNATURE
#undef ASSERT_SATURATION_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisReferenceImageSchemaContractTest)

#include "KisReferenceImageSchemaContractTest.moc"
