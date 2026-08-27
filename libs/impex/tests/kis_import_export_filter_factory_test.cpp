/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTemporaryDir>
#include <QTest>

#include <KisImportExportFilterRegistry.h>

class KisImportExportFilterFactoryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unregisteredMimeTypeHasNoFilter();
};

void KisImportExportFilterFactoryTest::unregisteredMimeTypeHasNoFilter()
{
    QTemporaryDir pluginDirectory;
    QVERIFY(pluginDirectory.isValid());
    QVERIFY(qputenv("KRITA_PLUGIN_PATH", pluginDirectory.path().toUtf8()));

    QVERIFY(!KisImportExportFilterRegistry::createFilter(QStringLiteral("image/x-unregistered-contract"),
                                                         KisImportExportFilterRegistry::Import));
    QVERIFY(!KisImportExportFilterRegistry::createFilter(QStringLiteral("image/x-unregistered-contract"),
                                                         KisImportExportFilterRegistry::Export));
}

QTEST_GUILESS_MAIN(KisImportExportFilterFactoryTest)

#include "kis_import_export_filter_factory_test.moc"
