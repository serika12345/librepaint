/*
    This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>
    SPDX-FileCopyrightText: 2002 Werner Trobin <trobin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KisImportExportFilter_p.h"

const QString KisImportExportFilter::ImageContainsTransparencyTag = "ImageContainsTransparency";
const QString KisImportExportFilter::ColorModelIDTag = "ColorModelID";
const QString KisImportExportFilter::ColorDepthIDTag = "ColorDepthID";
const QString KisImportExportFilter::sRGBTag = "sRGB";
const QString KisImportExportFilter::CICPPrimariesTag = "CICPCompatiblePrimaries";
const QString KisImportExportFilter::CICPTransferCharacteristicsTag = "CICPCompatibleTransferFunction";
const QString KisImportExportFilter::HDRTag = "HDRSupported";

KisImportExportFilter::Private::Private()
    : updater(nullptr)
    , batchmode(false)
{
}

KisImportExportFilter::Private::~Private()
{
    qDeleteAll(capabilities);
}

KisImportExportFilter::KisImportExportFilter(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

QString KisImportExportFilter::filename() const
{
    return d->filename;
}

QString KisImportExportFilter::realFilename() const
{
    return d->realFilename;
}

bool KisImportExportFilter::batchMode() const
{
    return d->batchmode;
}

KisImportUserFeedbackInterface *KisImportExportFilter::importUserFeedBackInterface() const
{
    return d->importUserFeedBackInterface;
}

void KisImportExportFilter::setBatchMode(bool batchmode)
{
    d->batchmode = batchmode;
}

void KisImportExportFilter::setImportUserFeedBackInterface(KisImportUserFeedbackInterface *interface)
{
    d->importUserFeedBackInterface = interface;
}

void KisImportExportFilter::setFilename(const QString &filename)
{
    d->filename = filename;
}

void KisImportExportFilter::setRealFilename(const QString &filename)
{
    d->realFilename = filename;
}

void KisImportExportFilter::setMimeType(const QString &mime)
{
    d->mime = mime.toLatin1();
}

QByteArray KisImportExportFilter::mimeType() const
{
    return d->mime;
}

bool KisImportExportFilter::exportSupportsGuides() const
{
    return false;
}
