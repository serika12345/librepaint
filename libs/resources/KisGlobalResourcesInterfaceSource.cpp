/*
 *  SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGlobalResourcesInterface.h"

#include <KisResourceModel.h>
#include <KisResourceModelProvider.h>

namespace
{
class GlobalResourcesSource : public KisResourcesInterface::ResourceSourceAdapter
{
public:
    GlobalResourcesSource(const QString &resourceType, KisAllResourcesModel *model)
        : KisResourcesInterface::ResourceSourceAdapter(resourceType)
        , m_model(model)
    {
    }

    QVector<KoResourceSP> resourcesForFilename(const QString &filename) const override
    {
        return m_model->resourcesForFilename(filename);
    }

    QVector<KoResourceSP> resourcesForName(const QString &name) const override
    {
        return m_model->resourcesForName(name);
    }

    QVector<KoResourceSP> resourcesForMD5(const QString &md5) const override
    {
        return m_model->resourcesForMD5(md5);
    }

    KoResourceSP fallbackResource() const override
    {
        return m_model->rowCount() > 0
            ? m_model->resourceForIndex(m_model->index(0, 0))
            : KoResourceSP();
    }

private:
    KisAllResourcesModel *m_model;
};
}

namespace KisGlobalResourcesInterfaceDetail
{
KisResourcesInterface::ResourceSourceAdapter *createModelBackedSource(const QString &type)
{
    return new GlobalResourcesSource(type, KisResourceModelProvider::resourceModel(type));
}
}
