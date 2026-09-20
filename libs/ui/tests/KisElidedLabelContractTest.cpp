/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_elided_label.h"

#include <QApplication>
#include <QFontMetrics>
#include <QLabel>
#include <QResizeEvent>
#include <QTest>
#include <QWidget>

namespace
{

class DerivedElidedLabel : public KisElidedLabel
{
public:
    explicit DerivedElidedLabel(bool *destroyed)
        : KisElidedLabel(QStringLiteral("derived"), Qt::ElideRight)
        , m_destroyed(destroyed)
    {
    }

    ~DerivedElidedLabel() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

} // namespace

class KisElidedLabelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionKeepsParentAndInitialText();
    void longTextUsesTheConfiguredElideMode();
    void resizeReelidesTheStoredLongText();
    void basePointerDeletionRunsDerivedDestructor();
};

void KisElidedLabelContractTest::constructionKeepsParentAndInitialText()
{
    QWidget parent;
    const QString initialText = QStringLiteral("initial label text");
    KisElidedLabel label(initialText, Qt::ElideRight, &parent);

    QCOMPARE(label.parentWidget(), &parent);
    QCOMPARE(label.text(), initialText);
}

void KisElidedLabelContractTest::longTextUsesTheConfiguredElideMode()
{
    const QString longText = QStringLiteral("A deliberately long label used for deterministic elision");
    const QList<Qt::TextElideMode> modes{
        Qt::ElideLeft,
        Qt::ElideMiddle,
        Qt::ElideRight,
    };

    for (const Qt::TextElideMode mode : modes) {
        KisElidedLabel label(QString(), mode);
        label.resize(80, 24);

        label.setLongText(longText);

        QCOMPARE(label.text(), QFontMetrics(label.font()).elidedText(longText, mode, label.width()));
    }
}

void KisElidedLabelContractTest::resizeReelidesTheStoredLongText()
{
    const QString longText = QStringLiteral("The complete value must survive repeated label resizing");
    KisElidedLabel label(QString(), Qt::ElideMiddle);
    label.resize(500, 24);
    label.setLongText(longText);
    QCOMPARE(label.text(), QFontMetrics(label.font()).elidedText(longText, Qt::ElideMiddle, label.width()));

    label.resize(60, 24);
    QResizeEvent resizeEvent(label.size(), QSize(500, 24));
    QApplication::sendEvent(&label, &resizeEvent);

    QCOMPARE(label.text(), QFontMetrics(label.font()).elidedText(longText, Qt::ElideMiddle, label.width()));
}

void KisElidedLabelContractTest::basePointerDeletionRunsDerivedDestructor()
{
    bool destroyed = false;
    QLabel *label = new DerivedElidedLabel(&destroyed);

    delete label;

    QVERIFY(destroyed);
}

QTEST_MAIN(KisElidedLabelContractTest)

#include "KisElidedLabelContractTest.moc"
