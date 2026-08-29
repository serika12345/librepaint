/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoVBox.h>

#include <QBoxLayout>
#include <QFrame>
#include <QPointer>
#include <QTest>
#include <QWidget>

namespace
{

class HintWidget : public QWidget
{
public:
    HintWidget(const QSize &sizeHint, const QSize &minimumSizeHint, QWidget *parent)
        : QWidget(parent)
        , m_sizeHint(sizeHint)
        , m_minimumSizeHint(minimumSizeHint)
    {
    }

    QSize sizeHint() const override
    {
        return m_sizeHint;
    }

    QSize minimumSizeHint() const override
    {
        return m_minimumSizeHint;
    }

private:
    QSize m_sizeHint;
    QSize m_minimumSizeHint;
};

QBoxLayout *boxLayout(KoVBox *box)
{
    return qobject_cast<QBoxLayout *>(box->layout());
}

} // namespace

class KoVBoxContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsChildrenInInsertionOrderAndHasVirtualLifetime();
    void exposesZeroLayoutDefaultsAndAppliesLayoutSettings();
    void sizeHintsTrackChildrenAndMargins();
};

void KoVBoxContractTest::ownsChildrenInInsertionOrderAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KoVBox> box = new KoVBox(parent);
    QPointer<QWidget> first = new QWidget(box);
    QPointer<QWidget> second = new QWidget(box);
    QBoxLayout *layout = boxLayout(box);

    QVERIFY(layout);
    QCOMPARE(box->parentWidget(), parent);
    QCOMPARE(first->parentWidget(), box.data());
    QCOMPARE(second->parentWidget(), box.data());
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->itemAt(0)->widget(), first.data());
    QCOMPARE(layout->itemAt(1)->widget(), second.data());

    second->setParent(nullptr);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->itemAt(0)->widget(), first.data());
    QVERIFY(second);

    delete parent;

    QVERIFY(box.isNull());
    QVERIFY(first.isNull());
    QVERIFY(second);
    delete second;

    QPointer<KoVBox> polymorphicBox = new KoVBox;
    QFrame *base = polymorphicBox.data();
    delete base;
    QVERIFY(polymorphicBox.isNull());
}

void KoVBoxContractTest::exposesZeroLayoutDefaultsAndAppliesLayoutSettings()
{
    KoVBox box;
    auto *first = new QWidget(&box);
    auto *second = new QWidget(&box);
    QBoxLayout *layout = boxLayout(&box);

    QVERIFY(layout);
    QCOMPARE(layout->direction(), QBoxLayout::TopToBottom);
    QCOMPARE(layout->spacing(), 0);
    QCOMPARE(layout->contentsMargins(), QMargins(0, 0, 0, 0));

    box.setMargin(7);
    box.setSpacing(5);
    box.setStretchFactor(first, 2);
    box.setStretchFactor(second, 3);

    QCOMPARE(layout->contentsMargins(), QMargins(7, 7, 7, 7));
    QCOMPARE(layout->spacing(), 5);
    QCOMPARE(layout->stretch(0), 2);
    QCOMPARE(layout->stretch(1), 3);
}

void KoVBoxContractTest::sizeHintsTrackChildrenAndMargins()
{
    KoVBox box;
    new HintWidget(QSize(40, 20), QSize(20, 10), &box);
    new HintWidget(QSize(60, 30), QSize(30, 15), &box);
    box.setSpacing(4);

    const QSize recommended = box.sizeHint();
    const QSize minimum = box.minimumSizeHint();

    QVERIFY(recommended.width() >= 60);
    QVERIFY(recommended.height() >= 54);
    QVERIFY(minimum.width() >= 30);
    QVERIFY(minimum.height() >= 29);
    QVERIFY(recommended.width() >= minimum.width());
    QVERIFY(recommended.height() >= minimum.height());

    box.setMargin(6);

    const QSize recommendedWithMargin = box.sizeHint();
    const QSize minimumWithMargin = box.minimumSizeHint();
    QCOMPARE(recommendedWithMargin.width(), recommended.width() + 12);
    QCOMPARE(recommendedWithMargin.height(), recommended.height() + 12);
    QCOMPARE(minimumWithMargin.width(), minimum.width() + 12);
    QCOMPARE(minimumWithMargin.height(), minimum.height() + 12);
}

QTEST_MAIN(KoVBoxContractTest)

#include "KoVBoxContractTest.moc"
