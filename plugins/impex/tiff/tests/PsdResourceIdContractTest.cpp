/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tiff_psd_resource_record.h"
#include "psd_resource_section.h"

#include <QTest>

#define PSD_RESOURCE_IDS(X) \
    X(UNKNOWN, 0) \
    X(PS2_IMAGE_INFO, 1000) \
    X(MAC_PRINT_INFO, 1001) \
    X(PS2_COLOR_TAB, 1003) \
    X(RESN_INFO, 1005) \
    X(ALPHA_NAMES, 1006) \
    X(DISPLAY_INFO, 1007) \
    X(CAPTION, 1008) \
    X(BORDER_INFO, 1009) \
    X(BACKGROUND_COL, 1010) \
    X(PRINT_FLAGS, 1011) \
    X(GREY_HALFTONE, 1012) \
    X(COLOR_HALFTONE, 1013) \
    X(DUOTONE_HALFTONE, 1014) \
    X(GREY_XFER, 1015) \
    X(COLOR_XFER, 1016) \
    X(DUOTONE_XFER, 1017) \
    X(DUOTONE_INFO, 1018) \
    X(EFFECTIVE_BW, 1019) \
    X(OBSOLETE_01, 1020) \
    X(EPS_OPT, 1021) \
    X(QUICK_MASK, 1022) \
    X(OBSOLETE_02, 1023) \
    X(LAYER_STATE, 1024) \
    X(WORKING_PATH, 1025) \
    X(LAYER_GROUP, 1026) \
    X(OBSOLETE_03, 1027) \
    X(IPTC_NAA_DATA, 1028) \
    X(IMAGE_MODE_RAW, 1029) \
    X(JPEG_QUAL, 1030) \
    X(GRID_GUIDE, 1032) \
    X(THUMB_RES, 1033) \
    X(COPYRIGHT_FLG, 1034) \
    X(URL, 1035) \
    X(THUMB_RES2, 1036) \
    X(GLOBAL_ANGLE, 1037) \
    X(COLOR_SAMPLER, 1038) \
    X(ICC_PROFILE, 1039) \
    X(WATERMARK, 1040) \
    X(ICC_UNTAGGED, 1041) \
    X(EFFECTS_VISIBLE, 1042) \
    X(SPOT_HALFTONE, 1043) \
    X(DOC_IDS, 1044) \
    X(ALPHA_NAMES_UNI, 1045) \
    X(IDX_COL_TAB_CNT, 1046) \
    X(IDX_TRANSPARENT, 1047) \
    X(GLOBAL_ALT, 1049) \
    X(SLICES, 1050) \
    X(WORKFLOW_URL_UNI, 1051) \
    X(JUMP_TO_XPEP, 1052) \
    X(ALPHA_ID, 1053) \
    X(URL_LIST_UNI, 1054) \
    X(VERSION_INFO, 1057) \
    X(EXIF_DATA, 1058) \
    X(EXIF_DATA_3, 1059) \
    X(XMP_DATA, 1060) \
    X(CAPTION_DIGEST, 1061) \
    X(PRINT_SCALE, 1062) \
    X(PIXEL_ASPECT_RATION, 1064) \
    X(LAYER_COMPS, 1065) \
    X(ALTERNATE_DUOTONE, 1066) \
    X(ALTERNATE_SPOT, 1067) \
    X(LAYER_SELECTION_ID, 1069) \
    X(HDR_TONING, 1070) \
    X(CS2_PRINT_INFO, 1071) \
    X(LAYER_GROUP_ENABLED_ID, 1072) \
    X(COLOR_SAMPLERS, 1073) \
    X(MEASUREMENT_SCALE, 1074) \
    X(TIMELINE_INFO, 1075) \
    X(SHEET_DISCLOSURE, 1076) \
    X(CS3_DISPLAY_INFO, 1077) \
    X(ONION_SKINS, 1078) \
    X(COUNT_INFO, 1080) \
    X(CS5_PRINT_INFO, 1082) \
    X(CS5_PRINT_STYLE, 1083) \
    X(CS5_NSPrintInfo, 1084) \
    X(CS5_WIN_DEVMODE, 1085) \
    X(CS6_AUTOSAVE_FILE_PATH, 1086) \
    X(CS6_AUTOSAVE_FORMAT, 1087) \
    X(CC_PATH_SELECTION_SATE, 1088) \
    X(PATH_INFO_FIRST, 2000) \
    X(PATH_INFO_LAST, 2998) \
    X(CLIPPING_PATH, 2999) \
    X(CC_ORIGIN_PATH_INFO, 3000) \
    X(PLUGIN_RESOURCE_START, 4000) \
    X(PLUGIN_RESOURCE_END, 4999) \
    X(IMAGE_READY_VARS, 7000) \
    X(IMAGE_READY_DATA_SETS, 7001) \
    X(LIGHTROOM_WORKFLOW, 8000) \
    X(PRINT_FLAGS_2, 10000)

class PsdResourceIdContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void psdAndTiffResourceIdsRemainCompatible();
};

void PsdResourceIdContractTest::psdAndTiffResourceIdsRemainCompatible()
{
#define CHECK_RESOURCE_ID(name, value) \
    QCOMPARE(int(PSDImageResourceSection::name), value); \
    QCOMPARE(int(KisTiffPsdResourceRecord::name), value); \
    QCOMPARE(int(PSDImageResourceSection::name), int(KisTiffPsdResourceRecord::name));
    PSD_RESOURCE_IDS(CHECK_RESOURCE_ID)
#undef CHECK_RESOURCE_ID
}

QTEST_GUILESS_MAIN(PsdResourceIdContractTest)

#include "PsdResourceIdContractTest.moc"
