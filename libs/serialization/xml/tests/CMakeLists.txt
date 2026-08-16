# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: BSD-3-Clause

kis_add_tests(
    TestXmlWriter.cpp
    LINK_LIBRARIES kritaxmlserialization Qt${QT_MAJOR_VERSION}::Test
)
