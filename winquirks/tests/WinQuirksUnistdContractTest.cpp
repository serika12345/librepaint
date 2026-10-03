/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <unistd.h>

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <iostream>

namespace
{
int fail(const char *message)
{
    std::cerr << message << '\n';
    return 1;
}
}

int main()
{
    if (getpid() <= 0 || getpid() != _getpid()) {
        return fail("getpid() must expose the current Windows process identifier");
    }

    const uid_t unsupportedUserId = static_cast<uid_t>(-2);
    if (getuid() != unsupportedUserId || geteuid() != unsupportedUserId) {
        return fail("Windows user identifiers must use the documented unsupported sentinel");
    }

    if (STDIN_FILENO != 0 || STDOUT_FILENO != 1 || STDERR_FILENO != 2) {
        return fail("standard stream identifiers must match the Windows CRT descriptors");
    }

    char buffer[256] = {};
    errno = 0;
    if (readlink(nullptr, buffer, sizeof(buffer)) != -1 || errno != EINVAL) {
        return fail("readlink() must reject a null path with EINVAL");
    }

    errno = 0;
    if (readlink("abcd", buffer, 4) != -1 || errno != ENAMETOOLONG) {
        return fail("readlink() must reject a path that does not fit in the output buffer");
    }

    const char *missingPath = "winquirks-missing-link-target";
    std::remove(missingPath);
    errno = 0;
    if (readlink(missingPath, buffer, sizeof(buffer)) != -1 || errno != ENOENT) {
        return fail("readlink() must report ENOENT for a missing path");
    }

    const auto startedAt = std::chrono::steady_clock::now();
    if (sleep(1) != 0) {
        return fail("sleep() must report successful completion");
    }
    const auto elapsed = std::chrono::steady_clock::now() - startedAt;
    if (elapsed < std::chrono::milliseconds(900)) {
        return fail("sleep() must wait for the requested interval");
    }

    return 0;
}
