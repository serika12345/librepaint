/*
 * SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef KOCOMPOSITEOPREGISTRY_H
#define KOCOMPOSITEOPREGISTRY_H

#include <QString>
#include <QList>
#include <QMultiMap>

#include "kritapigment_export.h"
#include "KoCompositeOpIds.h"

#include <KoID.h>

class KoColorSpace;

class KRITAPIGMENT_EXPORT KoCompositeOpRegistry
{
    typedef QMultiMap<KoID,KoID> KoIDMap;
    typedef QList<KoID>          KoIDList;

public:
    KoCompositeOpRegistry();
    static const KoCompositeOpRegistry& instance();

    KoID     getDefaultCompositeOp() const;
    KoID     getKoID(const QString& compositeOpID) const;
    QString  getCompositeOpDisplayName(const QString& compositeOpID) const;
    KoIDMap  getCompositeOps() const;
    KoIDMap  getLayerStylesCompositeOps() const;
    KoIDList getCategories() const;
    QString  getCategoryDisplayName(const QString& categoryID) const;
    KoIDList getCompositeOps(const KoColorSpace* colorSpace) const;
    KoIDList getCompositeOps(const KoID& category, const KoColorSpace* colorSpace=0) const;
    bool     colorSpaceHasCompositeOp(const KoColorSpace* colorSpace, const KoID& compositeOp) const;

    template<class TKoIdIterator>
    KoIDList filterCompositeOps(TKoIdIterator begin, TKoIdIterator end, const KoColorSpace* colorSpace, bool removeInvalidOps=true) const {
        KoIDList list;

        for(; begin!=end; ++begin){
            if (colorSpaceHasCompositeOp(colorSpace, *begin) == removeInvalidOps) {
                list.push_back(*begin);
            }
        }

        return list;
    }

private:
    KoIDList m_categories;
    KoIDMap  m_map;
};


#endif // KOCOMPOSITEOPREGISTRY_H
