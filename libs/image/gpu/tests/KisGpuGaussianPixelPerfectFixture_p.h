/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GAUSSIAN_PIXEL_PERFECT_FIXTURE_P_H
#define KIS_GPU_GAUSSIAN_PIXEL_PERFECT_FIXTURE_P_H

#include "../KisGpuOperationDocument.h"
#include <QRect>

namespace LibrePaint::Image::Gpu::Test {
struct GaussianPixelPerfectCase
{
    const char *name;
    QList<InputSample> input;
    QRect extent;
    QByteArray rgba;
};

inline QList<GaussianPixelPerfectCase> gaussianPixelPerfectCases()
{
    // Actual CPU mouse -> builder -> helper -> Basic-5 Gaussian WASH, preset
    // bd061017..., flow/canvas opacity .57, RGB (17,34,51), pressure 1,
    // transparent128x128 RGBA8 sRGB and four drawing workers.
    // Raw inputs and full tile pixels: source 1761c79a (eight cases),
    // source 96154fb3 (centered-tap); only B/R byte order changes below.
    // Raw tap is retained. Static/held-single legacy zero-LINE blanks are
    // classified defects: their corrected singleton keeps the normal centered
    // POINT image on release, using that separate CPU observation.
    // Moving paths retain the CPU's initial .5 spacing and all off-canvas pixels.
    // tap full BGRA SHA-256 661aadddce8cd975364c43118a6c7221c515e0de8280430e74e22a1585b0e7ed
    const auto tap = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dx9S9NRGMdhR5E4KFPLEEvCjLIntAULi4zMgrJC9v7fi/eB3yD6Iyj3O+eeuy74voHzOa09uaUlAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAPg/a7ujjdjj2Cj2Pvap23FsHHse244tO60r0/xO7HXsR2zyD/sQe+gE57b73e7f+OSS+14eM5zo3HQfxA5m0P3PnZQ75YRTt78d+9xD+9/nsSBn"
        "+83Yr57bT/fSiad7jndeqf10L5x8ivbD7jnapMH2FGje/12j9tOtq9Cs/V7j9mUflWjS/nrsZ4L+ZbuKVO+/n6R92VdFqvf/lqh/2bYq1dpvJWtf9laZ"
        "av0PE/Yv7z8M1KnS/zRh/4nPB6q0v5a0vfeD6vRfTdz/QKHe+28m7j9WaCGf+093pFDv/e8l7u81YP/91xP3HynUe/+VxP33FapyB86S9vcecJ3+R0n7"
        "D9Wp0v9RwvYnyiz0c4CnylS9A+Nk/VdUqdp/I1H7Q0Wa3IE3ST739byvTf9hgu8A+nugtndgx/u9C38HnrV4vVe+g+z009yBVxXbn/o/P+UdqPGd8GOv"
        "9VLfgfs9fj7g+z3zcQeWu998mVX38htBW0527u7BancPzi/xWL/jJOf+HpTvDD/ofvfry196n3V/T/wkdsvJXdn7MOjeOyqPD2uxm7EbTgYAAAAAAAAA"
        "AAAAAAAAAIBZuQAPmHjO"));
    // centered-tap full BGRA SHA-256 f65aad86275bab00c6fa09a8d7c348675332c67ab0f3a1273dd0db90ac7428ae
    const auto centered_tap = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dy9qoMwGAbgTm4dnEoHl16BcCYnoaOTkPu/l0ZID+VsBU/81OeBj0JpLeSNNYk/lwsAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAfK99"
        "/DS5ulx9rjHXlGvOlcrrVN7vy+carXaI3O+5hpLzt7V8764Vd5n7rezPaYVatnPTqrvJvl8p97/Va93QuV9zPf8p+3ct279q7XDZt2UMlyrU8jutVg+1"
        "39fK/rMP+B+Ikf+zcva/xwKtf9ixnjHhPuZ4KUCZG26T/xgk/1Eam6zrpUBlnbBu/kOw/AepVMu+CZb9u5wzqpN/FzT/TjqnmPOZCxr3mwdsl/8UNP9J"
        "OlXyn4PmP0unSv4paknH/o/jP8b/mP9j/Y/V87f+rw84/3fu/J3/1wdc/3Pu/F3/pw+4/lcfcP3/ufN3/48+4P4/fcD9v7j/H8//4HOd0PN/9APP/wIA"
        "AAAAAAAAAAAAAAAAAACqeAHQynNz"));
    // flushed-corner full BGRA SHA-256 dea7a695aedd274df5910b56af336bb84b9702284ff97bcb4e62eee15d5ba0bc
    const auto flushed_corner = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dzrS9tQHMZxcW/KYBsWHIgvnIp0003Y3EU2hiCIQzYZOrtLO0WXTjdF6mXtv7/fgV8hC2mSXs6l7vuB5422NuQ5rclJT8bGAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgP5MzK2UJRXJqmRDsi3Z0Wzrz1b1MWX22I3ovCRZlGxKdnvMpj63xJ4cud7HJUv63t4dMDv6"
        "t8bZsyPR/VSf7/cinwdT7OGgu69Y6D2ZCns6yO6XB+z1i6QuOdDsS2qSzymPXWaPB9X9Uh99m16PJGeSa0k7I+b3p5JDyZ4+f4k9H0T3sz32bt7Tv3P6"
        "zkpLcqKfF7M04LX7uz0c41clvwboPS0/Jfdowlv/bwp2b/6XXw25+04iySRtOO9+umD3Pyz1Ho85XpyhFaf9rxXoPnLQvcm5vt4czTjpvlyg+yNH3XdS"
        "09edpiHv5/p1x92bHOtrf5TcoSWr/W/k9N/00H8z9vpvacla96Wc7hseuu+kGtsO5gbs9H8/5xy/5bH/emxb3tGW8/m+yGP3bZ0fjm/PAxobev9Z1/gu"
        "PfcfJbZnjcaG3v9ixrx+23MaKdt1m9acvP+jAPqPUraLeUE3///PAuj/MGW7ntKak+P/ywD6r6dsF8cAbs7/WwH0X03ZLs4D7c//fQqg+2aXcfmBxqzP"
        "/+8F0P9xl/63aWzo/add//Pdf61L/1s0ZmUMJK//X3ns/jxjTmqdtqz0n/z+z7nH/g8y+n9JW9bGQPz7f8eeuj/LuR65QFPW+o9//3ffQ/fmnPNrTv8T"
        "NOVsPvBPAPN9/6wZpCEnY6Cz/ufE87WeZB7RjtM5gW+ez/WT4d4BbsdAZcC1Xf1e40vLExrxMgbmJRcWejd/83vB7rckt2jD2xhY0P/PrSEd4zf0GkPR"
        "NaasAfA/Blb0elyjz8+DC31utcf1xawLD2cMvE58PyzSdfxNXdPf0lzrz071MbU+7yHxjL0e3BhYdXBPGL7nE/65IfcE+r/HwLSF+4Kts/Z/5MbBQ8n7"
        "IdwLbp69OdLjYEavHxa9f4xZ1/uKc7sbNw7MvUIndd7IHCc8l7zQ88fH5p4O3A8YAAAAAAAAAAAAAAAAAAAAAAAAwKD+An0J0HQ="));
    // negative-floor full BGRA SHA-256 5e933796f12c8b4ac25e586e94b7d84cf8caccf4d8af3e55f8f944dec7e07c49
    const auto negative_floor = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7ZyJSxRRHMeN6CBKk8hQM48OKDukLDu0okQLhE5T88wokzSPlDw2XVP37+438AZkmZn3e3Psqnw+8AWR3Z3HfN7MO2cqKgAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACoqqlvuHZPUSW5IOiTPJN0m3t8PJNcltZKjnLFD471B8kTywTEPJfWc"
        "wQPrvUnSE8N7cbz7QwNn9MB4r5R0Ojrul4xLpiSzkkXJssmSZEEyJmk3bQjtw/50f0nyzsH7qOSn5I8yS6YevJfcl5znrO8b91cdvf9y8F6cr5IBk6eS"
        "cxgoq/srDu5/OLpekeQkecm2ZFdSMP/zP+O1E0OSNtM+HMFKydzXKb17fuYcvK8b54WIrBd9x+s7fJK8MXWhEkOZuj8p6VO4H5b8VnpflWxZvPvZCfi+"
        "11cc3HPsu5IT2MrE/33ldb+kdP9X6X1v8gG/M2/uA34Z+hhDpu6+RuH+o3GhcZ+P4d7PWsDvzQaUpxVzqfnXzOlNK91vJXAfdg/w8i2gTG3YS+y+SuF+"
        "rETu/ayE/P5wQNluYzGR/1aFf01ffzMl9wXTdwg6xkxI+VowGdt/t8X9uHJ8V0gx/yKONRxSTsaH7u5PKK79WYX/7ZT9FyKONR1SzscYdfZ/weJ+IKNx"
        "XtxxgD8/GFbeWqw6+W+x+P+i8L+Tkf9cxDFHQ8rbhVUn/9cTjvlyGbmP6gOGjQX9VGFW7f+mxf98icZ7QdmIOO5MRJmZF0pv7Lds8b9bput/PqLMLzCr"
        "9n/VMt9rW9spZJicZe9IVL1lT5HOf30C/7mM/a8n8F+NXZX/Mwn8b2TsfyWBf/Ya6+tAX8z2fzND9zuWurdg8d+MWbX/9pj9/3yG/jct/mcs/i9jVu2/"
        "Lub4f7NMbb+X7xb/TZh1qgO9Meb/smr/txVzjqMW/8wDu/kP2/M7WIb+f87iftn0TaP8sxboXge6Hdf/Vst07U9b3L/FZiz/tSHnc6KEaz9rCv8jFv+d"
        "2Ex9PWC+BGOADYX7GcV+hSuYTFQHOhz2AKW17yev3F84rPB/EouJ68Bjh7HgdoncTynct2Mvs3mhfjPvluY4QOv+l/K5tLOYS7UOXCs6v59D5oRd9wHs"
        "WtZ392bR7EOzub+DsUzqQFVRezASUAdWHfYCbJnPa90PKdz3suabeT2oMe/x8e8DC47tQF45vvMzV/TsZ1R4d0Tp6oH3nHCz5JFkMmJOeMc4z1nWcuP2"
        "9fw0YqWs9eGi5LW5Tw/FeCdEcT9vxME9z/3sn3rgvRuuy3iZdPT+U7Gmszdv2OOxb+vBKfO+uOdmnXYpYO/OnLlPTBQ926+J96zyac70gRpD9inW7Gzp"
        "oa0/sHXgiHnW6HkM709438ehax8avfka825Jb935lclLM9dwy+xNPs4ZAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIA0+A9f+wW3"));
    return {
        {"tap", {
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
        }, {-64, -64, 128, 128}, tap},
        {"static2", {
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0},
        }, {-64, -64, 128, 128}, centered_tap},
        {"static3", {
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x0.0p+0},
        }, {-64, -64, 128, 128}, centered_tap},
        {"static8", {
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 113, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 157, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 211, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 263, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 317, 0x0.0p+0},
        }, {-64, -64, 128, 128}, centered_tap},
        {"held-corner", {
            {{0x1.0000000000000p+2, 0x1.0000000000000p+2}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x1.e9b0c21937e70p-7},
            {{0x1.a666666666666p+3, 0x1.899999999999ap+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x1.0ae722384ae72p-6},
            {{0x1.b99999999999ap+3, 0x1.b99999999999ap+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 113, 0x1.8ffc5aa26c70ep-7},
        }, {-64, -64, 128, 128}, centered_tap},
        {"flushed-corner", {
            {{0x1.0000000000000p+2, 0x1.0000000000000p+2}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x1.e9b0c21937e70p-7},
            {{0x1.a666666666666p+3, 0x1.899999999999ap+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x1.0ae722384ae72p-6},
            {{0x1.4cccccccccccdp+4, 0x1.4cccccccccccdp+4}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 113, 0x1.ff262a806b1e3p-7},
        }, {-64, -64, 128, 128}, flushed_corner},
        {"initial-tile-removed", {
            {{0x1.2c00000000000p+8, 0x1.2c00000000000p+8}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8666666666666p+3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x1.0442b9df18d52p-1},
        }, {-64, -64, 128, 128}, centered_tap},
        {"negative-floor", {
            {{0x1.0000000000000p+2, 0x1.0000000000000p+2}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{-0x1.999999999999ap-3, 0x1.9666666666666p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x1.8bb441bbcc046p-7},
            {{0x1.4cccccccccccdp+4, 0x1.4cccccccccccdp+4}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x1.49686e96a2872p-5},
        }, {-64, -64, 128, 128}, negative_floor},
    };
}
}
#endif
