/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_warning_block.h"

#include <QLabel>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include <type_traits>

namespace
{

struct WarningLabels {
    QLabel *icon = nullptr;
    QLabel *text = nullptr;
};

WarningLabels warningLabels(KisWarningBlock *block)
{
    WarningLabels result;

    const QList<QLabel *> labels = block->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
    for (QLabel *label : labels) {
        if (label->textFormat() == Qt::RichText) {
            result.text = label;
        } else {
            result.icon = label;
        }
    }

    return result;
}

} // namespace

class KisWarningBlockContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsLabelsAndFollowsParentLifetime();
    void copyOperationsRemainDisabled();
    void textAndPixmapRoundTripThroughQtProperties();
    void forwardsTextLabelLinksExactly();
};

void KisWarningBlockContractTest::ownsLabelsAndFollowsParentLifetime()
{
    auto *parent = new QWidget;
    QPointer<KisWarningBlock> block = new KisWarningBlock(parent);
    const WarningLabels labels = warningLabels(block);
    QPointer<QLabel> iconLabel = labels.icon;
    QPointer<QLabel> textLabel = labels.text;

    QCOMPARE(block->parentWidget(), parent);
    QCOMPARE(block->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly).size(), 2);
    QVERIFY(iconLabel);
    QVERIFY(textLabel);
    QCOMPARE(iconLabel->parentWidget(), block.data());
    QCOMPARE(textLabel->parentWidget(), block.data());
    QCOMPARE(iconLabel->alignment(), Qt::AlignLeft | Qt::AlignTop);
    QVERIFY(textLabel->wordWrap());
    QCOMPARE(textLabel->textInteractionFlags(), Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);

    delete parent;

    QVERIFY(block.isNull());
    QVERIFY(iconLabel.isNull());
    QVERIFY(textLabel.isNull());
}

void KisWarningBlockContractTest::copyOperationsRemainDisabled()
{
    QVERIFY(!std::is_copy_constructible_v<KisWarningBlock>);
    QVERIFY(!std::is_copy_assignable_v<KisWarningBlock>);
}

void KisWarningBlockContractTest::textAndPixmapRoundTripThroughQtProperties()
{
    KisWarningBlock block;

    QCOMPARE(block.text(), QString());
    QVERIFY(block.pixmap().isNull());

    const QString directText = QStringLiteral("<a href=\"details\">Review details</a>");
    block.setText(directText);
    QCOMPARE(block.text(), directText);
    QCOMPARE(block.property("text").toString(), directText);

    const QString propertyText = QStringLiteral("Stored through Q_PROPERTY");
    QVERIFY(block.setProperty("text", propertyText));
    QCOMPARE(block.text(), propertyText);

    QPixmap directPixmap(5, 3);
    directPixmap.fill(Qt::yellow);
    block.setPixmap(directPixmap);
    QCOMPARE(block.pixmap().toImage(), directPixmap.toImage());
    QCOMPARE(block.property("pixmap").value<QPixmap>().toImage(), directPixmap.toImage());

    QPixmap propertyPixmap(3, 4);
    propertyPixmap.fill(Qt::blue);
    QVERIFY(block.setProperty("pixmap", propertyPixmap));
    QCOMPARE(block.pixmap().toImage(), propertyPixmap.toImage());
}

void KisWarningBlockContractTest::forwardsTextLabelLinksExactly()
{
    KisWarningBlock block;
    const WarningLabels labels = warningLabels(&block);
    QVERIFY(labels.icon);
    QVERIFY(labels.text);

    QSignalSpy linkSpy(&block, &KisWarningBlock::linkActivated);

    const QString unrelatedLink = QStringLiteral("icon://ignored");
    QVERIFY(
        QMetaObject::invokeMethod(labels.icon, "linkActivated", Qt::DirectConnection, Q_ARG(QString, unrelatedLink)));
    QCOMPARE(linkSpy.count(), 0);

    const QString expectedLink = QStringLiteral("https://example.invalid/details?layer=3#warning");
    QVERIFY(
        QMetaObject::invokeMethod(labels.text, "linkActivated", Qt::DirectConnection, Q_ARG(QString, expectedLink)));
    QCOMPARE(linkSpy.count(), 1);
    QCOMPARE(linkSpy.takeFirst().at(0).toString(), expectedLink);
}

QTEST_MAIN(KisWarningBlockContractTest)

#include "KisWarningBlockContractTest.moc"
