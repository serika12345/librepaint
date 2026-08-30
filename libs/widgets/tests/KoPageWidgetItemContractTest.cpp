/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoPageWidgetItem.h>

#include <QString>
#include <QTest>
#include <QWidget>

namespace
{

class PageItemProbe : public KoPageWidgetItem
{
public:
    PageItemProbe(QWidget *page, int *destructionCount)
        : m_page(page)
        , m_destructionCount(destructionCount)
    {
    }

    ~PageItemProbe() override
    {
        ++*m_destructionCount;
    }

    QWidget *widget() override
    {
        ++widgetCount;
        return m_page;
    }

    QString name() const override
    {
        ++nameCount;
        return QStringLiteral("Document settings");
    }

    QString iconName() const override
    {
        ++iconNameCount;
        return QStringLiteral("document-properties");
    }

    bool shouldDialogCloseBeVetoed() override
    {
        ++vetoCount;
        return vetoClose;
    }

    void apply() override
    {
        ++applyCount;
    }

    int widgetCount = 0;
    mutable int nameCount = 0;
    mutable int iconNameCount = 0;
    int vetoCount = 0;
    int applyCount = 0;
    bool vetoClose = true;

private:
    QWidget *m_page;
    int *m_destructionCount;
};

} // namespace

class KoPageWidgetItemContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void routesPagePresentationApplicationAndVirtualLifetime();
};

void KoPageWidgetItemContractTest::routesPagePresentationApplicationAndVirtualLifetime()
{
    QWidget page;
    int destructionCount = 0;
    auto *probe = new PageItemProbe(&page, &destructionCount);
    KoPageWidgetItem *item = probe;

    QCOMPARE(item->widget(), &page);
    QCOMPARE(item->name(), QStringLiteral("Document settings"));
    QCOMPARE(item->iconName(), QStringLiteral("document-properties"));
    QVERIFY(item->shouldDialogCloseBeVetoed());
    item->apply();

    QCOMPARE(probe->widgetCount, 1);
    QCOMPARE(probe->nameCount, 1);
    QCOMPARE(probe->iconNameCount, 1);
    QCOMPARE(probe->vetoCount, 1);
    QCOMPARE(probe->applyCount, 1);

    probe->vetoClose = false;
    QVERIFY(!item->shouldDialogCloseBeVetoed());
    QCOMPARE(probe->vetoCount, 2);

    delete item;
    QCOMPARE(destructionCount, 1);
}

QTEST_MAIN(KoPageWidgetItemContractTest)

#include "KoPageWidgetItemContractTest.moc"
