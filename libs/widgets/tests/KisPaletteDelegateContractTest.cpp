/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPaletteDelegate.h>

#include <QAbstractListModel>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QStyleOptionViewItem>
#include <QTest>

#include <utility>

namespace
{

constexpr int IsGroupNameRole = Qt::UserRole + 1;
constexpr int CheckSlotRole = Qt::UserRole + 2;

class PaletteRowModel : public QAbstractListModel
{
public:
    struct Row {
        QString name;
        bool isGroup{false};
        bool hasSwatch{true};
        QBrush swatchBrush{Qt::blue};
    };

    explicit PaletteRowModel(Row row)
        : m_row(std::move(row))
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
            return m_row.name;
        case Qt::BackgroundRole:
            return m_row.swatchBrush;
        case IsGroupNameRole:
            return m_row.isGroup;
        case CheckSlotRole:
            return m_row.hasSwatch;
        default:
            return {};
        }
    }

private:
    Row m_row;
};

QStyleOptionViewItem optionForRect(bool selected = false)
{
    QStyleOptionViewItem option;
    option.rect = QRect(8, 8, 24, 24);
    option.decorationSize = QSize(31, 17);
    option.palette.setColor(QPalette::Highlight, QColor(210, 30, 170));
    option.state = selected ? QStyle::State_Selected : QStyle::State_None;
    return option;
}

QImage renderRow(const KisPaletteDelegate &delegate, const PaletteRowModel::Row &row, bool selected = false)
{
    PaletteRowModel model(row);
    QImage image(40, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    delegate.paint(&painter, optionForRect(selected), model.index(0));
    painter.end();
    return image;
}

} // namespace

class KisPaletteDelegateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentOwnershipAndHasVirtualLifetime();
    void sizeHintMirrorsDecorationAndInvalidPaintIsNoOp();
    void paintsGroupSwatchEmptySlotAndSelection();
    void crossedKeywordUsesCurrentCaseAndNullRules();
};

void KisPaletteDelegateContractTest::followsParentOwnershipAndHasVirtualLifetime()
{
    auto *parent = new QObject;
    QPointer<KisPaletteDelegate> delegate = new KisPaletteDelegate(parent);

    QCOMPARE(delegate->parent(), parent);
    delete parent;
    QVERIFY(delegate.isNull());

    QPointer<KisPaletteDelegate> polymorphicDelegate = new KisPaletteDelegate;
    QAbstractItemDelegate *base = polymorphicDelegate.data();
    delete base;
    QVERIFY(polymorphicDelegate.isNull());
}

void KisPaletteDelegateContractTest::sizeHintMirrorsDecorationAndInvalidPaintIsNoOp()
{
    KisPaletteDelegate delegate;
    const QStyleOptionViewItem option = optionForRect();
    PaletteRowModel model({QStringLiteral("swatch")});

    QCOMPARE(delegate.sizeHint(option, model.index(0)), option.decorationSize);
    QCOMPARE(delegate.sizeHint(option, QModelIndex()), option.decorationSize);

    QImage image(40, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(12, 34, 56));
    const QImage before = image;
    QPainter painter(&image);
    const QPen originalPen(QColor(76, 54, 32), 4.0);
    painter.setPen(originalPen);

    delegate.paint(&painter, option, QModelIndex());

    QCOMPARE(image, before);
    QCOMPARE(painter.pen(), originalPen);
}

void KisPaletteDelegateContractTest::paintsGroupSwatchEmptySlotAndSelection()
{
    KisPaletteDelegate delegate;

    const QImage group = renderRow(delegate, {QStringLiteral("group"), true}, true);
    QCOMPARE(group.pixelColor(8, 8), QColor(210, 30, 170));

    const QImage swatch = renderRow(delegate, {QStringLiteral("blue"), false, true, QBrush(Qt::blue)});
    QCOMPARE(swatch.pixelColor(8, 8), QColor(Qt::blue));
    QCOMPARE(swatch.pixelColor(20, 20), QColor(Qt::blue));

    const QImage selectedSwatch = renderRow(delegate, {QStringLiteral("blue"), false, true, QBrush(Qt::blue)}, true);
    QCOMPARE(selectedSwatch.pixelColor(8, 8), QColor(210, 30, 170));
    QCOMPARE(selectedSwatch.pixelColor(20, 20), QColor(Qt::blue));

    const QImage emptySlot = renderRow(delegate, {QStringLiteral("empty"), false, false});
    QCOMPARE(emptySlot.pixelColor(9, 9), QColor(Qt::darkGray));
    QCOMPARE(emptySlot.pixelColor(30, 9), QColor(Qt::gray));
    QCOMPARE(emptySlot.pixelColor(9, 30), QColor(Qt::gray));
    QCOMPARE(emptySlot.pixelColor(30, 30), QColor(Qt::darkGray));
}

void KisPaletteDelegateContractTest::crossedKeywordUsesCurrentCaseAndNullRules()
{
    KisPaletteDelegate delegate;
    const PaletteRowModel::Row row{QStringLiteral("BLOCKED swatch"), false, true, QBrush(Qt::blue)};
    const QImage uncrossed = renderRow(delegate, row);

    delegate.setCrossedKeyword(QStringLiteral("blocked"));
    const QImage lowercaseMatch = renderRow(delegate, row);
    QVERIFY(lowercaseMatch != uncrossed);
    QVERIFY(lowercaseMatch.pixelColor(20, 20).red() > lowercaseMatch.pixelColor(20, 20).blue());

    delegate.setCrossedKeyword(QStringLiteral("BLOCKED"));
    QCOMPARE(renderRow(delegate, row), uncrossed);

    delegate.setCrossedKeyword(QStringLiteral(""));
    QVERIFY(renderRow(delegate, row) != uncrossed);

    delegate.setCrossedKeyword(QString());
    QCOMPARE(renderRow(delegate, row), uncrossed);
}

QTEST_MAIN(KisPaletteDelegateContractTest)

#include "KisPaletteDelegateContractTest.moc"
