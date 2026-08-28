/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceThumbnailPainter.h>

#include <KisResourceModel.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QAbstractListModel>
#include <QPainter>
#include <QPointer>
#include <QTest>

#include <utility>

QString KisResourceThumbnailStorageLocation::makeAbsolute(const QString &storageLocation)
{
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

class ThumbnailPainterModel : public QAbstractListModel
{
public:
    ThumbnailPainterModel(QString filename, QImage thumbnail)
        : m_filename(std::move(filename))
        , m_thumbnail(std::move(thumbnail))
    {
    }

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
            return m_filename;
        case Qt::UserRole + KisAbstractResourceModel::Thumbnail:
            return m_thumbnail;
        default:
            return {};
        }
    }

private:
    QString m_filename;
    QImage m_thumbnail;
};

void seedThumbnail(const QString &filename, const QImage &thumbnail)
{
    KisResourceQueryMapper::insert(
        *KisResourceThumbnailCache::instance(),
        {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/") + filename},
        thumbnail);
}

class KisResourceThumbnailPainterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentLifetime();
    void createsReadyThumbnailAtRequestedSize();
    void paintsSelectionMarginAroundThumbnail();
};

void KisResourceThumbnailPainterContractTest::followsParentLifetime()
{
    QPointer<KisResourceThumbnailPainter> painter;
    {
        QObject parent;
        painter = new KisResourceThumbnailPainter(&parent);
        QCOMPARE(painter->parent(), &parent);
    }

    QVERIFY(painter.isNull());
}

void KisResourceThumbnailPainterContractTest::createsReadyThumbnailAtRequestedSize()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    seedThumbnail(QStringLiteral("ready.png"), thumbnail);
    ThumbnailPainterModel model(QStringLiteral("ready.png"), thumbnail);
    KisResourceThumbnailPainter painter;

    const QImage ready = painter.getReadyThumbnail(model.index(0, 0), QSize(4, 4), QPalette());

    QCOMPARE(ready.size(), QSize(4, 4));
    QCOMPARE(ready.pixelColor(0, 0), QColor(Qt::red));
    QCOMPARE(ready.pixelColor(3, 3), QColor(Qt::red));
}

void KisResourceThumbnailPainterContractTest::paintsSelectionMarginAroundThumbnail()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    seedThumbnail(QStringLiteral("paint.png"), thumbnail);
    ThumbnailPainterModel model(QStringLiteral("paint.png"), thumbnail);
    KisResourceThumbnailPainter thumbnailPainter;
    QPalette palette;
    palette.setColor(QPalette::Window, Qt::yellow);
    palette.setColor(QPalette::Highlight, Qt::green);
    QImage canvas(6, 6, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    thumbnailPainter.paint(
        &painter, model.index(0, 0), canvas.rect(), palette, true, true);
    painter.end();

    QCOMPARE(canvas.pixelColor(0, 0), QColor(Qt::green));
    QCOMPARE(canvas.pixelColor(3, 3), QColor(Qt::red));
}

QTEST_MAIN(KisResourceThumbnailPainterContractTest)

#include "KisResourceThumbnailPainterContractTest.moc"
