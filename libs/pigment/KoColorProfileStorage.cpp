/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoColorProfileStorage.h"

#include <cmath>

#include <QHash>
#include <QReadWriteLock>
#include <QString>

#include "KoColorSpaceFactory.h"
#include "KoColorProfile.h"
#include "KoColorProfileQuery.h"


struct KoColorProfileStorage::Private {
    QHash<QString, KoColorProfile * > profileMap;
    QHash<QByteArray, KoColorProfile * > profileUniqueIdMap;
    QList<KoColorProfile *> duplicates;
    QHash<QString, QString> profileAlias;
    QReadWriteLock lock;

    void populateUniqueIdMap();

    ~Private()
    {
        Q_FOREACH (KoColorProfile *p, profileMap) {
            profileUniqueIdMap.remove(p->uniqueId());
            duplicates.removeAll(p);
            delete p;
        }
        profileMap.clear();
        Q_FOREACH (KoColorProfile *p, profileUniqueIdMap) {
            duplicates.removeAll(p);
            delete p;
        }
        profileUniqueIdMap.clear();
        Q_FOREACH(KoColorProfile *p, duplicates) {
            delete p;
        }
        duplicates.clear();
    }
};

KoColorProfileStorage::KoColorProfileStorage()
    : d(new Private)
{

}

KoColorProfileStorage::~KoColorProfileStorage()
{
}

void KoColorProfileStorage::addProfile(KoColorProfile *profile)
{
    QWriteLocker locker(&d->lock);

    if (profile->valid()) {
        d->profileMap[profile->name()] = profile;
        if (d->profileUniqueIdMap.contains(profile->uniqueId())) {
            //warnPigment << "Duplicated profile" << profile->name() << profile->fileName() << d->profileUniqueIdMap[profile->uniqueId()]->fileName();
            d->duplicates.append(d->profileUniqueIdMap[profile->uniqueId()]);
        }
        d->profileUniqueIdMap.insert(profile->uniqueId(), profile);
    }
}

void KoColorProfileStorage::removeProfile(KoColorProfile *profile)
{
    QWriteLocker locker(&d->lock);

    d->profileMap.remove(profile->name());
    d->profileUniqueIdMap.remove(profile->uniqueId());
    d->duplicates.removeAll(profile);
}

bool KoColorProfileStorage::containsProfile(const KoColorProfile *profile)
{
    QReadLocker l(&d->lock);
    return d->profileMap.contains(profile->name());
}

void KoColorProfileStorage::addProfileAlias(const QString &name, const QString &to)
{
    QWriteLocker l(&d->lock);
    d->profileAlias[name] = to;
}

void KoColorProfileStorage::removeProfileAlias(const QString &name)
{
    QWriteLocker l(&d->lock);
    d->profileAlias.remove(name);
}

QString KoColorProfileStorage::profileAlias(const QString &name) const
{
    QReadLocker l(&d->lock);
    /**
     * Profile aliases are considered 'weak', i.e. they can
     * be overridden by a read profile added to the storage.
     */
    return d->profileMap.contains(name) ? name : d->profileAlias.value(name, name);
}

const KoColorProfile *KoColorProfileStorage::profileByName(const QString &name) const
{
    QReadLocker l(&d->lock);
    /**
     * Profile aliases are considered 'weak', i.e. they can
     * be overridden by a read profile added to the storage.
     */
    if (d->profileMap.contains(name)) {
        return d->profileMap[name];
    }

    return d->profileMap.value(d->profileAlias.value(name, name), 0);
}

void KoColorProfileStorage::Private::populateUniqueIdMap()
{
    QWriteLocker l(&lock);
    profileUniqueIdMap.clear();

    for (auto it = profileMap.constBegin();
         it != profileMap.constEnd();
         ++it) {

        KoColorProfile *profile = it.value();
        QByteArray id = profile->uniqueId();

        if (!id.isEmpty()) {
            profileUniqueIdMap.insert(id, profile);
        }
    }
}


