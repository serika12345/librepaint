/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisStorageChooserDelegate.h>

#include <KisStorageModel.h>

#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPointer>
#include <QProxyStyle>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QTest>

namespace KisIconUtils
{
QIcon loadIcon(const QString &)
{
    return QIcon();
}
}

class RecordingStyle : public QProxyStyle
{
public:
    RecordingStyle()
        : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
    {
    }

    void reset() const
    {
        panelDraws = 0;
        checkboxDraws = 0;
        panelState = {};
        checkboxState = {};
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        if (element == QStyle::PE_PanelButtonTool) {
            ++panelDraws;
            panelState = option->state;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget = nullptr) const override
    {
        if (element == QStyle::CE_CheckBox) {
            ++checkboxDraws;
            checkboxState = option->state;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }

    mutable int panelDraws = 0;
    mutable int checkboxDraws = 0;
    mutable QStyle::State panelState;
    mutable QStyle::State checkboxState;
};

class KisStorageChooserDelegateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void constructorUsesParentOwnership();
    void sizeHintUsesFixedWidthAndDecorationHeight();
    void paintSkipsInvalidIndexesAndDrawsStorageState();
};

void KisStorageChooserDelegateContractTest::initTestCase()
{
    QApplication::setStyle(new RecordingStyle);
}

void KisStorageChooserDelegateContractTest::constructorUsesParentOwnership()
{
    QObject *parent = new QObject;
    QPointer<KisStorageChooserDelegate> delegate = new KisStorageChooserDelegate(parent);

    QCOMPARE(delegate->parent(), parent);
    delete parent;
    QVERIFY(delegate.isNull());
}

void KisStorageChooserDelegateContractTest::sizeHintUsesFixedWidthAndDecorationHeight()
{
    KisStorageChooserDelegate delegate;
    QStyleOptionViewItem option;
    option.decorationSize = QSize(64, 32);

    QCOMPARE(delegate.sizeHint(option, QModelIndex()), QSize(200, 40));
}

void KisStorageChooserDelegateContractTest::paintSkipsInvalidIndexesAndDrawsStorageState()
{
    auto *style = dynamic_cast<RecordingStyle *>(QApplication::style());
    QVERIFY(style);
    KisStorageChooserDelegate delegate;
    QStyleOptionViewItem option;
    option.rect = QRect(10, 10, 220, 60);
    option.decorationSize = QSize(32, 32);
    option.palette = QApplication::palette();
    option.state = QStyle::State_Enabled;
    QImage canvas(240, 80, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);

    style->reset();
    delegate.paint(&painter, option, QModelIndex());
    QCOMPARE(style->panelDraws, 0);
    QCOMPARE(style->checkboxDraws, 0);
    QCOMPARE(canvas.pixelColor(14, 14), QColor(Qt::transparent));

    QStandardItemModel model(1, KisStorageModel::DisplayName + 1);
    const QModelIndex index = model.index(0, 0);
    QImage thumbnail(4, 4, QImage::Format_ARGB32);
    thumbnail.fill(Qt::red);
    model.setData(model.index(0, KisStorageModel::DisplayName),
                  QStringLiteral("Bundle_Name"),
                  Qt::DisplayRole);
    model.setData(model.index(0, KisStorageModel::Location),
                  QStringLiteral("bundle.asl"),
                  Qt::DisplayRole);
    model.setData(index, QStringLiteral("Bundle"),
                  Qt::UserRole + KisStorageModel::StorageType);
    model.setData(index, thumbnail, Qt::UserRole + KisStorageModel::Thumbnail);
    model.setData(index, false, Qt::UserRole + KisStorageModel::Active);

    style->reset();
    delegate.paint(&painter, option, index);
    QCOMPARE(style->panelDraws, 1);
    QVERIFY(!style->panelState.testFlag(QStyle::State_Sunken));
    QCOMPARE(style->checkboxDraws, 1);
    QVERIFY(style->checkboxState.testFlag(QStyle::State_Off));
    QCOMPARE(canvas.pixelColor(14, 14), QColor(Qt::red));

    model.setData(index, true, Qt::UserRole + KisStorageModel::Active);
    style->reset();
    delegate.paint(&painter, option, index);
    QCOMPARE(style->panelDraws, 1);
    QVERIFY(style->panelState.testFlag(QStyle::State_Sunken));
    QCOMPARE(style->checkboxDraws, 1);
    QVERIFY(style->checkboxState.testFlag(QStyle::State_On));
}

QTEST_MAIN(KisStorageChooserDelegateContractTest)

#include "KisStorageChooserDelegateContractTest.moc"
