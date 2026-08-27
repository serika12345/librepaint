/*
 * SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KO_PLUGIN_LOADER_TEST_H
#define KO_PLUGIN_LOADER_TEST_H

#include <QObject>

class KoPluginLoaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testPluginConfigValidity();
    void testTraderQueries();
    void testEmptyPluginWrapper();
    void testDynamicPluginWrapper();
    void testStaticPluginWrapper();

    void testLoadSinglePlugin_data();
    void testLoadSinglePlugin();
    void testLoadSinglePluginPredicates();
    void testCachedLoad();

    void testLoadAll_data();
    void testLoadAll();
};

#endif // KO_PLUGIN_LOADER_TEST_H
