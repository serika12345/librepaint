/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QEvent>
#include <QModelIndex>
#include <QPointer>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTest>
#include <QTextDocument>
#include <QWidget>

#include "KoItemToolTip.h"

namespace
{

class ItemToolTipProbe : public KoItemToolTip
{
public:
    void setDocumentHtml(const QString &html)
    {
        m_html = html;
    }

    QPointer<QTextDocument> lastCreatedDocument() const
    {
        return m_lastCreatedDocument;
    }

    QModelIndex lastIndex() const
    {
        return m_lastIndex;
    }

    int createDocumentCalls() const
    {
        return m_createDocumentCalls;
    }

protected:
    QTextDocument *createDocument(const QModelIndex &index) override
    {
        auto *document = new QTextDocument(this);
        document->setHtml(m_html);
        document->setTextWidth(180.0);

        m_lastCreatedDocument = document;
        m_lastIndex = index;
        ++m_createDocumentCalls;
        return document;
    }

private:
    QString m_html;
    QPointer<QTextDocument> m_lastCreatedDocument;
    QModelIndex m_lastIndex;
    int m_createDocumentCalls = 0;
};

QStyleOptionViewItem optionFor(const QWidget &widget)
{
    QStyleOptionViewItem option;
    option.rect = widget.rect().adjusted(10, 10, -10, -10);
    return option;
}

} // namespace

class KoItemToolTipContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsTooltipWindowAndProvidesVirtualLifetime();
    void showTipOwnsTheCreatedDocumentAndReportsItsSize();
    void repeatedContentKeepsTheDisplayedDocument();
    void applicationInteractionHidesTheVisibleTip();
};

void KoItemToolTipContractTest::constructsTooltipWindowAndProvidesVirtualLifetime()
{
    auto *toolTip = new ItemToolTipProbe;
    QPointer<ItemToolTipProbe> guardedToolTip(toolTip);

    QVERIFY(toolTip->isWindow());
    QCOMPARE(toolTip->windowType(), Qt::ToolTip);
    QVERIFY(toolTip->windowFlags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(toolTip->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
    QVERIFY(toolTip->windowFlags().testFlag(Qt::X11BypassWindowManagerHint));
    QVERIFY(!toolTip->parentWidget());

    QWidget *widget = toolTip;
    delete widget;

    QVERIFY(guardedToolTip.isNull());
}

void KoItemToolTipContractTest::showTipOwnsTheCreatedDocumentAndReportsItsSize()
{
    QWidget anchor;
    anchor.resize(400, 300);
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    ItemToolTipProbe toolTip;
    toolTip.setDocumentHtml(QStringLiteral("<h3>Layer</h3><p>Alpha channel</p>"));

    toolTip.showTip(&anchor, QPoint(30, 40), optionFor(anchor), index);

    const QPointer<QTextDocument> document = toolTip.lastCreatedDocument();
    QVERIFY(document);
    QCOMPARE(document->parent(), &toolTip);
    QCOMPARE(toolTip.lastIndex(), index);
    QCOMPARE(toolTip.createDocumentCalls(), 1);
    QCOMPARE(toolTip.sizeHint(), document->size().toSize());
    QCOMPARE(toolTip.size(), toolTip.sizeHint());
    QVERIFY(toolTip.isVisible());
}

void KoItemToolTipContractTest::repeatedContentKeepsTheDisplayedDocument()
{
    QWidget anchor;
    anchor.resize(400, 300);
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    ItemToolTipProbe toolTip;
    toolTip.setDocumentHtml(QStringLiteral("<p>Stable contents</p>"));
    const QStyleOptionViewItem option = optionFor(anchor);

    toolTip.showTip(&anchor, QPoint(20, 20), option, index);
    const QPointer<QTextDocument> displayedDocument = toolTip.lastCreatedDocument();
    const QPoint displayedPosition = toolTip.pos();

    toolTip.showTip(&anchor, QPoint(200, 200), option, index);

    QVERIFY(displayedDocument);
    QVERIFY(toolTip.lastCreatedDocument().isNull());
    QCOMPARE(toolTip.createDocumentCalls(), 2);
    QCOMPARE(toolTip.pos(), displayedPosition);

    toolTip.setDocumentHtml(QStringLiteral("<p>Changed contents</p>"));
    toolTip.showTip(&anchor, QPoint(200, 200), option, index);

    QVERIFY(displayedDocument.isNull());
    QVERIFY(toolTip.lastCreatedDocument());
    QCOMPARE(toolTip.createDocumentCalls(), 3);
    QCOMPARE(toolTip.pos(), displayedPosition);
}

void KoItemToolTipContractTest::applicationInteractionHidesTheVisibleTip()
{
    QWidget anchor;
    anchor.resize(400, 300);
    ItemToolTipProbe toolTip;
    toolTip.setDocumentHtml(QStringLiteral("<p>Dismiss me</p>"));
    toolTip.showTip(&anchor, QPoint(20, 20), optionFor(anchor), QModelIndex());
    QVERIFY(toolTip.isVisible());

    QEvent focusEvent(QEvent::FocusIn);
    QCoreApplication::sendEvent(&anchor, &focusEvent);

    QVERIFY(!toolTip.isVisible());
}

QTEST_MAIN(KoItemToolTipContractTest)

#include "KoItemToolTipContractTest.moc"
