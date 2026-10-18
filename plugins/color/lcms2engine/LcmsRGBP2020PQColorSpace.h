/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LCMSRGBP2020PQCOLORSPACE_H
#define LCMSRGBP2020PQCOLORSPACE_H


#include <colorspaces/rgb_u8/RgbU8ColorSpace.h>
#include <colorspaces/rgb_u16/RgbU16ColorSpace.h>

#ifdef HAVE_OPENEXR
#include <colorspaces/rgb_f16/RgbF16ColorSpace.h>
#endif

#include <colorspaces/rgb_f32/RgbF32ColorSpace.h>

#include "KoColorConversionTransformationFactory.h"

#include <LcmsRGBP2020PQColorSpaceTransformation.h>

template <class T>
struct ColorSpaceFromFactory {
};

template<>
struct ColorSpaceFromFactory<RgbU8ColorSpaceFactory> {
  typedef RgbU8ColorSpace type;
};

template<>
struct ColorSpaceFromFactory<RgbU16ColorSpaceFactory> {
  typedef RgbU16ColorSpace type;
};

#ifdef HAVE_OPENEXR
template<>
struct ColorSpaceFromFactory<RgbF16ColorSpaceFactory> {
  typedef RgbF16ColorSpace type;
};
#endif

template<>
struct ColorSpaceFromFactory<RgbF32ColorSpaceFactory> {
  typedef RgbF32ColorSpace type;
};

/**
 *  Define a singly linked list of supported bit depth traits
 */
template<class T> struct NextTrait { using type = void; };
template<> struct NextTrait<KoBgrU8Traits> { using type = KoBgrU16Traits; };

#ifdef HAVE_OPENEXR
template<> struct NextTrait<KoBgrU16Traits> { using type = KoRgbF16Traits; };
template<> struct NextTrait<KoRgbF16Traits> { using type = KoRgbF32Traits; };
#else
template<> struct NextTrait<KoBgrU16Traits> { using type = KoRgbF32Traits; };
#endif

/**
 * Recursively add bit-depths conversions to the color space. We add only
 * **outgoing** conversions for every RGB color space. That is, every color
 * space has exactly three outgoing edges for color conversion.
 */
template<typename ParentColorSpace, typename CurrentTraits>
void addInternalConversion(QList<KoColorConversionTransformationFactory*> &list, const KoColorProfile *targetProfile, CurrentTraits*)
{
    // general case: add a converter and recurse for the next traits
    list << new LcmsScaleRGBP2020PQTransformationFactory<ParentColorSpace, CurrentTraits>(targetProfile);

    using NextTraits = typename NextTrait<CurrentTraits>::type;
    addInternalConversion<ParentColorSpace>(list, targetProfile, static_cast<NextTraits*>(0));
}

template<typename ParentColorSpace>
void addInternalConversion(QList<KoColorConversionTransformationFactory*> &list, const KoColorProfile *targetProfile, typename ParentColorSpace::ColorSpaceTraits*)
{
    // exception: skip adding an edge to the same bit depth

    using CurrentTraits = typename ParentColorSpace::ColorSpaceTraits;
    using NextTraits = typename NextTrait<CurrentTraits>::type;
    addInternalConversion<ParentColorSpace>(list, targetProfile, static_cast<NextTraits*>(0));
}

template<typename ParentColorSpace>
void addInternalConversion(QList<KoColorConversionTransformationFactory*> &,  const KoColorProfile *, void*)
{
    // stop recursion
}

template <class BaseColorSpaceFactory>
class LcmsRGBP2020PQColorSpaceFactoryWrapper : public BaseColorSpaceFactory
{
    typedef typename ColorSpaceFromFactory<BaseColorSpaceFactory>::type RelatedColorSpaceType;
public:
    LcmsRGBP2020PQColorSpaceFactoryWrapper(const KoColorProfile *targetProfile, const KoColorProfile *linearProfile)
        : m_targetProfile(targetProfile), m_linearProfile(linearProfile) {

    }
public:
    bool isHdr() const override {
        return this->colorDepthId() != Integer8BitsColorDepthID;
    }

    QList<KoColorConversionTransformationFactory *> colorConversionLinks() const override
    {
        QList<KoColorConversionTransformationFactory *> list;

        /**
         * We explicitly disable direct conversions to/from integer color spaces, because
         * they may cause the conversion system to choose them as an intermediate
         * color space for the conversion chain, e.g.
         * p709-g10 F32 -> p2020-g10 U16 -> Rec2020-pq U16, which is incorrect and loses
         * all the HDR data
         */
        list << new LcmsFromRGBP2020PQTransformationFactory<RelatedColorSpaceType, KoRgbF32Traits>(m_targetProfile, m_linearProfile);
        list << new LcmsToRGBP2020PQTransformationFactory<RelatedColorSpaceType, KoRgbF32Traits>(m_targetProfile, m_linearProfile);
#ifdef HAVE_OPENEXR
        list << new LcmsFromRGBP2020PQTransformationFactory<RelatedColorSpaceType, KoRgbF16Traits>(m_targetProfile, m_linearProfile);
        list << new LcmsToRGBP2020PQTransformationFactory<RelatedColorSpaceType, KoRgbF16Traits>(m_targetProfile, m_linearProfile);
#endif


        // internally, we can convert to RGB U8 if needed
        addInternalConversion<RelatedColorSpaceType>(list, m_targetProfile, static_cast<KoBgrU8Traits*>(0));

        return list;
    }

private:
    KoColorSpace *createColorSpace(const KoColorProfile *p) const override
    {
        return new RelatedColorSpaceType(this->name(), p->clone());
    }

    const KoColorProfile *m_targetProfile;
    const KoColorProfile *m_linearProfile;
};

#endif // LCMSRGBP2020PQCOLORSPACE_H
