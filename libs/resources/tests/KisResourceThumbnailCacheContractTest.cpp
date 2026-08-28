/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceModel.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QAbstractListModel>
#include <QTest>

namespace
{
QStringList normalizedLocations;
}

QString KisResourceThumbnailStorageLocation::makeAbsolute(const QString &storageLocation)
{
    normalizedLocations.append(storageLocation);
    return QStringLiteral("/normalized/") + storageLocation;
}

class KisResourceQueryMapper
{
public:
    static void insert(KisResourceThumbnailCache &cache,
                       const QPair<QString, QString> &key,
                       const QImage &image)
    {
        cache.insert(key, image);
    }
};

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("unexpected assertion: %s at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class ThumbnailModel : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() != 0) {
            return {};
        }

        switch (role) {
        case Qt::UserRole + KisAbstractResourceModel::Location:
            return QStringLiteral("bundle.asl");
        case Qt::UserRole + KisAbstractResourceModel::ResourceType:
            return QStringLiteral("patterns");
        case Qt::UserRole + KisAbstractResourceModel::Filename:
            return QStringLiteral("tile.png");
        default:
            return {};
        }
    }
};

class KisResourceThumbnailCacheContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void directCacheScalesSeededImageAndPreservesReturnedValue();
    void instanceReturnsStableSharedCache();
};

void KisResourceThumbnailCacheContractTest::init()
{
    normalizedLocations.clear();
}

void KisResourceThumbnailCacheContractTest::directCacheScalesSeededImageAndPreservesReturnedValue()
{
    ThumbnailModel model;
    const QModelIndex index = model.index(0, 0);
    QImage returnedImage;

    {
        KisResourceThumbnailCache cache;
        QImage originalImage(4, 2, QImage::Format_ARGB32);
        originalImage.fill(Qt::red);
        KisResourceQueryMapper::insert(
            cache,
            {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/tile.png")},
            originalImage);

        QCOMPARE(cache.getImage(index), originalImage);
        returnedImage = cache.getImage(
            index, QSize(2, 2), Qt::KeepAspectRatio, Qt::FastTransformation);
        QCOMPARE(returnedImage.size(), QSize(2, 1));
        QCOMPARE(returnedImage.pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(normalizedLocations,
                 QStringList({QStringLiteral("bundle.asl"), QStringLiteral("bundle.asl")}));
    }

    QCOMPARE(returnedImage.size(), QSize(2, 1));
    QCOMPARE(returnedImage.pixelColor(0, 0), QColor(Qt::red));
}

void KisResourceThumbnailCacheContractTest::instanceReturnsStableSharedCache()
{
    KisResourceThumbnailCache *first = KisResourceThumbnailCache::instance();

    QVERIFY(first);
    QCOMPARE(KisResourceThumbnailCache::instance(), first);
}

QTEST_GUILESS_MAIN(KisResourceThumbnailCacheContractTest)

#include "KisResourceThumbnailCacheContractTest.moc"
