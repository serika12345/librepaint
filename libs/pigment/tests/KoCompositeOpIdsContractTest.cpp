/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoCompositeOpIds.h"

#include <QTest>

class KoCompositeOpIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compositeOperationIdsRemainStable();
};

void KoCompositeOpIdsContractTest::compositeOperationIdsRemainStable()
{
    QCOMPARE(COMPOSITE_OVER, QStringLiteral("normal"));
    QCOMPARE(COMPOSITE_ERASE, QStringLiteral("erase"));
    QCOMPARE(COMPOSITE_IN, QStringLiteral("in"));
    QCOMPARE(COMPOSITE_OUT, QStringLiteral("out"));
    QCOMPARE(COMPOSITE_ALPHA_DARKEN, QStringLiteral("alphadarken"));
    QCOMPARE(COMPOSITE_DESTINATION_IN, QStringLiteral("destination-in"));
    QCOMPARE(COMPOSITE_DESTINATION_ATOP, QStringLiteral("destination-atop"));
    QCOMPARE(COMPOSITE_XOR, QStringLiteral("xor"));
    QCOMPARE(COMPOSITE_OR, QStringLiteral("or"));
    QCOMPARE(COMPOSITE_AND, QStringLiteral("and"));
    QCOMPARE(COMPOSITE_NAND, QStringLiteral("nand"));
    QCOMPARE(COMPOSITE_NOR, QStringLiteral("nor"));
    QCOMPARE(COMPOSITE_XNOR, QStringLiteral("xnor"));
    QCOMPARE(COMPOSITE_IMPLICATION, QStringLiteral("implication"));
    QCOMPARE(COMPOSITE_NOT_IMPLICATION, QStringLiteral("not_implication"));
    QCOMPARE(COMPOSITE_CONVERSE, QStringLiteral("converse"));
    QCOMPARE(COMPOSITE_NOT_CONVERSE, QStringLiteral("not_converse"));
    QCOMPARE(COMPOSITE_PLUS, QStringLiteral("plus"));
    QCOMPARE(COMPOSITE_MINUS, QStringLiteral("minus"));
    QCOMPARE(COMPOSITE_ADD, QStringLiteral("add"));
    QCOMPARE(COMPOSITE_SUBTRACT, QStringLiteral("subtract"));
    QCOMPARE(COMPOSITE_INVERSE_SUBTRACT, QStringLiteral("inverse_subtract"));
    QCOMPARE(COMPOSITE_DIFF, QStringLiteral("diff"));
    QCOMPARE(COMPOSITE_MULT, QStringLiteral("multiply"));
    QCOMPARE(COMPOSITE_DIVIDE, QStringLiteral("divide"));
    QCOMPARE(COMPOSITE_ARC_TANGENT, QStringLiteral("arc_tangent"));
    QCOMPARE(COMPOSITE_GEOMETRIC_MEAN, QStringLiteral("geometric_mean"));
    QCOMPARE(COMPOSITE_ADDITIVE_SUBTRACTIVE, QStringLiteral("additive_subtractive"));
    QCOMPARE(COMPOSITE_NEGATION, QStringLiteral("negation"));
    QCOMPARE(COMPOSITE_MOD, QStringLiteral("modulo"));
    QCOMPARE(COMPOSITE_MOD_CON, QStringLiteral("modulo_continuous"));
    QCOMPARE(COMPOSITE_DIVISIVE_MOD, QStringLiteral("divisive_modulo"));
    QCOMPARE(COMPOSITE_DIVISIVE_MOD_CON, QStringLiteral("divisive_modulo_continuous"));
    QCOMPARE(COMPOSITE_MODULO_SHIFT, QStringLiteral("modulo_shift"));
    QCOMPARE(COMPOSITE_MODULO_SHIFT_CON, QStringLiteral("modulo_shift_continuous"));
    QCOMPARE(COMPOSITE_EQUIVALENCE, QStringLiteral("equivalence"));
    QCOMPARE(COMPOSITE_ALLANON, QStringLiteral("allanon"));
    QCOMPARE(COMPOSITE_PARALLEL, QStringLiteral("parallel"));
    QCOMPARE(COMPOSITE_GRAIN_MERGE, QStringLiteral("grain_merge"));
    QCOMPARE(COMPOSITE_GRAIN_EXTRACT, QStringLiteral("grain_extract"));
    QCOMPARE(COMPOSITE_EXCLUSION, QStringLiteral("exclusion"));
    QCOMPARE(COMPOSITE_HARD_MIX, QStringLiteral("hard mix"));
    QCOMPARE(COMPOSITE_HARD_MIX_HDR, QStringLiteral("hard_mix_hdr"));
    QCOMPARE(COMPOSITE_HARD_MIX_PHOTOSHOP, QStringLiteral("hard_mix_photoshop"));
    QCOMPARE(COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP, QStringLiteral("hard_mix_softer_photoshop"));
    QCOMPARE(COMPOSITE_OVERLAY, QStringLiteral("overlay"));
    QCOMPARE(COMPOSITE_BEHIND, QStringLiteral("behind"));
    QCOMPARE(COMPOSITE_GREATER, QStringLiteral("greater"));
    QCOMPARE(COMPOSITE_HARD_OVERLAY, QStringLiteral("hard overlay"));
    QCOMPARE(COMPOSITE_HARD_OVERLAY_HDR, QStringLiteral("hard_overlay_hdr"));
    QCOMPARE(COMPOSITE_INTERPOLATION, QStringLiteral("interpolation"));
    QCOMPARE(COMPOSITE_INTERPOLATIONB, QStringLiteral("interpolation 2x"));
    QCOMPARE(COMPOSITE_PENUMBRAA, QStringLiteral("penumbra a"));
    QCOMPARE(COMPOSITE_PENUMBRAB, QStringLiteral("penumbra b"));
    QCOMPARE(COMPOSITE_PENUMBRAC, QStringLiteral("penumbra c"));
    QCOMPARE(COMPOSITE_PENUMBRAD, QStringLiteral("penumbra d"));
    QCOMPARE(COMPOSITE_MARKER, QStringLiteral("marker"));
    QCOMPARE(COMPOSITE_DARKEN, QStringLiteral("darken"));
    QCOMPARE(COMPOSITE_BURN, QStringLiteral("burn"));
    QCOMPARE(COMPOSITE_LINEAR_BURN, QStringLiteral("linear_burn"));
    QCOMPARE(COMPOSITE_GAMMA_DARK, QStringLiteral("gamma_dark"));
    QCOMPARE(COMPOSITE_SHADE_IFS_ILLUSIONS, QStringLiteral("shade_ifs_illusions"));
    QCOMPARE(COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS, QStringLiteral("fog_darken_ifs_illusions"));
    QCOMPARE(COMPOSITE_EASY_BURN, QStringLiteral("easy burn"));
    QCOMPARE(COMPOSITE_LIGHTEN, QStringLiteral("lighten"));
    QCOMPARE(COMPOSITE_DODGE, QStringLiteral("dodge"));
    QCOMPARE(COMPOSITE_DODGE_HDR, QStringLiteral("dodge_hdr"));
    QCOMPARE(COMPOSITE_LINEAR_DODGE, QStringLiteral("linear_dodge"));
    QCOMPARE(COMPOSITE_SCREEN, QStringLiteral("screen"));
    QCOMPARE(COMPOSITE_HARD_LIGHT, QStringLiteral("hard_light"));
    QCOMPARE(COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS, QStringLiteral("soft_light_ifs_illusions"));
    QCOMPARE(COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI, QStringLiteral("soft_light_pegtop_delphi"));
    QCOMPARE(COMPOSITE_SOFT_LIGHT_PHOTOSHOP, QStringLiteral("soft_light"));
    QCOMPARE(COMPOSITE_SOFT_LIGHT_SVG, QStringLiteral("soft_light_svg"));
    QCOMPARE(COMPOSITE_GAMMA_LIGHT, QStringLiteral("gamma_light"));
    QCOMPARE(COMPOSITE_GAMMA_ILLUMINATION, QStringLiteral("gamma_illumination"));
    QCOMPARE(COMPOSITE_VIVID_LIGHT, QStringLiteral("vivid_light"));
    QCOMPARE(COMPOSITE_VIVID_LIGHT_HDR, QStringLiteral("vivid_light_hdr"));
    QCOMPARE(COMPOSITE_FLAT_LIGHT, QStringLiteral("flat_light"));
    QCOMPARE(COMPOSITE_LINEAR_LIGHT, QStringLiteral("linear light"));
    QCOMPARE(COMPOSITE_PIN_LIGHT, QStringLiteral("pin_light"));
    QCOMPARE(COMPOSITE_PNORM_A, QStringLiteral("pnorm_a"));
    QCOMPARE(COMPOSITE_PNORM_B, QStringLiteral("pnorm_b"));
    QCOMPARE(COMPOSITE_SUPER_LIGHT, QStringLiteral("super_light"));
    QCOMPARE(COMPOSITE_TINT_IFS_ILLUSIONS, QStringLiteral("tint_ifs_illusions"));
    QCOMPARE(COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS, QStringLiteral("fog_lighten_ifs_illusions"));
    QCOMPARE(COMPOSITE_EASY_DODGE, QStringLiteral("easy dodge"));
    QCOMPARE(COMPOSITE_LUMINOSITY_SAI, QStringLiteral("luminosity_sai"));
    QCOMPARE(COMPOSITE_HUE, QStringLiteral("hue"));
    QCOMPARE(COMPOSITE_COLOR, QStringLiteral("color"));
    QCOMPARE(COMPOSITE_TINT, QStringLiteral("tint"));
    QCOMPARE(COMPOSITE_SATURATION, QStringLiteral("saturation"));
    QCOMPARE(COMPOSITE_INC_SATURATION, QStringLiteral("inc_saturation"));
    QCOMPARE(COMPOSITE_DEC_SATURATION, QStringLiteral("dec_saturation"));
    QCOMPARE(COMPOSITE_LUMINIZE, QStringLiteral("luminize"));
    QCOMPARE(COMPOSITE_INC_LUMINOSITY, QStringLiteral("inc_luminosity"));
    QCOMPARE(COMPOSITE_DEC_LUMINOSITY, QStringLiteral("dec_luminosity"));
    QCOMPARE(COMPOSITE_HUE_HSV, QStringLiteral("hue_hsv"));
    QCOMPARE(COMPOSITE_COLOR_HSV, QStringLiteral("color_hsv"));
    QCOMPARE(COMPOSITE_SATURATION_HSV, QStringLiteral("saturation_hsv"));
    QCOMPARE(COMPOSITE_INC_SATURATION_HSV, QStringLiteral("inc_saturation_hsv"));
    QCOMPARE(COMPOSITE_DEC_SATURATION_HSV, QStringLiteral("dec_saturation_hsv"));
    QCOMPARE(COMPOSITE_VALUE, QStringLiteral("value"));
    QCOMPARE(COMPOSITE_INC_VALUE, QStringLiteral("inc_value"));
    QCOMPARE(COMPOSITE_DEC_VALUE, QStringLiteral("dec_value"));
    QCOMPARE(COMPOSITE_HUE_HSL, QStringLiteral("hue_hsl"));
    QCOMPARE(COMPOSITE_COLOR_HSL, QStringLiteral("color_hsl"));
    QCOMPARE(COMPOSITE_SATURATION_HSL, QStringLiteral("saturation_hsl"));
    QCOMPARE(COMPOSITE_INC_SATURATION_HSL, QStringLiteral("inc_saturation_hsl"));
    QCOMPARE(COMPOSITE_DEC_SATURATION_HSL, QStringLiteral("dec_saturation_hsl"));
    QCOMPARE(COMPOSITE_LIGHTNESS, QStringLiteral("lightness"));
    QCOMPARE(COMPOSITE_INC_LIGHTNESS, QStringLiteral("inc_lightness"));
    QCOMPARE(COMPOSITE_DEC_LIGHTNESS, QStringLiteral("dec_lightness"));
    QCOMPARE(COMPOSITE_HUE_HSI, QStringLiteral("hue_hsi"));
    QCOMPARE(COMPOSITE_COLOR_HSI, QStringLiteral("color_hsi"));
    QCOMPARE(COMPOSITE_SATURATION_HSI, QStringLiteral("saturation_hsi"));
    QCOMPARE(COMPOSITE_INC_SATURATION_HSI, QStringLiteral("inc_saturation_hsi"));
    QCOMPARE(COMPOSITE_DEC_SATURATION_HSI, QStringLiteral("dec_saturation_hsi"));
    QCOMPARE(COMPOSITE_INTENSITY, QStringLiteral("intensity"));
    QCOMPARE(COMPOSITE_INC_INTENSITY, QStringLiteral("inc_intensity"));
    QCOMPARE(COMPOSITE_DEC_INTENSITY, QStringLiteral("dec_intensity"));
    QCOMPARE(COMPOSITE_COPY, QStringLiteral("copy"));
    QCOMPARE(COMPOSITE_COPY_RED, QStringLiteral("copy_red"));
    QCOMPARE(COMPOSITE_COPY_GREEN, QStringLiteral("copy_green"));
    QCOMPARE(COMPOSITE_COPY_BLUE, QStringLiteral("copy_blue"));
    QCOMPARE(COMPOSITE_TANGENT_NORMALMAP, QStringLiteral("tangent_normalmap"));
    QCOMPARE(COMPOSITE_COLORIZE, QStringLiteral("colorize"));
    QCOMPARE(COMPOSITE_BUMPMAP, QStringLiteral("bumpmap"));
    QCOMPARE(COMPOSITE_COMBINE_NORMAL, QStringLiteral("combine_normal"));
    QCOMPARE(COMPOSITE_CLEAR, QStringLiteral("clear"));
    QCOMPARE(COMPOSITE_DISSOLVE, QStringLiteral("dissolve"));
    QCOMPARE(COMPOSITE_DISPLACE, QStringLiteral("displace"));
    QCOMPARE(COMPOSITE_NO, QStringLiteral("nocomposition"));
    QCOMPARE(COMPOSITE_PASS_THROUGH, QStringLiteral("pass through"));
    QCOMPARE(COMPOSITE_DARKER_COLOR, QStringLiteral("darker color"));
    QCOMPARE(COMPOSITE_LIGHTER_COLOR, QStringLiteral("lighter color"));
    QCOMPARE(COMPOSITE_UNDEF, QStringLiteral("undefined"));
    QCOMPARE(COMPOSITE_REFLECT, QStringLiteral("reflect"));
    QCOMPARE(COMPOSITE_GLOW, QStringLiteral("glow"));
    QCOMPARE(COMPOSITE_FREEZE, QStringLiteral("freeze"));
    QCOMPARE(COMPOSITE_HEAT, QStringLiteral("heat"));
    QCOMPARE(COMPOSITE_GLEAT, QStringLiteral("glow_heat"));
    QCOMPARE(COMPOSITE_HELOW, QStringLiteral("heat_glow"));
    QCOMPARE(COMPOSITE_REEZE, QStringLiteral("reflect_freeze"));
    QCOMPARE(COMPOSITE_FRECT, QStringLiteral("freeze_reflect"));
    QCOMPARE(COMPOSITE_FHYRD, QStringLiteral("heat_glow_freeze_reflect_hybrid"));
    QCOMPARE(COMPOSITE_LAMBERT_LIGHTING, QStringLiteral("lambert_lighting"));
    QCOMPARE(COMPOSITE_LAMBERT_LIGHTING_GAMMA_2_2, QStringLiteral("lambert_lighting_gamma2.2"));
}

QTEST_GUILESS_MAIN(KoCompositeOpIdsContractTest)

#include "KoCompositeOpIdsContractTest.moc"
