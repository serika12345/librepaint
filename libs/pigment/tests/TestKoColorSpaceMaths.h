/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TESTKOCOLORSPACEMATHS_H
#define TESTKOCOLORSPACEMATHS_H

#include <QObject>

class TestKoColorSpaceMaths : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void numericTraitsAndLutsExposeStableDomains();
    void coreMathPreservesConversionsAndSpecializedRounding();
    void boundsCompositeAndFuzzyPoliciesPreserveThresholds();
    void arithmeticFacadePreservesDelegationAndBounds();
    void colorModelPoliciesPreserveLightnessAndSaturation();
    void hueAndRgbPreservePrimarySectors();
    void toneAndComponentAdjustmentsPreserveMappedValues();
};

#endif
