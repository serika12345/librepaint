/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisIconToolTip.h>

#include <KisResourceModel.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QAbstractListModel>
#include <QPointer>
#include <QTest>
#include <QTextDocument>
#include <QUrl>

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

class ToolTipModel : public QAbstractListModel
{
public:
    ToolTipModel(QString filename, QImage thumbnail)
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
        case Qt::DisplayRole:
            return QStringLiteral("Pattern");
        case Qt::DecorationRole:
        case Qt::UserRole + KisAbstractResourceModel::Thumbnail:
            return m_thumbnail;
        case Qt::UserRole + KisAbstractResourceModel::Location:
            return QStringLiteral("bundle.asl");
        case Qt::UserRole + KisAbstractResourceModel::ResourceType:
            return QStringLiteral("patterns");
        case Qt::UserRole + KisAbstractResourceModel::Filename:
            return m_filename;
        default:
            return {};
        }
    }

private:
    QString m_filename;
    QImage m_thumbnail;
};

class ExposedIconToolTip : public KisIconToolTip
{
public:
    using KisIconToolTip::createDocument;
};

QImage documentThumbnail(QTextDocument *document)
{
    return document->resource(QTextDocument::ImageResource, QUrl(QStringLiteral("data:thumbnail")))
        .value<QImage>();
}

class KisIconToolTipContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fixedThumbnailSizeControlsDocumentImage();
    void checkerRenderingCanBeEnabledAndDisabled();
    void destructionReleasesOwnedDocument();
};

void KisIconToolTipContractTest::fixedThumbnailSizeControlsDocumentImage()
{
    QImage thumbnail(4, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    ToolTipModel model(QStringLiteral("fixed.png"), thumbnail);
    KisResourceQueryMapper::insert(
        *KisResourceThumbnailCache::instance(),
        {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/fixed.png")},
        thumbnail);

    ExposedIconToolTip toolTip;
    toolTip.setFixedToolTipThumbnailSize(QSize(2, 2));
    QTextDocument *document = toolTip.createDocument(model.index(0, 0));
    const QImage documentImage = documentThumbnail(document);

    QCOMPARE(documentImage.deviceIndependentSize(), QSizeF(2, 1));
    QCOMPARE(documentImage.pixelColor(0, 0), QColor(Qt::red));
}

void KisIconToolTipContractTest::checkerRenderingCanBeEnabledAndDisabled()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::transparent);
    ToolTipModel model(QStringLiteral("checker.png"), thumbnail);
    ExposedIconToolTip toolTip;

    toolTip.setToolTipShouldRenderCheckers(true);
    QTextDocument *checkerDocument = toolTip.createDocument(model.index(0, 0));
    QCOMPARE(documentThumbnail(checkerDocument).pixelColor(0, 0).alpha(), 255);

    toolTip.setToolTipShouldRenderCheckers(false);
    QTextDocument *plainDocument = toolTip.createDocument(model.index(0, 0));
    QCOMPARE(documentThumbnail(plainDocument).pixelColor(0, 0).alpha(), 0);
}

void KisIconToolTipContractTest::destructionReleasesOwnedDocument()
{
    QImage thumbnail(1, 1, QImage::Format_ARGB32);
    thumbnail.fill(Qt::blue);
    ToolTipModel model(QStringLiteral("lifetime.png"), thumbnail);
    QPointer<QTextDocument> document;

    {
        ExposedIconToolTip toolTip;
        document = toolTip.createDocument(model.index(0, 0));
        QVERIFY(document);
        QCOMPARE(document->parent(), &toolTip);
    }

    QVERIFY(document.isNull());
}

QTEST_MAIN(KisIconToolTipContractTest)

#include "KisIconToolTipContractTest.moc"
