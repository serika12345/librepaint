/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GAUSSIAN_WEIGHTED_SMOOTHING_FIXTURE_P_H
#define KIS_GPU_GAUSSIAN_WEIGHTED_SMOOTHING_FIXTURE_P_H

#include "../KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Test {
struct GaussianWeightedCase
{
    const char *name;
    QList<InputSample> input;
    WeightedSmoothing smoothing;
    QByteArray rgba;
    QList<qsizetype> flushes{};
};

inline QList<GaussianWeightedCase> gaussianWeightedCases()
{
    // CPU tool, shipped Basic-5 preset bd061017..., flow/opacity .57,
    // foreground (17,34,51), transparent128x128 RGBA8 sRGB, four workers.
    // Raw pointer position/time, public pressureToCurve and builder last speed
    // form InputSample; derived weighted positions are not substituted here.
    // CPU input and pixels originate at cd2efcb0; channel-order recipe below.
    // Original BGRA tap: 8bc9d71aeb5f2e9640d5e9688154e2040c2b7283d41d7a4a952dc13644644e3c.
    // Original BGRA variable: ed59b863ac985b0641e35c2fe5dce33628b0c52b59f7791ca3cafe0f97a3b788.
    // Moving-end BGRA: f1b6d3b71965ac268c49f2086d258ee3927f80415f1e7c83b855df819855d64d.
    // Moving-idle BGRA: 09b141425f64557c9178665198da3e7fd09c784448c2e73a393494129d8cb4cc.
    // Their raw elapsed inputs and emitted sensor/speed values are retained.
    // First duplicate-idle speed0 follows the builder first-input contract.
    // Only R/B channel order changes. Stationary3/8 use the maintained single
    // tap invariant; their observed legacy blank is a classified defect.
    const auto tap = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dvBSgJhFIbhFhXSohhFaFOEtQgsNxYtohBpoUQQdP/30n/AbatMnL7ngXMD5x2GmfG3m8y/ut3M/QF7p9td/5obG4/uXzOy9ej+S1uP"
        "7l9zbfPR/dc2H92/5tL2o/s/2350/5pDBaL7nysQ3f9Wgej+cwWi+3sGzO7/qkB0/xcFovs/KRDdf6ZAdP8rBaL7nyoQ239l+9H9nQfM7u/en9vf"
        "e192/zObj+0/tfXY/r735/Zftzmx8cj+7206247s/+Zdb6/7X7T5+KP2j8549uIaGNRZrG1+23W+v5fXwbDNwy/v9f7b1f/r4Lh+m63vdJvn9p96"
        "f7ZZtLnzv95/fT0c1TNcNW4z3twnvMsBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFv1DVsXrlc="));
    const auto variable = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dvZaxNRFMdxlwcRXxqEuuFCEe2DiNAHxQdtfRHpg8Vauri0bm1dG1uTNJMumfzlngNnYLjMZDqTCdxbvh/4YVvbPJzfzexpTM0sNobn"
        "neST5Jtky6Jff5F8lqxLNiRfJZv2r37/QbIqeWuv8/QMvJPT+UfJD8k/SctJV3Io6VtiybEkcn5vR/Ld1sl7e93rTNzr/vV928zoXLMvOXJ6HzjR"
        "n/Wcv2vaNkNf+wUT97L/FcmvnN5b1mnfyWBI+rad6Fja9vq6H5lk6l71r/vpvSHdH2R0Hxf0n+QgtQY6tk+YZepe9d8q+b4/afd5a0CPES8xee/7"
        "38/ovkr/WWvgIZP3vv+jnP4HFROl+l9m8l73383pfpT++84+4ArT97b/wzH0n94PbEseMX0v++8M6X7U/mM7rvgpmZOcowHv+u+Osf9kG6Dv/1nJ"
        "BA14139U0H88Yv/Hdh1I+79GA9713yvov45tgF5vfC65RQNB9h+PeAywSP9B9191DcR2brFk/XM/MLzjv6prIE5dA1iz/hs0ENz5X9YaiAt6j1O/"
        "37TnAbT/8zQQ3PWfk6wFt/MkPbsPrP0/YPre9t+t2H9Rdq3/VZ4D8Lr/lp2n19l9ZN1r5pm89/3XvQ3YSfU/zeS977/MuWBR9lLd67OlF5h8EP3n"
        "Pf9VJu1U9/oM6DOmHlT/o2wH2s77fkEyxdSD6z85HihzTNiy3jfter92P88932D7z/v8hxv9v7/2OaA16z3JPSbuXf8b1lfZddCxtRDZ/qFrz3Ws"
        "2P2dBSdzTNvL/pN+1gs+A5KXP3Y//01G5+lcZtpe959kydbClq2Hpl2327Wvf6c+z7Vc0HmSO0w6mP7rzl2m7HX/k5JXY+r+NhMOYg1clDyusXfu"
        "7Ye5Dm7Yc9lVe3/J9Z1TsQ6uSmbsek1R568lTyQ3mdypXAsT+pym5L4+s2GZ1mN6PaeTnGVKAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACgTv8BKNhINQ=="));
    const auto movingEnd = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7ZvpaxQxGIftBw8sIv2gICJqC15fvBFRRMULr6LFUhXRYvHqahevtdZe2+7lOrP/som+g2HIJNNuZtvK88Dvi0trmefNm2STGRg8PTLw"
        "N49UPqjUVFoqvxyJVDoZieXz9M+0VeqW6P9vWqWSka8qoyp3VXZsgqCI+3F51jVx0l6l+3QdpH+2mVEDSw7/OhPyd17FWHD/7+QZL4mLpmfsxzn9"
        "23pBVg/QmXf4Lw/861PHsRbUv36+s4aHVoCxn06UowfozDhqYMyogV2YC+q/Zjhw9f5OF0lq4KfD/6LD/wvD/0XMBfM/l3IQeuzbaqC+ih7w2vBP"
        "Dwjnf9l49o1A875vTdhw+F/I8D+Z8n8Ge0H813P4DzH2zRpwrQFqGf6nUv6HVfow2BP/cUD/Hc8aIGsOKKf86+zG4Ib071sDzOUY/zpHMdgT/53A"
        "iT1rANs+oGTxfw6DQf3Xe+i/5fBv+z7wjcX/FQwG998ucO1n+nd9F7hs8T9u8X8Tg8H9t3rgP/LsA23+H1v838JgcP/NHvT/aIXnQR8t7nVuYDC4"
        "/3oP/Ce/N2sNUE35f5Xh/xIGC/HfKnD/F+fwv5Cj9+ucxmAh/hsF+jfPArO+B5o13L/NcK8zhMFC/Kd7QFTA2Hf5n844901nJwYL82/uBaMCxn5W"
        "/1+23PsZYe9XmP+qw38j4Bxguw/WctwDKjnc6xzDXhD/Pzw9oBmoB0Q57wN+k7OeUY//7dgL4t+8++dbC8QBx77t+58FcT/mcX8Cc0H9z3j8J30g"
        "Cui+bTn3n8ox7u+rbMVcUP8VGXu+GmissAZix32i9Nxf9nhPcgBrhfjPMw8kibp0b/b+asbZvi0nMVao/+nUfUDfviBewVovve9flHmnlNP9eWwV"
        "4l/vsb90UQPJ3iD2eG+L95Zxvwv3a+8/efdvQubgiuzBqjlroCVpWD5rWP593nGfg7X+2vg3o89aXsp96++yLjezJL17Tj6viNM86wb9s59T73Jk"
        "5Z7Kfgz13L+tHiY972cm88ai5173ZI793Z93/FS2YGdd+E/yRN7B+ZSjDuaNNcSSnOc99/z+aypHVLZhZV36N/NU5gh9Nvte1g1l2ceVZG4flz7/"
        "TOWyvqshjq/r97hVLui5XeUg7/Wvqf9Tq6yBPNHO+3nK674G9um7lAG9P9S9nCe7oWqgT+Wwyu0uvD+QtRtncxu7FvaqnFW5k8P5sMzleh7fzNP7"
        "72qhX2WPyqDKIekRQ1Ij3L8CAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAIDC+A296WbR"));
    const auto movingIdle = qUncompress(QByteArray::fromBase64(
        "AAEAAHja7dvra9NQGIBxwRs4hgwdgnhDRP0ik01E8fpBFMELuHnDKeLU6VTGmPO2bt1c27Rrm/7LvgffYAjJSdxyzqo8P3g/bLC19Dk9Tdp06PjY"
        "+NDv+SRTl+lYJpTpbWK6sf/Vkvkp8zVjZvV+PZDZtw1O6GM8I1PNad/dZPuerp9OYg2sWtbApN6/WzI7qOWk/2uZRZmm4/Zpe0BHb3c5o//7oT/7"
        "0xi1nPQ3j3NNZt3hvm/bA9oyDZkfKf0/x/qbOUCx0vtX9fH38dzP2gNsrwNPY/2vUqz0/uaYL/D03M/q39E1uJjS/2ViDximWqn9Gzmv/T0HE6bc"
        "TtYe8CbRn+MAf/27HvtHxwHfLMeAZu5RzVv/0FH/XsbtmdehSqL/TKL/OO8HeOvf89zf3I+VnOe/mROU+y/7m3PQtUT/6ZT+o5QrtX/g8bXf1r+t"
        "5yPx/lMp/S9RrtT+Dc/9Q0v/RqL/85T+1ylXev92H/TvpJwDPKK/l/6tPuw/m9LezGXKld4/8Hj81y24/09l9D9HudL7p70G+Dz2i/rXcvZ+M6cp"
        "56R/08P7P7a9P35NyHRGez4HdNc/uQd0Pe790fs/5vPIBZnHlv7bKeesf+Cwf5hzjVH0GeArS/uLVHPaP3kuEHp67kev/R8s7c0coprz/o3Y9UBd"
        "D+2jvf+7zISl/U2Kld5/rcAaCB3v+229H89ynvvHKFZ6/4qlf7QGug7bd/Q9/3c57a9Qy0n/eb3uxrYGWhvcA4q0N8ebczntzXcA9lLLSf8X+n5r"
        "LWcNBH+5DxTZ8wO97cmc/kcp5az/3dh3ABoF1kCR44G84/xmwXM9M6eo5LT/Tpkzep1lpeAaaGvjMLEWbJ/rtvRvo/9T0XVna3+SQt7WwS6ZG/re"
        "W73AOlhPaRvoc7sZ+zn5d3VtP2Xpfl/mMFW2ZB0ckXmi62BZr8dbjc2K/t7s3UsFjhviY87xvmRc0xHNBZkBSmzpGtgjc1735wXL9zOjqRbYM6p6"
        "jvcwo/s1mYM8+n21DobNebfMW/0uXt46WEqcS9a1+8fE97iiuS1zVmY/j3Zfr4MBcywmc0c/l52zrIF5XS8TunZM31GdEfPZvXkPX2aQR/afXQ+D"
        "Zq/WliOxruZ3u3mEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGzE"
        "L3Bl4ko="));
    return {
        {"tap", {
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0}
        }, {24, 24, .01, false}, tap},
        {"stationary2", {
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0}
        }, {24, 24, .01, false}, tap},
        {"stationary3", {
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x0.0p+0}
        }, {24, 24, .01, false}, tap},
        {"stationary8", {
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 113, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 157, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 211, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 263, 0x0.0p+0},
            {{0x1.8800000000000p+3, 0x1.b800000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 317, 0x0.0p+0}
        }, {24, 24, .01, false}, tap},
        {"variable-distance-pressure", {
            {{0x1.4000000000000p+3, 0x1.4000000000000p+3}, 0x1.0000000000000p+0, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 7, 0x0.0p+0},
            {{0x1.8000000000000p+3, 0x1.2000000000000p+4}, 0x1.999999999999ap-1, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 41, 0x1.51c3cac664e1ep-7},
            {{0x1.2000000000000p+4, 0x1.c000000000000p+3}, 0x1.3333333333333p-1, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 79, 0x1.3c90d76524e15p-6},
            {{0x1.6000000000000p+4, 0x1.0000000000000p+3}, 0x1.999999999999ap-2, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 113, 0x1.275de403e4e0cp-6},
            {{0x1.c000000000000p+4, 0x1.e000000000000p+3}, 0x1.999999999999ap-3, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 157, 0x1.507fe91c31be4p-6},
            {{0x1.0000000000000p+5, 0x1.6000000000000p+3}, 0x1.999999999999ap-4, 0x0.0p+0, 0x0.0p+0, 0x0.0p+0, 211, 0x1.30ab2c5a3b615p-6}
        }, {24, 48, .01, true}, variable},
        {"moving-end", {
            {{0x1.4000000000000p+3, 0x1.4000000000000p+3}, 1, 0, 0, 0, 7, 0x0.0p+0},
            {{0x1.8000000000000p+3, 0x1.2000000000000p+4}, 1, 0, 0, 0, 41, 0x1.51c3cac664e1ep-7},
            {{0x1.2000000000000p+4, 0x1.c000000000000p+3}, 1, 0, 0, 0, 79, 0x1.3c90d76524e15p-6},
            {{0x1.6000000000000p+4, 0x1.0000000000000p+3}, 1, 0, 0, 0, 113, 0x1.275de403e4e0cp-6},
            {{0x1.c000000000000p+4, 0x1.e000000000000p+3}, 1, 0, 0, 0, 157, 0x1.507fe91c31be4p-6},
            {{0x1.0000000000000p+5, 0x1.6000000000000p+3}, 1, 0, 0, 0, 211, 0x1.30ab2c5a3b615p-6}
        }, {24, 24, .01, false}, movingEnd, {}},
        {"moving-duplicate-idle", {
            {{0x1.4000000000000p+3, 0x1.4000000000000p+3}, 1, 0, 0, 0, 7, 0x0.0p+0},
            {{0x1.4000000000000p+3, 0x1.4000000000000p+3}, 1, 0, 0, 0, 41, 0x0.0p+0},
            {{0x1.2000000000000p+4, 0x1.c000000000000p+3}, 1, 0, 0, 0, 79, 0x1.6e5b7d16657e2p-8},
            {{0x1.6000000000000p+4, 0x1.0000000000000p+3}, 1, 0, 0, 0, 113, 0x1.b92640bc3194ap-7},
            {{0x1.c000000000000p+4, 0x1.e000000000000p+3}, 1, 0, 0, 0, 157, 0x1.c0aa8c2597a85p-7},
            {{0x1.0000000000000p+5, 0x1.6000000000000p+3}, 1, 0, 0, 0, 211, 0x1.c900c2875911ep-7}
        }, {24, 24, .01, false}, movingIdle, {5, 6}},
    };
}

inline GaussianWeightedCase gaussianWeightedImportedPixelCase()
{
    // Normal document KRA comparison: the continued Weighted stroke over a loaded
    // partial-alpha pixel at (-9,5), using the same raw idle boundaries.
    // CPU saved KRA SHA-256 86bb34784d3fed266781e24141f388819f7bd3226af0e0f5af6c926fd5087347
    // contains RGBA (148,47,94,117) there after the continued (83,177,41) stroke
    // with flow/canvas opacity .57; only input positions are translated.
    auto result = gaussianWeightedCases().last();
    result.name = "imported-partial-alpha";
    for (auto &sample : result.input) sample.position += QPointF(.375, -.25);
    result.rgba = QByteArray::fromHex("942f5e75");
    return result;
}
}
#endif
