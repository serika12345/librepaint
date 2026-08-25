/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTemporaryFileConfiguration.h"

#include <KConfigGroup>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>

#ifdef Q_OS_MACOS
#include "KisMacosSecurityBookmarkManager.h"
#endif

namespace KritaUtils
{

QString writableTemporaryFileLocation(KConfigGroup config,
                                      const QString &suffix,
                                      const QString &configKey,
                                      bool requestDefault)
{
#ifdef Q_OS_MACOS
    QString location;

    KisMacosSecurityBookmarkManager *bookmarkManager = KisMacosSecurityBookmarkManager::instance();
    if (bookmarkManager->isSandboxed()) {
        QDir sandboxHome = QDir::home();
        if (sandboxHome.cd("tmp")) {
            location = sandboxHome.path();
        }
    } else {
        location = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + '/' + suffix;
    }
#else
    Q_UNUSED(suffix);
    QString location = QDir::tempPath();
#endif

    if (requestDefault) {
        return location;
    }

    const QString configuredLocation = config.readEntry(configKey, location);
    if (!configuredLocation.isEmpty()) {
        location = configuredLocation;
    }

    QString chosenLocation;
    const QStringList proposedLocations {location, QDir::tempPath(), QDir::homePath()};

    for (const QString &proposedLocation : proposedLocations) {
        if (!QFileInfo(proposedLocation).isWritable()) {
            continue;
        }

        QTemporaryFile temporaryFile;
        temporaryFile.setFileTemplate(proposedLocation + '/' + "krita_test_swap_location");
        if (temporaryFile.open() && !temporaryFile.fileName().isEmpty()) {
            chosenLocation = proposedLocation;
            break;
        }
    }

    if (chosenLocation.isEmpty()) {
        qCritical() << "CRITICAL: no writable location for a swap file found! Tried the following paths:"
                    << proposedLocations;
        qCritical() << "CRITICAL: hope I don't crash...";
        chosenLocation = location;
    }

    if (chosenLocation != location) {
        qWarning() << "WARNING: configured swap location is not writable, using a fall-back location"
                   << location << "->" << chosenLocation;
    }

    return chosenLocation;
}

void normalizeSwapFileLocation(KConfigGroup config)
{
#ifdef Q_OS_MACOS
    const QString configuredLocation = config.readEntry("swaplocation", "");
    if (configuredLocation.startsWith("/var/folders/")) {
        config.deleteEntry("swaplocation");
    }
#endif
}

QString writableSwapFileLocation(KConfigGroup config, bool requestDefault)
{
    normalizeSwapFileLocation(config);

    return writableTemporaryFileLocation(config, "swap", "swaplocation", requestDefault);
}

}
