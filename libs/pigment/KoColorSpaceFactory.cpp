/*
 *  SPDX-FileCopyrightText: 2010 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoColorSpaceFactory.h"


#include <QMutex>
#include <QMutexLocker>

#include "KoColorProfile.h"
#include "KoColorProfileQuery.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"

#include "kis_assert.h"

struct Q_DECL_HIDDEN KoColorSpaceFactory::Private {
    QHash<QString, KoColorSpace* > availableColorspaces;
    QMutex mutex;
#ifndef NDEBUG
    QHash<KoColorSpace*, QString> stackInformation;
#endif
};

KoColorSpaceFactory::KoColorSpaceFactory() : d(new Private)
{
}

KoColorSpaceFactory::~KoColorSpaceFactory()
{
#ifndef NDEBUG
    // Check that all color spaces have been released
    int count = 0;
    count += d->availableColorspaces.size();

    for(QHash<KoColorSpace*, QString>::const_iterator it = d->stackInformation.constBegin();
        it != d->stackInformation.constEnd(); ++it)
    {
        errorPigment << "*******************************************";
        errorPigment << it.key()->id() << " still in used, and grabbed in: ";
        errorPigment << it.value();
    }
#endif
    delete d;
}

const KoColorSpace *KoColorSpaceFactory::grabColorSpace(const KoColorProfile * profile)
{
    QMutexLocker l(&d->mutex);
    Q_ASSERT(profile);
    auto it = d->availableColorspaces.find(profile->name());
    KoColorSpace* cs;

    if (it == d->availableColorspaces.end()) {
        cs = createColorSpace(profile);
        KIS_ASSERT_X(cs != nullptr, "KoColorSpaceFactory::grabColorSpace", "createColorSpace returned nullptr.");
        if (cs) {
            d->availableColorspaces[profile->name()] = cs;
        }
    }
    else {
        cs = it.value();
    }

    return cs;
}

QList<KoColorProfileQuery> KoColorSpaceFactory::requiredConnectionProfiles(const KoColorProfile *profile) const
{
    Q_UNUSED(profile)
    return {};
}
