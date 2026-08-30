/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorPopupButton.h>

#include <QCoreApplication>
#include <QPointer>
#include <QProxyStyle>
#include <QSignalSpy>
#include <QStyleOption>
#include <QTest>
#include <QToolButton>
#include <QWidget>

namespace
{

class FixedContentsStyle : public QProxyStyle
{
public:
    FixedContentsStyle(int horizontalChrome, int verticalChrome)
        : m_horizontalChrome(horizontalChrome)
        , m_verticalChrome(verticalChrome)
    {
    }

    QSize sizeFromContents(ContentsType type,
                           const QStyleOption *option,
                           const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override
    {
        if (type == QStyle::CT_ToolButton) {
            return contentsSize + QSize(m_horizontalChrome, m_verticalChrome);
        }
        return QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }

private:
    int m_horizontalChrome;
    int m_verticalChrome;
};

} // namespace

class KoColorPopupButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentOwnershipAndHasVirtualLifetime();
    void sizeHintUsesDeterministicToolButtonContents();
    void widthResizeUpdatesIconWidthAndEmitsOnce();
    void heightOnlyResizeEmitsWithoutChangingIconSize();
};

void KoColorPopupButtonContractTest::followsParentOwnershipAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KoColorPopupButton> button = new KoColorPopupButton(parent);

    QCOMPARE(button->parentWidget(), parent);
    QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonIconOnly);

    delete parent;
    QVERIFY(button.isNull());

    QPointer<KoColorPopupButton> polymorphicButton = new KoColorPopupButton;
    QToolButton *base = polymorphicButton.data();
    delete base;
    QVERIFY(polymorphicButton.isNull());
}

void KoColorPopupButtonContractTest::sizeHintUsesDeterministicToolButtonContents()
{
    FixedContentsStyle style(8, 6);
    KoColorPopupButton button;
    button.setStyle(&style);

    QCOMPARE(button.sizeHint(), QSize(24, 22));

    button.setIconSize(QSize(32, 28));
    QCOMPARE(button.sizeHint(), QSize(24, 22));
}

void KoColorPopupButtonContractTest::widthResizeUpdatesIconWidthAndEmitsOnce()
{
    FixedContentsStyle style(8, 6);
    KoColorPopupButton button;
    button.setStyle(&style);
    button.resize(QSize(40, 20));
    button.show();
    QCoreApplication::processEvents();
    button.setIconSize(QSize(10, 8));
    QSignalSpy iconSizeSpy(&button, &KoColorPopupButton::iconSizeChanged);

    button.resize(QSize(60, 20));
    QCoreApplication::processEvents();

    QCOMPARE(iconSizeSpy.count(), 1);
    QCOMPARE(button.iconSize(), QSize(52, 8));
}

void KoColorPopupButtonContractTest::heightOnlyResizeEmitsWithoutChangingIconSize()
{
    FixedContentsStyle style(8, 6);
    KoColorPopupButton button;
    button.setStyle(&style);
    button.resize(QSize(40, 20));
    button.show();
    QCoreApplication::processEvents();
    button.setIconSize(QSize(10, 8));
    button.resize(QSize(60, 20));
    QCoreApplication::processEvents();
    const QSize iconSizeBeforeHeightChange = button.iconSize();
    QSignalSpy iconSizeSpy(&button, &KoColorPopupButton::iconSizeChanged);

    button.resize(QSize(60, 30));
    QCoreApplication::processEvents();

    QCOMPARE(button.iconSize(), iconSizeBeforeHeightChange);
    QCOMPARE(iconSizeSpy.count(), 1);
}

QTEST_MAIN(KoColorPopupButtonContractTest)

#include "KoColorPopupButtonContractTest.moc"
