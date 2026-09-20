/*
 *  SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TestFontLibraryResourceUtils.h"

#include <KoFontLibraryResourceUtils.h>

#include <QTest>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace
{

struct LibraryResource {
    explicit LibraryResource(int *destructionCount, int value = 0)
        : destructionCount(destructionCount)
        , value(value)
    {
    }

    int *destructionCount = nullptr;
    int value = 0;
};

int regularDeleterCalls = 0;

void destroyLibraryResource(LibraryResource *resource)
{
    ++regularDeleterCalls;
    ++*resource->destructionCount;
    delete resource;
}

int checkedDeleterCalls = 0;
int checkedDeleterResult = 0;

int destroyCheckedLibraryResource(LibraryResource *resource)
{
    ++checkedDeleterCalls;
    ++*resource->destructionCount;
    delete resource;
    return checkedDeleterResult;
}

QStringList *capturedWarnings = nullptr;

void captureWarnings(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type == QtWarningMsg && capturedWarnings) {
        capturedWarnings->append(message);
    }
}

class WarningCapture
{
public:
    explicit WarningCapture(QStringList *warnings)
        : m_previousHandler(qInstallMessageHandler(captureWarnings))
    {
        capturedWarnings = warnings;
    }

    ~WarningCapture()
    {
        capturedWarnings = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

private:
    QtMessageHandler m_previousHandler = nullptr;
};

using LibraryResourceSP = KisLibraryResourcePointer<LibraryResource, destroyLibraryResource>;
using CheckedLibraryResourceSP =
    KisLibraryResourcePointerWithSanityCheck<LibraryResource, destroyCheckedLibraryResource>;

} // namespace

void TestFontLibraryResourceUtils::nullPointersDoNotInvokeTheDeleter()
{
    regularDeleterCalls = 0;

    {
        LibraryResourceSP defaultResource;
        LibraryResourceSP explicitNullResource(nullptr);
        QCOMPARE(defaultResource.data(), nullptr);
        QCOMPARE(explicitNullResource.data(), nullptr);
    }

    QCOMPARE(regularDeleterCalls, 0);
}

void TestFontLibraryResourceUtils::copyAssignmentAndMoveReleaseAtTheFinalOwner()
{
    regularDeleterCalls = 0;
    int sharedDestructionCount = 0;
    int replacedDestructionCount = 0;

    {
        LibraryResourceSP original(new LibraryResource(&sharedDestructionCount, 17));
        LibraryResourceSP copy(original);
        LibraryResourceSP assigned(new LibraryResource(&replacedDestructionCount));

        QCOMPARE(original.data(), copy.data());
        QCOMPARE(original->value, 17);

        assigned = copy;
        QCOMPARE(replacedDestructionCount, 1);
        QCOMPARE(assigned.data(), original.data());

        LibraryResourceSP moved(std::move(assigned));
        QCOMPARE(assigned.data(), nullptr);
        QCOMPARE(moved.data(), original.data());

        original.reset();
        copy.reset();
        QCOMPARE(sharedDestructionCount, 0);
    }

    QCOMPARE(sharedDestructionCount, 1);
    QCOMPARE(replacedDestructionCount, 1);
    QCOMPARE(regularDeleterCalls, 2);
}

void TestFontLibraryResourceUtils::resetReleasesThePreviousAndCurrentResources()
{
    regularDeleterCalls = 0;
    int firstDestructionCount = 0;
    int secondDestructionCount = 0;

    LibraryResourceSP resource;
    resource.reset(new LibraryResource(&firstDestructionCount, 1));
    QCOMPARE(resource->value, 1);

    resource.reset(new LibraryResource(&secondDestructionCount, 2));
    QCOMPARE(firstDestructionCount, 1);
    QCOMPARE(secondDestructionCount, 0);
    QCOMPARE(resource->value, 2);

    resource.reset();
    QCOMPARE(resource.data(), nullptr);
    QCOMPARE(secondDestructionCount, 1);

    resource.reset();
    QCOMPARE(regularDeleterCalls, 2);
}

void TestFontLibraryResourceUtils::sanityCheckedDeleterReportsOnlyFailures()
{
    checkedDeleterCalls = 0;
    int successfulDestructionCount = 0;
    int failedDestructionCount = 0;
    QStringList successfulWarnings;
    QStringList failedWarnings;

    checkedDeleterResult = 0;
    {
        WarningCapture capture(&successfulWarnings);
        CheckedLibraryResourceSP resource(new LibraryResource(&successfulDestructionCount));
    }

    checkedDeleterResult = -1;
    {
        WarningCapture capture(&failedWarnings);
        CheckedLibraryResourceSP resource(new LibraryResource(&failedDestructionCount));
    }

    const auto isOwnedFailureWarning = [](const QString &message) {
        return message.contains(QStringLiteral("WARNING: failed to release a library resource"));
    };

    QCOMPARE(successfulDestructionCount, 1);
    QCOMPARE(failedDestructionCount, 1);
    QCOMPARE(checkedDeleterCalls, 2);
    QCOMPARE(std::count_if(successfulWarnings.cbegin(), successfulWarnings.cend(), isOwnedFailureWarning), 0);
    QCOMPARE(std::count_if(failedWarnings.cbegin(), failedWarnings.cend(), isOwnedFailureWarning), 1);
}

void TestFontLibraryResourceUtils::fontconfigAliasesKeepTheirDestroyFunctions()
{
    static_assert(std::is_same_v<FcConfigSP, KisLibraryResourcePointer<FcConfig, FcConfigDestroy>>);
    static_assert(std::is_same_v<FcCharSetSP, KisLibraryResourcePointer<FcCharSet, FcCharSetDestroy>>);
    static_assert(std::is_same_v<FcPatternSP, KisLibraryResourcePointer<FcPattern, FcPatternDestroy>>);
    static_assert(std::is_same_v<FcFontSetSP, KisLibraryResourcePointer<FcFontSet, FcFontSetDestroy>>);
}

void TestFontLibraryResourceUtils::freeTypeAliasesKeepTheirCheckedDestroyFunctions()
{
    static_assert(
        std::is_same_v<FT_LibrarySP,
                       KisLibraryResourcePointerWithSanityCheck<std::remove_pointer_t<FT_Library>, FT_Done_FreeType>>);
    static_assert(
        std::is_same_v<FT_FaceSP,
                       KisLibraryResourcePointerWithSanityCheck<std::remove_pointer_t<FT_Face>, FT_Done_Face>>);
}

void TestFontLibraryResourceUtils::harfBuzzAliasesKeepTheirDestroyFunctions()
{
    static_assert(std::is_same_v<hb_font_t_sp, KisLibraryResourcePointer<hb_font_t, hb_font_destroy>>);
    static_assert(std::is_same_v<hb_face_t_sp, KisLibraryResourcePointer<hb_face_t, hb_face_destroy>>);
    static_assert(std::is_same_v<hb_set_t_sp, KisLibraryResourcePointer<hb_set_t, hb_set_destroy>>);
    static_assert(std::is_same_v<hb_blob_t_sp, KisLibraryResourcePointer<hb_blob_t, hb_blob_destroy>>);
}

QTEST_GUILESS_MAIN(TestFontLibraryResourceUtils)