const KoColorProfile *KoColorProfileStorage::profileByUniqueId(const QByteArray &id) const
{
    QReadLocker l(&d->lock);
    if (d->profileUniqueIdMap.isEmpty()) {
        l.unlock();
        d->populateUniqueIdMap();
        l.relock();
    }
    return d->profileUniqueIdMap.value(id, 0);

}

QList<const KoColorProfile *> KoColorProfileStorage::profilesFor(const KoColorSpaceFactory *csf) const
{
    QList<const KoColorProfile *>  profiles;
    if (!csf) return profiles;

    QReadLocker l(&d->lock);

    QHash<QString, KoColorProfile * >::ConstIterator it;
    for (it = d->profileMap.constBegin(); it != d->profileMap.constEnd(); ++it) {
        KoColorProfile *  profile = it.value();
        if (csf->profileIsCompatible(profile)) {
            Q_ASSERT(profile);
            //         if (profile->colorSpaceSignature() == csf->colorSpaceSignature()) {
            profiles.push_back(profile);
        }
    }
    return profiles;
}

QList<const KoColorProfile *> KoColorProfileStorage::profilesFor(const KoColorProfileQuery &query, double error)
{
    QList<const KoColorProfile *> profiles;

    if (!query.isValid()) {
        return profiles;
    }

    QReadLocker l(&d->lock);
    for (const KoColorProfile* profile : d->profileMap) {
        bool colorantMatch = (!query.rgbColorants.isEmpty() || query.primaries != PRIMARIES_UNSPECIFIED);
        bool colorantTypeMatch = (query.primaries == PRIMARIES_UNSPECIFIED);
        bool transferMatch = (query.transfer == TRC_UNSPECIFIED);
        bool luminanceMatch = true;
        if (query.primaries != PRIMARIES_UNSPECIFIED) {
            if (int(profile->getColorPrimaries()) == query.primaries) {
                colorantTypeMatch = true;
            }
        }
        if (query.transfer != TRC_UNSPECIFIED) {
            if (int(profile->getTransferCharacteristics()) == query.transfer) {
                transferMatch = true;
            }
        }

        if (!query.rgbColorants.isEmpty() && query.primaries == PRIMARIES_UNSPECIFIED) {
            KoColorimetryUtils::xyY wp = profile->getWhitePointxyY();
            if (profile->getColorantsxyY().size() == query.rgbColorants.size()) {
                QVector<KoColorimetryUtils::xyY> col = profile->getColorantsxyY();
                if (col.isEmpty()) {
                    // too few colorants, skip.
                    continue;
                }

                colorantMatch = (std::fabs(wp.x - query.whitePoint.x) < error) && (std::fabs(wp.y - query.whitePoint.y) < error);
                if (!colorantMatch) {
                    continue;
                }

                for (int i = 0; i < col.size(); i++) {
                    colorantMatch = (std::fabs(col[i].x - query.rgbColorants[i].x) < error) && (std::fabs(col[i].y - query.rgbColorants[i].y) < error);
                    if (!colorantMatch) {
                        break;
                    }
                }
            } else {
                if (std::fabs(wp.x - query.whitePoint.x) < error && std::fabs(wp.y - query.whitePoint.y) < error) {
                    colorantMatch = true;
                }
            }
        }

        if (query.transfer == TRC_ITU_R_BT_2100_0_PQ) {
            // by default we expect HDR profiles to have HDR Reference White point
            // to set to 203 nits
            // TODO: should we spit a warning if the reference white is not set
            //       for a pq space?
            const qreal requestedHdrReferenceWhite = query.hdrReferenceWhite.value_or(203.0);
            luminanceMatch = (profile->hdrReferenceWhite() && qFuzzyCompare(*profile->hdrReferenceWhite(), requestedHdrReferenceWhite));
        }

        if (transferMatch && colorantMatch && colorantTypeMatch && luminanceMatch) {
            profiles.push_back(profile);
        }
    }

    return profiles;
}
