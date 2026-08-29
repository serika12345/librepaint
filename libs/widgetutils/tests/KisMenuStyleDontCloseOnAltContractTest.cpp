/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMenuStyleDontCloseOnAlt.h"

#include <QStyleFactory>
#include <QStyleOption>
#include <QTest>
#include <QWidget>

#include <memory>

class RecordingStyle : public QProxyStyle
{
public:
    int styleHint(QStyle::StyleHint hint,
                  const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData) const override
    {
        ++m_callCount;
        m_hint = hint;
        m_option = option;
        m_widget = widget;
        m_returnData = returnData;
        return 731;
    }

    int callCount() const
    {
        return m_callCount;
    }

    QStyle::StyleHint hint() const
    {
        return m_hint;
    }

    const QStyleOption *option() const
    {
        return m_option;
    }

    const QWidget *widget() const
    {
        return m_widget;
    }

    QStyleHintReturn *returnData() const
    {
        return m_returnData;
    }

private:
    mutable int m_callCount = 0;
    mutable QStyle::StyleHint m_hint = QStyle::SH_CustomBase;
    mutable const QStyleOption *m_option = nullptr;
    mutable const QWidget *m_widget = nullptr;
    mutable QStyleHintReturn *m_returnData = nullptr;
};

class KisMenuStyleDontCloseOnAltContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorClonesNamedBaseStyle();
    void altNavigationHintIsDisabled();
    void otherHintsDelegateToBaseStyle();
};

void KisMenuStyleDontCloseOnAltContractTest::constructorClonesNamedBaseStyle()
{
    std::unique_ptr<QStyle> sourceStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QVERIFY(sourceStyle);
    const QString sourceName = sourceStyle->objectName();

    KisMenuStyleDontCloseOnAlt style(sourceStyle.get());

    QVERIFY(style.baseStyle());
    QVERIFY(style.baseStyle() != sourceStyle.get());
    QCOMPARE(style.baseStyle()->objectName(), sourceName);
}

void KisMenuStyleDontCloseOnAltContractTest::altNavigationHintIsDisabled()
{
    std::unique_ptr<QStyle> sourceStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QVERIFY(sourceStyle);
    KisMenuStyleDontCloseOnAlt style(sourceStyle.get());
    auto *recordingStyle = new RecordingStyle;
    style.setBaseStyle(recordingStyle);

    QStyleOption option;
    QWidget widget;
    QStyleHintReturn returnData;

    QCOMPARE(style.styleHint(QStyle::SH_MenuBar_AltKeyNavigation, &option, &widget, &returnData), 0);
    QCOMPARE(recordingStyle->callCount(), 0);
}

void KisMenuStyleDontCloseOnAltContractTest::otherHintsDelegateToBaseStyle()
{
    std::unique_ptr<QStyle> sourceStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QVERIFY(sourceStyle);
    KisMenuStyleDontCloseOnAlt style(sourceStyle.get());
    auto *recordingStyle = new RecordingStyle;
    style.setBaseStyle(recordingStyle);

    QStyleOption option;
    QWidget widget;
    QStyleHintReturn returnData;

    QCOMPARE(style.styleHint(QStyle::SH_ToolButton_PopupDelay, &option, &widget, &returnData), 731);
    QCOMPARE(recordingStyle->callCount(), 1);
    QCOMPARE(recordingStyle->hint(), QStyle::SH_ToolButton_PopupDelay);
    QCOMPARE(recordingStyle->option(), &option);
    QCOMPARE(recordingStyle->widget(), &widget);
    QCOMPARE(recordingStyle->returnData(), &returnData);
}

QTEST_MAIN(KisMenuStyleDontCloseOnAltContractTest)

#include "KisMenuStyleDontCloseOnAltContractTest.moc"
