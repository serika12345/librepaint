/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDockWidgetTitleBarButton.h>

#include <QAbstractButton>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QProxyStyle>
#include <QStyleOption>
#include <QTest>
#include <QWidget>

namespace
{

class FixedMetricStyle : public QProxyStyle
{
public:
    FixedMetricStyle(int iconSize, int margin)
        : m_iconSize(iconSize)
        , m_margin(margin)
    {
    }

    int pixelMetric(PixelMetric metric,
                    const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override
    {
        if (metric == QStyle::PM_SmallIconSize) {
            return m_iconSize;
        }
        if (metric == QStyle::PM_DockWidgetTitleBarButtonMargin) {
            return m_margin;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

private:
    int m_iconSize;
    int m_margin;
};

QIcon iconWithSize(const QSize &size, const QColor &color)
{
    QPixmap pixmap(size);
    pixmap.fill(color);
    return QIcon(pixmap);
}

QSize expectedIconHint(const KoDockWidgetTitleBarButton &button, int iconSize, int margin)
{
    return button.icon().pixmap(iconSize).size() + QSize(margin, margin);
}

} // namespace

class KoDockWidgetTitleBarButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentOwnershipAndHasVirtualLifetime();
    void iconlessHintsRemainFixedAndEqual();
    void iconHintsFollowDeterministicStyleMetrics();
    void marginAndIconOnlyChangesRetainTheCachedHint();
};

void KoDockWidgetTitleBarButtonContractTest::followsParentOwnershipAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KoDockWidgetTitleBarButton> button = new KoDockWidgetTitleBarButton(parent);

    QCOMPARE(button->parentWidget(), parent);
    QCOMPARE(button->focusPolicy(), Qt::NoFocus);

    delete parent;
    QVERIFY(button.isNull());

    QPointer<KoDockWidgetTitleBarButton> polymorphicButton = new KoDockWidgetTitleBarButton;
    QAbstractButton *base = polymorphicButton.data();
    delete base;
    QVERIFY(polymorphicButton.isNull());
}

void KoDockWidgetTitleBarButtonContractTest::iconlessHintsRemainFixedAndEqual()
{
    KoDockWidgetTitleBarButton button;

    QVERIFY(button.icon().isNull());
    QCOMPARE(button.sizeHint(), QSize(18, 18));
    QCOMPARE(button.minimumSizeHint(), button.sizeHint());
}

void KoDockWidgetTitleBarButtonContractTest::iconHintsFollowDeterministicStyleMetrics()
{
    FixedMetricStyle compactStyle(12, 4);
    FixedMetricStyle spaciousStyle(20, 7);
    KoDockWidgetTitleBarButton button;
    button.setIcon(iconWithSize(QSize(32, 24), Qt::blue));

    button.setStyle(&compactStyle);
    const QSize compactHint = expectedIconHint(button, 12, 4);
    QCOMPARE(button.sizeHint(), compactHint);
    QCOMPARE(button.minimumSizeHint(), compactHint);

    button.setStyle(&spaciousStyle);
    const QSize spaciousHint = expectedIconHint(button, 20, 7);
    QVERIFY(spaciousHint != compactHint);
    QCOMPARE(button.sizeHint(), spaciousHint);
    QCOMPARE(button.minimumSizeHint(), spaciousHint);
}

void KoDockWidgetTitleBarButtonContractTest::marginAndIconOnlyChangesRetainTheCachedHint()
{
    FixedMetricStyle initialStyle(20, 4);
    FixedMetricStyle widerMarginStyle(20, 9);
    KoDockWidgetTitleBarButton button;
    button.setIcon(iconWithSize(QSize(20, 12), Qt::red));
    button.setStyle(&initialStyle);

    const QSize cachedHint = button.sizeHint();
    QCOMPARE(cachedHint, expectedIconHint(button, 20, 4));

    button.setStyle(&widerMarginStyle);
    const QSize marginAdjustedHint = expectedIconHint(button, 20, 9);
    QVERIFY(marginAdjustedHint != cachedHint);
    QCOMPARE(button.sizeHint(), cachedHint);
    QCOMPARE(button.minimumSizeHint(), cachedHint);

    button.setIcon(iconWithSize(QSize(8, 20), Qt::green));
    const QSize iconAdjustedHint = expectedIconHint(button, 20, 9);
    QVERIFY(iconAdjustedHint != cachedHint);
    QCOMPARE(button.sizeHint(), cachedHint);
    QCOMPARE(button.minimumSizeHint(), cachedHint);
}

QTEST_MAIN(KoDockWidgetTitleBarButtonContractTest)

#include "KoDockWidgetTitleBarButtonContractTest.moc"
