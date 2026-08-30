/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_icon_utils.h"
#include "widgets/KisWarningWidget.h"

#include <QColor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QStringList>
#include <QTest>
#include <QWidget>

namespace
{

QStringList requestedIconNames;

QLabel *warningTextLabel(KisWarningWidget &widget)
{
    const QList<QLabel *> labels = widget.findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->wordWrap()) {
            return label;
        }
    }
    return nullptr;
}

} // namespace

namespace KisIconUtils
{

QIcon loadIcon(const QString &name)
{
    requestedIconNames.append(name);

    QPixmap pixmap(2, 2);
    pixmap.fill(QColor(61, 113, 167));
    return QIcon(pixmap);
}

} // namespace KisIconUtils

class KisWarningWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionCreatesTheWarningPresentation();
    void setTextUpdatesTheWrappedExternalLinkLabel();
    void profileChangeWarningDescribesEveryRiskCondition();
};

void KisWarningWidgetContractTest::constructionCreatesTheWarningPresentation()
{
    requestedIconNames.clear();
    QWidget parent;
    KisWarningWidget widget(&parent);

    QCOMPARE(widget.parentWidget(), &parent);
    QCOMPARE(requestedIconNames, QStringList{QStringLiteral("warning")});
    QCOMPARE(widget.findChildren<QLabel *>().size(), 2);
    QVERIFY(qobject_cast<QHBoxLayout *>(widget.layout()));

    QLabel *textLabel = warningTextLabel(widget);
    QVERIFY(textLabel);
    QVERIFY(textLabel->wordWrap());
    QVERIFY(textLabel->openExternalLinks());
}

void KisWarningWidgetContractTest::setTextUpdatesTheWrappedExternalLinkLabel()
{
    KisWarningWidget widget(nullptr);
    QLabel *textLabel = warningTextLabel(widget);
    QVERIFY(textLabel);

    const QString warning = QStringLiteral("<p>A <a href=\"https://example.invalid\">warning</a></p>");
    widget.setText(warning);

    QCOMPARE(textLabel->text(), warning);
}

void KisWarningWidgetContractTest::profileChangeWarningDescribesEveryRiskCondition()
{
    const QString warning = KisWarningWidget::changeImageProfileWarningText();

    QVERIFY(warning.startsWith(QStringLiteral("<html><body>")));
    QVERIFY(warning.contains(QStringLiteral("more than one layer")));
    QVERIFY(warning.contains(QStringLiteral("transparent pixels")));
    QVERIFY(warning.contains(QStringLiteral("blending modes other than \"Normal\"")));
    QVERIFY(warning.endsWith(QStringLiteral("</body></html>")));
}

QTEST_MAIN(KisWarningWidgetContractTest)

#include "KisWarningWidgetContractTest.moc"
