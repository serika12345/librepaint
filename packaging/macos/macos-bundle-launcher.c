/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <limits.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef BUNDLED_EXECUTABLE
#error BUNDLED_EXECUTABLE must name the executable beside this launcher
#endif

static int join_path(char *output, size_t size, const char *base, const char *suffix)
{
    const int length = snprintf(output, size, "%s/%s", base, suffix);
    return length >= 0 && length < (int)size ? 0 : -1;
}

static int prepend_path(const char *name, const char *value)
{
    const char *current = getenv(name);
    const size_t value_length = strlen(value);
    const size_t current_length = current ? strlen(current) : 0;
    char *combined = malloc(value_length + current_length + 2);
    int result;

    if (!combined) {
        return -1;
    }
    if (current_length) {
        snprintf(combined, value_length + current_length + 2, "%s:%s", value, current);
    } else {
        memcpy(combined, value, value_length + 1);
    }
    result = setenv(name, combined, 1);
    free(combined);
    return result;
}

int main(int argc, char **argv)
{
    char unresolved[PATH_MAX];
    char executable[PATH_MAX];
    char macos[PATH_MAX];
    char target[PATH_MAX];
    char resources[PATH_MAX];
    char plugins[PATH_MAX];
    char qml[PATH_MAX];
    char fontconfig[PATH_MAX];
    char *separator;
    uint32_t size = sizeof(unresolved);

    (void)argc;
    if (_NSGetExecutablePath(unresolved, &size) != 0
        || !realpath(unresolved, executable)) {
        fputs("LibrePaint launcher path is too long\n", stderr);
        return 1;
    }
    separator = strrchr(executable, '/');
    if (!separator) {
        perror("realpath");
        return 1;
    }
    *separator = '\0';
    if (snprintf(macos, sizeof(macos), "%s", executable) >= (int)sizeof(macos)
        || join_path(target, sizeof(target), macos, BUNDLED_EXECUTABLE)
        || join_path(resources, sizeof(resources), macos, "../Resources/share")
        || join_path(plugins, sizeof(plugins), macos, "../PlugIns")
        || join_path(qml, sizeof(qml), macos, "../Resources/qml")
        || join_path(fontconfig, sizeof(fontconfig), macos,
                     "../Resources/fontconfig/fonts.conf")
        || prepend_path("XDG_DATA_DIRS", resources)
        || prepend_path("QT_PLUGIN_PATH", plugins)
        || prepend_path("QML2_IMPORT_PATH", qml)
        || prepend_path("QML_IMPORT_PATH", qml)
        || setenv("FONTCONFIG_FILE", fontconfig, 1)) {
        fputs("LibrePaint launcher could not configure bundled resources\n", stderr);
        return 1;
    }

    execv(target, argv);
    perror("execv");
    return 1;
}
