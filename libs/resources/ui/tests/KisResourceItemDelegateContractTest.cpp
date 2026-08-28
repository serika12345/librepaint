/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemDelegate.h>

#include <KisResourceModel.h>
#include <KisResourceModelIndexResolver.h>
#include <KisResourceThumbnailCache.h>
#include <KisResourceThumbnailStorageLocation.h>

#include <QAbstractListModel>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QStyleOptionViewItem>
#include <QTest>

#include <utility>

namespace
{
QPersistentModelIndex resolvedResourceIndex;
QString resolvedResourceType;
int resolvedResourceId = -1;
}

QModelIndex KisResourceModelIndexResolver::resourceIndex(const QString &resourceType, int resourceId)
{
    resolvedResourceType = resourceType;
    resolvedResourceId = resourceId;
    return resolvedResourceIndex;
}

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

class DelegateModel : public QAbstractListModel
{
public:
    DelegateModel(QString filename, QImage thumbnail, int resourceId = 1)
        : m_filename(std::move(filename))
        , m_thumbnail(std::move(thumbnail))
        , m_resourceId(resourceId)
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
        case Qt::UserRole + KisAbstractResourceModel::Id:
            return m_resourceId;
        case Qt::UserRole + KisAbstractResourceModel::Name:
            return QStringLiteral("Pattern_Name");
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
    int m_resourceId;
};

void seedThumbnail(const QString &filename, const QImage &thumbnail)
{
    KisResourceQueryMapper::insert(
        *KisResourceThumbnailCache::instance(),
        {QStringLiteral("/normalized/bundle.asl"), QStringLiteral("patterns/") + filename},
        thumbnail);
}

QStyleOptionViewItem delegateOption(const QRect &rect)
{
    QStyleOptionViewItem option;
    option.rect = rect;
    option.decorationSize = QSize(9, 7);
    option.palette.setColor(QPalette::Window, Qt::yellow);
    option.palette.setColor(QPalette::Highlight, Qt::green);
    return option;
}

QImage paintDelegate(KisResourceItemDelegate &delegate,
                     const QModelIndex &index,
                     const QStyleOptionViewItem &option)
{
    QImage canvas(option.rect.size(), QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    delegate.paint(&painter, option, index);
    painter.end();
    return canvas;
}

class KisResourceItemDelegateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void followsParentLifetime();
    void sizeHintUsesDecorationSize();
    void showTextControlsThumbnailLayout();
    void selectionStateReachesThumbnailPainter();
    void indexConversionUsesResolvedGlobalIndex();
};

void KisResourceItemDelegateContractTest::init()
{
    resolvedResourceIndex = QPersistentModelIndex();
    resolvedResourceType.clear();
    resolvedResourceId = -1;
}

void KisResourceItemDelegateContractTest::followsParentLifetime()
{
    QPointer<KisResourceItemDelegate> delegate;
    {
        QObject parent;
        delegate = new KisResourceItemDelegate(&parent);
        QCOMPARE(delegate->parent(), &parent);
    }

    QVERIFY(delegate.isNull());
}

void KisResourceItemDelegateContractTest::sizeHintUsesDecorationSize()
{
    KisResourceItemDelegate delegate;
    const QStyleOptionViewItem option = delegateOption(QRect(0, 0, 20, 8));

    QCOMPARE(delegate.sizeHint(option, QModelIndex()), QSize(9, 7));
}

void KisResourceItemDelegateContractTest::showTextControlsThumbnailLayout()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    seedThumbnail(QStringLiteral("layout.png"), thumbnail);
    DelegateModel model(QStringLiteral("layout.png"), thumbnail);
    KisResourceItemDelegate delegate;
    const QStyleOptionViewItem option = delegateOption(QRect(0, 0, 20, 8));

    const QImage imageOnly = paintDelegate(delegate, model.index(0, 0), option);
    QCOMPARE(imageOnly.pixelColor(10, 4), QColor(Qt::red));

    delegate.setShowText(true);
    const QImage withText = paintDelegate(delegate, model.index(0, 0), option);
    QCOMPARE(withText.pixelColor(10, 4).alpha(), 0);
}

void KisResourceItemDelegateContractTest::selectionStateReachesThumbnailPainter()
{
    QImage thumbnail(2, 2, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    seedThumbnail(QStringLiteral("selected.png"), thumbnail);
    DelegateModel model(QStringLiteral("selected.png"), thumbnail);
    KisResourceItemDelegate delegate;
    QStyleOptionViewItem option = delegateOption(QRect(0, 0, 6, 6));
    option.state = QStyle::State_Selected;

    const QImage selected = paintDelegate(delegate, model.index(0, 0), option);

    QCOMPARE(selected.pixelColor(0, 0), QColor(Qt::green));
    QCOMPARE(selected.pixelColor(3, 3), QColor(Qt::red));
}

void KisResourceItemDelegateContractTest::indexConversionUsesResolvedGlobalIndex()
{
    QImage localThumbnail(2, 2, QImage::Format_ARGB32);
    localThumbnail.fill(Qt::red);
    DelegateModel localModel(QStringLiteral("local.png"), localThumbnail, 42);
    QImage globalThumbnail(2, 2, QImage::Format_ARGB32);
    globalThumbnail.fill(Qt::blue);
    seedThumbnail(QStringLiteral("global.png"), globalThumbnail);
    DelegateModel globalModel(QStringLiteral("global.png"), globalThumbnail, 99);
    resolvedResourceIndex = globalModel.index(0, 0);
    KisResourceItemDelegate delegate;
    delegate.setNeedIndexConversion(true);

    const QImage canvas = paintDelegate(
        delegate, localModel.index(0, 0), delegateOption(QRect(0, 0, 6, 6)));

    QCOMPARE(resolvedResourceType, QStringLiteral("patterns"));
    QCOMPARE(resolvedResourceId, 42);
    QCOMPARE(canvas.pixelColor(3, 3), QColor(Qt::blue));
}

QTEST_MAIN(KisResourceItemDelegateContractTest)

#include "KisResourceItemDelegateContractTest.moc"
