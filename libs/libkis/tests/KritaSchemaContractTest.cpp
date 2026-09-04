/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Krita.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_KRITA_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Krita::method)), signature>)
} // namespace

class KritaSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void kritaTypeLifetimeAndApplicationStateSchemaRemainStable();
    void kritaDocumentAndWindowSignaturesRemainStable();
    void kritaActionExtensionAndNotificationSignaturesRemainStable();
    void kritaColorFilterAndResourceCatalogSignaturesRemainStable();
    void kritaSettingsLocalizationAndConversionSignaturesRemainStable();
};

void KritaSchemaContractTest::kritaTypeLifetimeAndApplicationStateSchemaRemainStable()
{
    static_assert(std::is_class_v<Krita>);
    static_assert(std::is_base_of_v<QObject, Krita>);
    static_assert(std::is_constructible_v<Krita, QObject *>);
    static_assert(std::is_constructible_v<Krita>);
    static_assert(std::has_virtual_destructor_v<Krita>);

    ASSERT_KRITA_SIGNATURE(instance, Krita * (*)());
    ASSERT_KRITA_SIGNATURE(version, QString (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(batchmode, bool (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(setBatchmode, void (Krita::*)(bool));
}

void KritaSchemaContractTest::kritaDocumentAndWindowSignaturesRemainStable()
{
    ASSERT_KRITA_SIGNATURE(documents, QList<Document *> (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(activeDocument, Document * (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(setActiveDocument, void (Krita::*)(Document *));
    ASSERT_KRITA_SIGNATURE(
        createDocument,
        Document * (Krita::*)(int, int, const QString &, const QString &, const QString &, const QString &, double));
    ASSERT_KRITA_SIGNATURE(openDocument, Document * (Krita::*)(const QString &));
    ASSERT_KRITA_SIGNATURE(recentDocuments, QStringList (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(windows, QList<Window *> (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(activeWindow, Window * (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(openWindow, Window * (Krita::*)());
    ASSERT_KRITA_SIGNATURE(views, QList<View *> (Krita::*)() const);
}

void KritaSchemaContractTest::kritaActionExtensionAndNotificationSignaturesRemainStable()
{
    ASSERT_KRITA_SIGNATURE(action, QAction * (Krita::*)(const QString &) const);
    ASSERT_KRITA_SIGNATURE(actions, QList<QAction *> (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(addDockWidgetFactory, void (Krita::*)(DockWidgetFactoryBase *));
    ASSERT_KRITA_SIGNATURE(dockers, QList<QDockWidget *> (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(addExtension, void (Krita::*)(Extension *));
    ASSERT_KRITA_SIGNATURE(extensions, QList<Extension *> (Krita::*)());
    ASSERT_KRITA_SIGNATURE(notifier, Notifier * (Krita::*)() const);
}

void KritaSchemaContractTest::kritaColorFilterAndResourceCatalogSignaturesRemainStable()
{
    using ResourcesSignature = QMap<QString, Resource *> (Krita::*)(QString &) const;

    ASSERT_KRITA_SIGNATURE(colorModels, QStringList (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(colorDepths, QStringList (Krita::*)(const QString &) const);
    ASSERT_KRITA_SIGNATURE(profiles, QStringList (Krita::*)(const QString &, const QString &) const);
    ASSERT_KRITA_SIGNATURE(addProfile, bool (Krita::*)(const QString &));
    ASSERT_KRITA_SIGNATURE(filter, Filter * (Krita::*)(const QString &) const);
    ASSERT_KRITA_SIGNATURE(filters, QStringList (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(filterStrategies, QStringList (Krita::*)() const);
    ASSERT_KRITA_SIGNATURE(resources, ResourcesSignature);
}

void KritaSchemaContractTest::kritaSettingsLocalizationAndConversionSignaturesRemainStable()
{
    ASSERT_KRITA_SIGNATURE(readSetting, QString (Krita::*)(const QString &, const QString &, const QString &));
    ASSERT_KRITA_SIGNATURE(writeSetting, void (Krita::*)(const QString &, const QString &, const QString &));
    ASSERT_KRITA_SIGNATURE(getAppDataLocation, QString (*)());
    ASSERT_KRITA_SIGNATURE(krita_i18n, QString (*)(const QString &));
    ASSERT_KRITA_SIGNATURE(krita_i18nc, QString (*)(const QString &, const QString &));
    ASSERT_KRITA_SIGNATURE(icon, QIcon (Krita::*)(QString &) const);
    ASSERT_KRITA_SIGNATURE(fromVariant, QObject * (*)(const QVariant &));
}

QTEST_APPLESS_MAIN(KritaSchemaContractTest)

#include "KritaSchemaContractTest.moc"
