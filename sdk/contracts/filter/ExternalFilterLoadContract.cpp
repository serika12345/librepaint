/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QPluginLoader>
#include <QApplication>
#include <QStandardPaths>
#include <QTest>

class ExternalFilterLoadContract : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void independentlyLoadsTheInstalledModule()
    {
        const auto path = qEnvironmentVariable("LIBREPAINT_EXTERNAL_FILTER");
        QVERIFY2(!path.isEmpty(), "Set LIBREPAINT_EXTERNAL_FILTER to the separately rebuilt filter");
        QPluginLoader plugin(path);
        QVERIFY2(plugin.instance(), qPrintable(plugin.errorString()));
    }
};

int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    QApplication application(argc, argv);
    ExternalFilterLoadContract test;
    return QTest::qExec(&test, argc, argv);
}
#include "ExternalFilterLoadContract.moc"
