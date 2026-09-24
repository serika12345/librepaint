/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <vector>

int kis_qtest_main(int argc, char **argv);

#ifdef main
#undef main
#endif

extern "C" __attribute__((visibility("default"))) int main(int argc, char **argv)
{
    const QByteArray filesDirectory = qgetenv("LIBREPAINT_ANDROID_TEST_FILES");
    if (filesDirectory.isEmpty()) {
        return 1;
    }
    if (!QDir::setCurrent(QString::fromUtf8(filesDirectory))) {
        return 1;
    }

    QByteArray outputOption("-o");
    QByteArray resultArgument = filesDirectory + "/result.xml,xunitxml";
    std::vector<char *> testArguments;
    testArguments.reserve(static_cast<std::size_t>(argc) + 3);
    for (int index = 0; index < argc; ++index) {
        testArguments.push_back(argv[index]);
    }
    testArguments.push_back(outputOption.data());
    testArguments.push_back(resultArgument.data());
    testArguments.push_back(nullptr);

    const int testArgumentCount = static_cast<int>(testArguments.size()) - 1;
    const int result = kis_qtest_main(testArgumentCount, testArguments.data());

    QFile statusFile(QString::fromUtf8(filesDirectory + "/status"));
    if (statusFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream stream(&statusFile);
        stream << result << Qt::endl;
    }

    return result;
}
