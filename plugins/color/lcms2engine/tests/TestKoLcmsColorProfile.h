/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TESTKOLCMSCOLORPROFILE_H
#define TESTKOLCMSCOLORPROFILE_H

#include <QObject>

class TestKoLcmsColorProfile : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void displayStagesRetainActualRgbValues_data();
    void displayStagesRetainActualRgbValues();
    void testConversion();
    void testProofingConversion();
    void builtInDefinitionsKeepCanonicalBytesColorsAndImportedProfiles_data();
    void builtInDefinitionsKeepCanonicalBytesColorsAndImportedProfiles();

};

#endif
