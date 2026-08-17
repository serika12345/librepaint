/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_UPDATE_INFO_H
#define KIS_UPDATE_INFO_H

#include <QMetaType>
#include <QRect>

#include <kis_shared.h>
#include <kis_shared_ptr.h>
#include <kritacanvas_export.h>

class KisUpdateInfo;
using KisUpdateInfoSP = KisSharedPtr<KisUpdateInfo>;

class KRITACANVAS_EXPORT KisUpdateInfo : public KisShared
{
public:
    KisUpdateInfo();
    virtual ~KisUpdateInfo();

    virtual QRect dirtyViewportRect();
    virtual QRect dirtyImageRect() const = 0;
    virtual int levelOfDetail() const = 0;
    virtual bool canBeCompressed() const;
};

Q_DECLARE_METATYPE(KisUpdateInfoSP)

class KRITACANVAS_EXPORT KisMarkerUpdateInfo : public KisUpdateInfo
{
public:
    enum Type {
        StartBatch = 0,
        EndBatch,
        BlockLodUpdates,
        UnblockLodUpdates,
    };

    KisMarkerUpdateInfo(Type type, const QRect &dirtyImageRect);

    Type type() const;
    QRect dirtyImageRect() const override;
    int levelOfDetail() const override;
    bool canBeCompressed() const override;

private:
    Type m_type;
    QRect m_dirtyImageRect;
};

#endif
