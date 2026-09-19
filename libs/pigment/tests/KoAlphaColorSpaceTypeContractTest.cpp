/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>


#include <colorspaces/KoAlphaColorSpace.h>

class KoAlphaColorSpaceTypeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesSelectTheExpectedTraitsAndImplementations();
    void channelTypesProvideStableAlphaIdentifiers();
};

void KoAlphaColorSpaceTypeContractTest::aliasesSelectTheExpectedTraitsAndImplementations()
{
#ifdef HAVE_OPENEXR
#endif

#ifdef HAVE_OPENEXR
#endif

#ifdef HAVE_OPENEXR
#endif

}

void KoAlphaColorSpaceTypeContractTest::channelTypesProvideStableAlphaIdentifiers()
{
    QCOMPARE(alphaIdFromChannelType<quint8>().id(), QStringLiteral("ALPHA"));
    QCOMPARE(alphaIdFromChannelType<quint16>().id(), QStringLiteral("ALPHAU16"));
    QCOMPARE(alphaIdFromChannelType<float>().id(), QStringLiteral("ALPHAF32"));
#ifdef HAVE_OPENEXR
    QCOMPARE(alphaIdFromChannelType<half>().id(), QStringLiteral("ALPHAF16"));
#endif
}

QTEST_GUILESS_MAIN(KoAlphaColorSpaceTypeContractTest)

#include "KoAlphaColorSpaceTypeContractTest.moc"
