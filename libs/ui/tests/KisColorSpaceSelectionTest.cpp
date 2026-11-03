/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QApplication>
#include <QColorSpace>
#include <QComboBox>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <memory>
#include <type_traits>

#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceEngine.h>
#include <KoColorSpaceRegistry.h>
#include <KoResourcePaths.h>
#include <KoTestConfig.h>
#include <widgets/kis_advanced_color_space_selector.h>
#include <widgets/kis_color_space_selector.h>
#include <widgets/kis_cmb_idlist.h>

namespace {
struct Profiles {
    QByteArray embedded = QColorSpace(QColorSpace::SRgb).iccProfile();
    QByteArray installed = embedded;
    const KoColorProfile *registered = nullptr;
    const KoColorSpace *space = nullptr;
};

Profiles profiles()
{
    Profiles result;
    result.installed.replace(80, 4, QByteArray("LPnt"));
    auto *registry = KoColorSpaceRegistry::instance();
    result.registered = KoColorSpaceEngineRegistry::instance()->get("icc")->addProfile(result.installed);
    const auto *profile = registry->createColorProfileFromData(
        RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), result.embedded);
    if (profile) result.space = registry->rgb8(profile);
    return result;
}

void selectorRows()
{
    QTest::addColumn<bool>("advanced");
    QTest::newRow("basic") << false;
    QTest::newRow("advanced") << true;
}

template<typename Action>
void withSelector(bool advanced, Action action)
{
    if (advanced) {
        auto widget = std::make_unique<KisAdvancedColorSpaceSelector>(nullptr, QStringLiteral("Color profile selection"));
        action(widget);
    } else {
        auto widget = std::make_unique<KisColorSpaceSelector>(nullptr);
        action(widget);
    }
}
}

class KisColorSpaceSelectionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
        qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
        KoResourcePaths::addAssetType("icc_profiles", "data", "/color/icc");
        KoResourcePaths::addAssetType("icc_profiles", "data", "/profiles/");
        QVERIFY(KoColorSpaceRegistry::instance());
        QVERIFY(KoColorSpaceEngineRegistry::instance()->get("icc"));
    }

    void preservesProvidedProfile_data() { selectorRows(); }
    void preservesProvidedProfile()
    {
        QFETCH(bool, advanced);
        const auto input = profiles();
        QVERIFY(input.registered);
        QVERIFY(input.space);
        QCOMPARE(input.space->profile()->rawData(), input.embedded);
        withSelector(advanced, [&](auto &widget) {
            using Selector = typename std::decay_t<decltype(widget)>::element_type;
            QList<QByteArray> delivered;
            connect(widget.get(), static_cast<void (Selector::*)(const KoColorSpace*)>(&Selector::colorSpaceChanged),
                    this, [&](const KoColorSpace *space) {
                delivered.append(space ? space->profile()->rawData() : QByteArray());
            });
            widget->setCurrentColorSpace(input.space);
            QVERIFY(widget->currentColorSpace());
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.embedded);
            QVERIFY(!delivered.isEmpty());
            for (const auto &data : delivered) QCOMPARE(data, input.embedded);
            QCOMPARE(KoColorSpaceRegistry::instance()->profileByName(input.registered->name())->rawData(), input.installed);
        });
    }

    void namedSelectionAndAutomaticDepthDefaults_data() { selectorRows(); }
    void namedSelectionAndAutomaticDepthDefaults()
    {
        QFETCH(bool, advanced);
        const auto input = profiles();
        QVERIFY(input.space);
        withSelector(advanced, [&](auto &widget) {
            widget->setCurrentColorSpace(input.space);
            widget->setCurrentProfile(QStringLiteral("Missing profile for selection test"));
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.embedded);
            widget->setCurrentProfile(input.registered->name());
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.installed);
            widget->setCurrentColorDepth(Integer16BitsColorDepthID);
            const auto *expected = KoColorSpaceRegistry::instance()->colorSpace(
                RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), QString());
            QVERIFY(expected);
            QCOMPARE(widget->currentColorSpace()->colorDepthId(), Integer16BitsColorDepthID);
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), expected->profile()->rawData());
            QCOMPARE(input.space->profile()->rawData(), input.embedded);
        });
    }

    void manualProfileDepthPreferences_data() { selectorRows(); }
    void manualProfileDepthPreferences()
    {
        QFETCH(bool, advanced);
        const auto input = profiles();
        QVERIFY(input.space);
        withSelector(advanced, [&](auto &widget) {
            widget->setCurrentColorSpace(input.space);
            using Selector = typename std::decay_t<decltype(widget)>::element_type;
            if constexpr (std::is_same_v<Selector, KisColorSpaceSelector>) {
                auto *profiles = widget->template findChild<QComboBox*>(QStringLiteral("cmbProfile"));
                QVERIFY(profiles);
                QTest::keyClick(profiles, Qt::Key_Up);
                QTest::keyClick(profiles, Qt::Key_Down);
            } else {
                auto *profiles = widget->template findChild<QListWidget*>(QStringLiteral("lstProfile"));
                QVERIFY(profiles);
                QTest::keyClick(profiles, Qt::Key_Up);
                QTest::keyClick(profiles, Qt::Key_Down);
            }
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.embedded);
            auto *depths = widget->template findChild<KisCmbIDList*>(QStringLiteral("cmbColorDepth"));
            QVERIFY(depths);
            const auto supported = KoColorSpaceRegistry::instance()->colorDepthList(
                RGBAColorModelID, KoColorSpaceRegistry::OnlyUserVisible);
            QVERIFY(supported.contains(Integer16BitsColorDepthID));
            // Use the normal activated signal from keyboard input to rebuild the profile choices.
            QTest::keyClick(depths, Qt::Key_Home);
            for (int i = 1; i < depths->count() && depths->currentItem() != Integer16BitsColorDepthID; ++i) {
                QTest::keyClick(depths, Qt::Key_Down);
            }
            QCOMPARE(depths->currentItem(), Integer16BitsColorDepthID);
            QCOMPARE(widget->currentColorSpace()->colorDepthId(), Integer16BitsColorDepthID);
            const auto *expected = advanced
                ? KoColorSpaceRegistry::instance()->rgb16()
                : KoColorSpaceRegistry::instance()->rgb16(input.space->profile());
            QVERIFY(expected);
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), expected->profile()->rawData());
        });
    }

    void selectedProfileSurvivesUnregistration_data() { selectorRows(); }
    void selectedProfileSurvivesUnregistration()
    {
        QFETCH(bool, advanced);
        const auto input = profiles();
        QVERIFY(input.space);
        withSelector(advanced, [&](auto &widget) {
            widget->setCurrentColorSpace(input.space);
            widget->setCurrentProfile(input.registered->name());
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.installed);
            const QString name = input.registered->name();
            auto *registry = KoColorSpaceRegistry::instance();
            registry->removeProfile(const_cast<KoColorProfile*>(input.registered));
            delete input.registered;
            const auto *namedAfterRemoval = registry->profileByName(name);
            QVERIFY(widget->currentColorSpace());
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.installed);
            QCOMPARE(registry->profileByName(name), namedAfterRemoval);
        });
    }

    void nestedSelectionKeepsFinalNotificationCoherent_data() { selectorRows(); }
    void nestedSelectionKeepsFinalNotificationCoherent()
    {
        QFETCH(bool, advanced);
        const auto input = profiles();
        QVERIFY(input.space);
        const auto *replacement = KoColorSpaceRegistry::instance()->rgb8(input.registered);
        QVERIFY(replacement);
        withSelector(advanced, [&](auto &widget) {
            using Selector = typename std::decay_t<decltype(widget)>::element_type;
            QList<QByteArray> delivered;
            bool replacing = false;
            connect(widget.get(), &Selector::selectionChanged, this, [&](bool valid) {
                if (valid && !replacing) {
                    replacing = true;
                    widget->setCurrentColorSpace(replacement);
                }
            });
            connect(widget.get(), static_cast<void (Selector::*)(const KoColorSpace*)>(&Selector::colorSpaceChanged),
                    this, [&](const KoColorSpace *space) { delivered.append(space->profile()->rawData()); });
            widget->setCurrentColorSpace(input.space);
            QCOMPARE(widget->currentColorSpace()->profile()->rawData(), input.installed);
            QVERIFY(!delivered.isEmpty());
            for (const auto &data : delivered) QCOMPARE(data, input.installed);
        });
    }

    void destructionDuringSelectionNotification_data()
    {
        QTest::addColumn<bool>("advanced");
        QTest::addColumn<bool>("validitySignal");
        QTest::newRow("basic-validity") << false << true;
        QTest::newRow("basic-value") << false << false;
        QTest::newRow("advanced-validity") << true << true;
        QTest::newRow("advanced-value") << true << false;
    }
    void destructionDuringSelectionNotification()
    {
        QFETCH(bool, advanced);
        QFETCH(bool, validitySignal);
        const auto input = profiles();
        QVERIFY(input.space);
        withSelector(advanced, [&](auto &widget) {
            using Selector = typename std::decay_t<decltype(widget)>::element_type;
            QPointer<Selector> guard(widget.get());
            if (validitySignal) {
                connect(widget.get(), &Selector::selectionChanged, this, [&](bool valid) {
                    if (valid) widget.reset();
                });
            } else {
                connect(widget.get(), static_cast<void (Selector::*)(const KoColorSpace*)>(&Selector::colorSpaceChanged),
                        this, [&](const KoColorSpace*) { widget.reset(); });
            }
            guard->setCurrentColorSpace(input.space);
            QVERIFY(guard.isNull());
        });
    }

    void acceptingAdvancedSelectionCanDestroyParent()
    {
        const auto input = profiles();
        QVERIFY(input.space);
        auto widget = std::make_unique<KisColorSpaceSelector>(nullptr);
        widget->setCurrentColorSpace(input.space);
        QPointer<KisColorSpaceSelector> guard(widget.get());
        auto *button = widget->findChild<QPushButton*>(QStringLiteral("bnAdvanced"));
        QVERIFY(button);
        bool armed = false;
        connect(widget.get(), &KisColorSpaceSelector::selectionChanged, this, [&](bool valid) {
            if (armed && valid) widget.reset();
        });
        QTimer::singleShot(0, this, [&] {
            QPointer<KisAdvancedColorSpaceSelector> dialog = guard ? guard->findChild<KisAdvancedColorSpaceSelector*>() : nullptr;
            auto close = qScopeGuard([&] { if (dialog) dialog->reject(); });
            QVERIFY(dialog);
            QCOMPARE(dialog->currentColorSpace()->profile()->rawData(), input.embedded);
            dialog->setCurrentProfile(input.registered->name());
            QCOMPARE(dialog->currentColorSpace()->profile()->rawData(), input.installed);
            armed = true;
            dialog->accept();
            close.dismiss();
        });
        button->click();
        QVERIFY(guard.isNull());
    }

    void cancellingAdvancedSelectionKeepsParentValue()
    {
        const auto input = profiles();
        QVERIFY(input.space);
        KisColorSpaceSelector widget(nullptr);
        widget.setCurrentColorSpace(input.space);
        auto *button = widget.findChild<QPushButton*>(QStringLiteral("bnAdvanced"));
        QVERIFY(button);
        QSignalSpy changed(&widget, &KisColorSpaceSelector::selectionChanged);
        QTimer::singleShot(0, this, [&] {
            auto *dialog = widget.findChild<KisAdvancedColorSpaceSelector*>();
            QVERIFY(dialog);
            dialog->setCurrentProfile(input.registered->name());
            dialog->reject();
        });
        button->click();
        QCOMPARE(widget.currentColorSpace()->profile()->rawData(), input.embedded);
        QCOMPARE(changed.count(), 0);
    }

    void destroyingParentDuringAdvancedSelection()
    {
        auto widget = std::make_unique<KisColorSpaceSelector>(nullptr);
        QPointer<KisColorSpaceSelector> guard(widget.get());
        auto *button = widget->findChild<QPushButton*>(QStringLiteral("bnAdvanced"));
        QVERIFY(button);
        QTimer::singleShot(0, this, [&] { widget.reset(); });
        button->click();
        QVERIFY(guard.isNull());
    }
};

QTEST_MAIN(KisColorSpaceSelectionTest)
#include "KisColorSpaceSelectionTest.moc"
