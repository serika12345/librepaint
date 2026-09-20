/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KisResourcesInterface_P_H
#define KisResourcesInterface_P_H

#include "kritaresources_export.h"
#include "KisResourcesInterface.h"
#include <unordered_map>
#include <memory>

#include <QReadWriteLock>

#include "kis_assert.h"

class KRITARESOURCES_EXPORT KisResourcesInterfacePrivate
{
public:
    virtual ~KisResourcesInterfacePrivate() {}

private:
    friend class KisResourcesInterface;

    mutable std::unordered_map<QString,
                       std::unique_ptr<
                           KisResourcesInterface::ResourceSourceAdapter>> sourceAdapters;
    mutable QReadWriteLock lock;

    KisResourcesInterface::ResourceSourceAdapter* findExistingSource(const QString &type) const {
        auto it = this->sourceAdapters.find(type);
        if (it != this->sourceAdapters.end()) {
            KIS_ASSERT(bool(it->second));

            return it->second.get();
        }

        return nullptr;
    }
};

#endif // KisResourcesInterface_P_H
