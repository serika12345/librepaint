/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_DOCUMENT_H
#define KIS_GPU_OPERATION_DOCUMENT_H

#include <array>
#include <cmath>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QPair>
#include <QPointF>
#include <QRect>
#include <QSharedPointer>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QUuid>
#include <kritaimagegpuoperations_export.h>
#include "KisGpuRasterPixels.h"
#include "KisGpuAffineTransform.h"
#include "KisGpuSelectionMask.h"
#include <NativeLabConversion.h>
#include <variant>
#include <memory>
#include <optional>

namespace LibrePaint::Image::Gpu {

using LayerId = quint64;
class DocumentVersion;
using Snapshot = QSharedPointer<const DocumentVersion>;
enum class LayerMergeMode : quint8 { Down, Selected, FlattenLayer, FlattenImage, NewFromVisible };

/** Native layer-operation inputs resolved by the image consumer. Bounds limit
 * each projection plane's contribution; they include off-canvas native pixels.
 */
struct LayerMergeRequest
{
    Snapshot before;
    QList<LayerId> layers;
    LayerId above = 0;
    QUuid parentUuid;
    QString name;
    QUuid destinationUuid;
    QHash<QUuid, QUuid> copiedUuids;
    LayerMergeMode mode = LayerMergeMode::Selected;
    bool homogeneousComposition = false;
    bool pinnedToTimeline = false;
    int colorLabelIndex = 0;
    QHash<LayerId, QRect> sourceBounds;
    /** Exact destination storage blocks, aligned to the native 64-pixel grid. */
    QList<QRect> allocatedBlocks;
};

/** Frozen predecessor projections used as a paint layer's initial pixels.
 * The renderer evaluates these inputs on its GPU queue. Subsequent paint,
 * transforms, history and external readers use the resulting layer pixels.
 */
struct FrozenLayerProjection
{
    Snapshot source;
    QHash<LayerId, QRect> sourceBounds;
    QRect extent;
    QList<QRect> allocatedBlocks;
    qsizetype retainedBytes = 0;
    bool normalizeRootProjection = false;
};
using FrozenLayerProjectionSP = QSharedPointer<const FrozenLayerProjection>;
enum class SelectionKind : quint8 { Rectangle, Ellipse, Freehand };

enum class RegionFillMode : quint8 { Contiguous, Similar };
enum class RegionFillReference : quint8 { CurrentLayer, AllLayers, ColorLabeledLayers, CurrentProjection };
enum class RegionCoveragePurpose : quint8 { Paint, Selection };

/** Colour-region fill inputs in document coordinates. Difference and coverage
 * use the native fill tool's threshold and opacity-spread rules. */
struct RegionFillSettings
{
    QPoint seed;
    RegionFillMode mode = RegionFillMode::Contiguous;
    RegionFillReference reference = RegionFillReference::CurrentLayer;
    /** Selection tools generate coverage independently of the previous
     * selection; paint consumers intersect their final result with it. */
    RegionCoveragePurpose purpose = RegionCoveragePurpose::Paint;
    /** Visible labelled roots use native normal composition; selected groups
     * retain their descendants. The active paint layer can also be included. */
    QList<int> colorLabels;
    bool includeActiveLayer = false;
    int threshold = 8;
    int opacitySpread = 100;
    bool selectionAsBoundary = false;
    bool antiAlias = false;
    /** Native circular dilation (positive) or erosion (negative), in pixels. */
    int sizeMod = 0;
    /** A colour boundary selects the contiguous component outside its similarity range. */
    std::optional<RgbaPixel> boundaryColor;
    /** Exact native colour definition captured from the document's colour space. */
    LibrePaint::Pigment::Color::NativeLabConversionSP labConversion;
};

/** Frozen reference pixels for a colour-region fill. Replaying history, cloning
 * and recovery evaluate the same predecessor, independently of later writes. */
struct RegionFill
{
    Snapshot source;
    QWeakPointer<const DocumentVersion> preparedFor;
    LayerId layer = 0;
    RegionFillSettings settings;
    /** Finite coverage includes native selection blocks for edge filtering. */
    QSize coverageSize;
    QPoint coverageOrigin;
    QRect bounds;
};
using RegionFillSP = QSharedPointer<const RegionFill>;

/** One participant in an ordinary move. Pixel and transparency-mask locks
 * are resolved by the tool's current node selection before admission.
 */
struct LayerTranslationTarget
{
    LayerId layer = 0;
    bool pixels = true;
    bool mask = false;
};

/** Attributes enabled for one layer by the properties dialog. Component masks
 * select the RGBA bits whose values participate in Undo and Redo.
 */
struct LayerPropertiesSelection
{
    LayerId layer = 0;
    bool name = false;
    bool opacity = false;
    bool visible = false;
    bool locked = false;
    bool colorLabel = false;
    quint8 protectedChannels = 0;
    quint8 projectionChannels = 0;
    bool passThrough = false;
    bool blendMode = false;
};

struct OperationId
{
    QUuid document;
    quint64 sequence = 0;

    bool isNull() const { return document.isNull() || sequence == 0; }
    bool operator==(const OperationId &other) const
    { return document == other.document && sequence == other.sequence; }
    bool operator!=(const OperationId &other) const { return !(*this == other); }
};

struct InputSample
{
    QPointF position;
    qreal pressure = 1;
    qreal xTilt = 0;
    qreal yTilt = 0;
    qreal rotation = 0;
    quint64 timestamp = 0;
    /** Normalized drawing speed supplied by the input interpreter. */
    qreal speed = 0;
    qreal canvasRotation = 0;
    bool canvasMirroredH = false;
    bool canvasMirroredV = false;
    /** Native initial direction when the stroke has no earlier painted dab. */
    qreal initialDrawingAngle = 0;
    /** Native pen-axis calibration in [0, 360) degrees, frozen by the input producer. */
    qreal tiltDirectionOffset = 0;

    bool operator==(const InputSample &other) const
    {
        return position == other.position && pressure == other.pressure &&
            xTilt == other.xTilt && yTilt == other.yTilt &&
            rotation == other.rotation && timestamp == other.timestamp && speed == other.speed &&
            canvasRotation == other.canvasRotation && canvasMirroredH == other.canvasMirroredH &&
            canvasMirroredV == other.canvasMirroredV && initialDrawingAngle == other.initialDrawingAngle &&
            tiltDirectionOffset == other.tiltDirectionOffset;
    }
};

struct WeightedSmoothing
{
    qreal distanceMin = 0;
    qreal distanceMax = 0;
    qreal tailAggressiveness = 0;
    bool smoothPressure = false;

    bool operator==(const WeightedSmoothing &other) const
    {
        return distanceMin == other.distanceMin && distanceMax == other.distanceMax &&
            tailAggressiveness == other.tailAggressiveness && smoothPressure == other.smoothPressure;
    }
};

/** Frozen constant, direct or sampled native pressure response. */
struct PressureResponse
{
    enum class Kind { Constant, Direct, Curve };
    Kind kind = Kind::Constant;
    qreal strength = 1;
    QList<qreal> curve;

    bool operator==(const PressureResponse &other) const
    { return kind == other.kind && strength == other.strength && curve == other.curve; }
};

/** Frozen repeating Alpha8 paper resource and native per-dab strength inputs. */
struct BrushTexture
{
    enum class Mode { Multiply, Subtract, Darken };
    QSize size;
    /** Row-major Alpha8, padded to a four-byte storage word. */
    QByteArray alpha;
    QPoint offset;
    Mode mode = Mode::Multiply;
    bool soft = false;
    qreal strength = 1;
    std::optional<PressureResponse> pressure;
    std::optional<PressureResponse> fade;
    int fadeLength = 1000;
    bool fadePeriodic = false;
    int curveMode = 0;

    bool operator==(const BrushTexture &other) const
    {
        return size == other.size && alpha == other.alpha && offset == other.offset && mode == other.mode &&
            soft == other.soft && strength == other.strength && pressure == other.pressure && fade == other.fade &&
            fadeLength == other.fadeLength && fadePeriodic == other.fadePeriodic && curveMode == other.curveMode;
    }
};

/** One imported image-pyramid level, with a transparent one-pixel border. */
struct ImageBrushLevel
{
    QSize size;
    qreal scale = 1;
    quint32 wordOffset = 0;
    quint32 tipWordStride = 0;

    bool operator==(const ImageBrushLevel &other) const
    { return size == other.size && scale == other.scale && wordOffset == other.wordOffset && tipWordStride == other.tipWordStride; }
};

/** Immutable grayscale image tips. Each packed texel retains straight red and alpha. */
struct ImageBrushTips
{
    QSize originalSize;
    int tipCount = 0;
    bool randomSelection = false;
    QList<ImageBrushLevel> levels;
    QByteArray pixels;

    bool operator==(const ImageBrushTips &other) const
    { return originalSize == other.originalSize && tipCount == other.tipCount && randomSelection == other.randomSelection && levels == other.levels && pixels == other.pixels; }
};

/** Frozen pixel-brush tip and direct or WASH application inputs. */
struct AutoBrush
{
    enum class Mask { GaussianCircle, Circle, Rectangle, Image };
    Mask mask = Mask::GaussianCircle;
    QSharedPointer<const ImageBrushTips> imageTips;
    qreal fade = 1;
    /** Unset uses the horizontal fade for an isotropic tip. */
    std::optional<qreal> verticalFade{};
    qreal ratio = 1;
    bool isotropicSpacing = false;
    bool antialiasEdges = true;
    qreal angle = 0;
    qreal flow = 1;
    /** Direct painting applies each dab to the destination and its selection. */
    bool incremental = false;
    bool creamy = true;
    PressureResponse opacityResponse;
    PressureResponse flowResponse;
    /** Native alpha threshold, applied before the repeating paper mask. */
    std::optional<PressureResponse> sharpnessResponse;
    bool alignOutlinePixels = false;
    int sharpnessSoftness = 0;
    int precisionLevel = 5;
    bool autoPrecision = false;
    /** Checked native rotation with its sensor curve disabled. */
    bool constantRotation = false;
    /** Drawing-direction sensor, with the native absolute-rotation curve. */
    std::optional<PressureResponse> drawingAngleRotation;
    std::optional<PressureResponse> tiltDirectionRotation;
    qreal drawingAngleOffset = 0;
    /** Zero disables the native corner fan; otherwise radians per extra dab. */
    qreal fanCornersStep = 0;
    std::optional<PressureResponse> scatter;
    bool scatterAxisX = true;
    bool scatterAxisY = true;

    bool operator==(const AutoBrush &other) const
    {
        return mask == other.mask && (imageTips == other.imageTips ||
            (imageTips && other.imageTips && *imageTips == *other.imageTips)) && fade == other.fade && verticalFade == other.verticalFade &&
            ratio == other.ratio && isotropicSpacing == other.isotropicSpacing && antialiasEdges == other.antialiasEdges &&
            angle == other.angle && flow == other.flow && incremental == other.incremental && creamy == other.creamy &&
            opacityResponse == other.opacityResponse && flowResponse == other.flowResponse &&
            sharpnessResponse == other.sharpnessResponse && alignOutlinePixels == other.alignOutlinePixels &&
            sharpnessSoftness == other.sharpnessSoftness &&
            precisionLevel == other.precisionLevel && autoPrecision == other.autoPrecision &&
            constantRotation == other.constantRotation && drawingAngleRotation == other.drawingAngleRotation &&
            tiltDirectionRotation == other.tiltDirectionRotation &&
            drawingAngleOffset == other.drawingAngleOffset && fanCornersStep == other.fanCornersStep &&
            scatter == other.scatter && scatterAxisX == other.scatterAxisX && scatterAxisY == other.scatterAxisY;
    }
};

/** RGBA blend modes shared by layer composition, fill and stroke coverage. */
enum class LayerBlendMode : quint8 {
    Normal, Multiply, Screen, Darken, Lighten, Addition, LinearDodge,
    Difference, Exclusion, Subtract, LinearBurn, InverseSubtract,
    GrainMerge, GrainExtract, Overlay, HardLight, PinLight,
    Divide, ColorDodge, ColorBurn, HardMix, VividLight, LinearLight,
    Equivalence, Allanon, HardMixPhotoshop, Erase, Behind,
    CopyRed, CopyGreen, CopyBlue,
    BitwiseXor, BitwiseOr, BitwiseAnd, BitwiseNand, BitwiseNor, BitwiseXnor,
    Implication, NotImplication, Converse, NotConverse, DestinationIn, DestinationAtop,
    Reflect, Glow, Freeze, Heat, GlowHeat, HeatGlow, ReflectFreeze, FreezeReflect, QuadraticHybrid,
    Parallel, HardOverlay, SoftLightPegtopDelphi, Negation, PenumbraA, PenumbraB, FlatLight, HardMixSofterPhotoshop,
    GeometricMean, Interpolation, InterpolationB, PenumbraC, PenumbraD,
    SoftLightIFSIllusions, SoftLightSvg, SoftLightPhotoshop, GammaLight, GammaIllumination,
    PNormA, PNormB, SuperLight, TintIFSIllusions, FogLightenIFSIllusions,
    EasyDodge, GammaDark, ShadeIFSIllusions, FogDarkenIFSIllusions, EasyBurn,
    Modulo, ModuloContinuous, DivisiveModulo, DivisiveModuloContinuous, ModuloShift,
    ModuloShiftContinuous, ArcTangent, AdditiveSubtractive,
    ColorHsy, HueHsy, TintHsy, SaturationHsy,
    IncreaseSaturationHsy, DecreaseSaturationHsy, LightnessHsy, IncreaseLightnessHsy,
    DecreaseLightnessHsy, ColorHsi, HueHsi, SaturationHsi,
    IncreaseSaturationHsi, DecreaseSaturationHsi, LightnessHsi, IncreaseLightnessHsi,
    DecreaseLightnessHsi, ColorHsl, HueHsl, SaturationHsl,
    IncreaseSaturationHsl, DecreaseSaturationHsl, LightnessHsl, IncreaseLightnessHsl,
    DecreaseLightnessHsl, ColorHsv, HueHsv, SaturationHsv,
    IncreaseSaturationHsv, DecreaseSaturationHsv, LightnessHsv, IncreaseLightnessHsv,
    DecreaseLightnessHsv, DarkerColor, LighterColor, TangentNormalMap,
    ReorientedNormalMap, LambertLighting, LambertLightingGamma22,
    Marker = 127, Greater, LuminositySai, Copy
};

/** Frozen marker or pixel-brush settings used by painting and figure callers. */
/** One ordinary Multibrush hand, frozen in tool order before input starts.
 * The angle and reflections retain the native decomposition of the rigid map.
 */
struct BrushTransform
{
    enum class Mapping : quint8 { Identity, Translation, Scale, Affine };
    std::array<double, 6> matrix{1, 0, 0, 1, 0, 0};
    Mapping mapping = Mapping::Identity;
    qreal rotation = 0;
    bool mirroredH = false;
    bool mirroredV = false;
    static constexpr qsizetype MaximumBytes = 64 * 1024 * 1024;
    bool operator==(const BrushTransform &other) const
    { return matrix == other.matrix && mapping == other.mapping && rotation == other.rotation && mirroredH == other.mirroredH && mirroredV == other.mirroredV; }
    QPointF mapPoint(QPointF point) const
    {
        switch (mapping) {
        case Mapping::Identity: return point;
        case Mapping::Translation: return {point.x() + matrix[4], point.y() + matrix[5]};
        case Mapping::Scale: return {matrix[0] * point.x() + matrix[4], matrix[3] * point.y() + matrix[5]};
        case Mapping::Affine: return {matrix[0] * point.x() + matrix[2] * point.y() + matrix[4],
                                      matrix[1] * point.x() + matrix[3] * point.y() + matrix[5]};
        }
        return point;
    }
};

struct Brush
{
    enum class SizeSensor { Pressure, TiltElevation };
    RgbaPixel rgba{0, 0, 0, 255};
    qreal diameter = 10;
    qreal spacing = 0.1;
    bool autoSpacing = false;
    qreal autoSpacingCoefficient = 1;
    PressureResponse sizeResponse{PressureResponse::Kind::Direct, 1, {}};
    SizeSensor sizeSensor = SizeSensor::Pressure;
    /** Canvas opacity in [0, 1], frozen before input is accepted. */
    qreal opacity = 1;
    bool erase = false;
    bool alphaLocked = false;
    /** Alpha8 mask editing composites in native GrayA8 before flattening coverage. */
    bool maskPainting = false;
    /** Stroke coverage blend mode, frozen at stroke start. Erase never blends. */
    LayerBlendMode blendMode = LayerBlendMode::Normal;
    /** Writable R, G, B, A channels in bits 0 through 3, frozen at stroke start. */
    quint8 channelWriteMask = 15;
    bool mirrorHorizontal = false;
    bool mirrorVertical = false;
    /** Absolute document-pixel axes, rounded when input begins. */
    QPoint mirrorCenter;
    /** Pixel-centered freehand input with adjacent corner samples held back. */
    bool pixelPerfect = false;
    /** Basic freehand interpolation retains raw sensors and explicit idle boundaries. */
    bool basicSmoothing = false;
    /** Weighted positions feed basic interpolation; distances are frozen in document pixels. */
    std::optional<WeightedSmoothing> weightedSmoothing{};
    std::optional<AutoBrush> autoBrush{};
    std::optional<BrushTexture> texture{};
    /** Frozen single GrayA8 secondary WASH tip used by additive Charcoal.
     * Its coverage adds to nonzero primary alpha (native linear_dodge).
     * Validation keeps paper resources on the primary brush.
     */
    QSharedPointer<const Brush> additiveMaskingBrush;
    /** Fineliner size multiplies pressure by a unit-strength speed curve. */
    std::optional<PressureResponse> sizeSpeedResponse{};
    /** Empty is the ordinary single hand; each entry otherwise owns its distance/cache state. */
    QList<BrushTransform> transforms{};

    bool operator==(const Brush &other) const
    {
        return rgba == other.rgba && diameter == other.diameter &&
            spacing == other.spacing && autoSpacing == other.autoSpacing &&
            autoSpacingCoefficient == other.autoSpacingCoefficient && sizeResponse == other.sizeResponse && sizeSensor == other.sizeSensor &&
            sizeSpeedResponse == other.sizeSpeedResponse && transforms == other.transforms &&
            opacity == other.opacity && erase == other.erase &&
            alphaLocked == other.alphaLocked && maskPainting == other.maskPainting && blendMode == other.blendMode &&
            channelWriteMask == other.channelWriteMask &&
            mirrorHorizontal == other.mirrorHorizontal && mirrorVertical == other.mirrorVertical &&
            mirrorCenter == other.mirrorCenter && pixelPerfect == other.pixelPerfect &&
            basicSmoothing == other.basicSmoothing && weightedSmoothing == other.weightedSmoothing &&
            autoBrush == other.autoBrush && texture == other.texture &&
            ((!additiveMaskingBrush && !other.additiveMaskingBrush) ||
             (additiveMaskingBrush && other.additiveMaskingBrush && *additiveMaskingBrush == *other.additiveMaskingBrush));
    }
};

enum class ShapeKind { Freehand, Line, Ellipse, Rectangle, RoundedRectangle, Polyline, Polygon, Path };
enum class PathCommand : quint8 { MoveTo, LineTo, CubicTo, Point };

struct Shape
{
    /** Lines retain intermediate sensor values at their projected input positions. */
    ShapeKind kind = ShapeKind::Freehand;
    QPointF first;
    QPointF second;
    qreal rotation = 0;
    QSizeF cornerRadii{0, 0};
    /** Commands consume one input point, or three for the cubic controls and endpoint.
     * Point stamps its input without joining it to the next MoveTo/LineTo pair.
     * Shape tools retain their actual outline path alongside the editable bounds.
     */
    QList<PathCommand> pathCommands{};
    bool windingFill = false;
    /** Half-open input ranges for separate painter-path calls. Each call owns
     * its distance state; line calls share the state outside these ranges.
     * Empty preserves one compound path.
     */
    QList<QPair<qsizetype, qsizetype>> pathCalls{};
};

/** Frozen straight native RGBA pattern resource. The inverse map transforms
 * document pixel coordinates to the repeating pattern; sampled maps use the
 * native four-pixel, alpha-weighted interpolation. Versions retain the value.
 */
struct PatternFill
{
    static constexpr qsizetype MaximumBytes = 64 * 1024 * 1024;

    QSize size;
    QByteArray rgba;
    PixelFormat format = PixelFormat::Rgba8;
    std::array<double, 6> inverse{1, 0, 0, 1, 0, 0};
    bool sampled = false;

    bool isValid() const
    {
        if (size.width() <= 0 || size.height() <= 0 ||
            size.width() > RgbaTile::CoordinateLimit || size.height() > RgbaTile::CoordinateLimit) return false;
        if (!pixelByteCount(format)) return false;
        for (const auto value : inverse) if (!std::isfinite(value)) return false;
        // The shader wraps integral coordinates before indexing the resource.
        // Keep the map within exact binary64 integer representation.
        for (int row = 0; row < 2; ++row)
            if ((std::abs(inverse[row]) + std::abs(inverse[row + 2])) * RgbaTile::CoordinateLimit +
                    std::abs(inverse[row + 4]) > 4503599627370494.0) return false;
        const qint64 bytes = qint64(size.width()) * size.height() * pixelByteCount(format);
        return bytes > 0 && bytes <= MaximumBytes && rgba.size() == bytes;
    }
    qsizetype byteCount() const { return rgba.size(); }
    bool operator==(const PatternFill &other) const {
        return size == other.size && rgba == other.rgba && format == other.format &&
            inverse == other.inverse && sampled == other.sampled;
    }
    bool operator!=(const PatternFill &other) const { return !(*this == other); }
};

/** Frozen gradient resource in native RGBA16 mixing precision. The palette is
 * sampled from the selected color resource, rather than rasterized document
 * pixels. Geometry, repetition, smoothing and quantization run on the GPU.
 */
struct GradientFill
{
    enum class Shape : quint32 { Linear, BiLinear, Radial, Square, Conical, ConicalSymmetric, Spiral, ReverseSpiral };
    enum class Repeat : quint32 { None, Forwards, Alternate };
    static constexpr qsizetype MaximumBytes = 64 * 1024 * 1024;
    QByteArray rgba16;
    /** Native 64 by 64 blue-noise resource, encoded as little-endian u32. */
    QByteArray noise;
    QPointF start, end;
    Shape shape = Shape::Linear;
    Repeat repeat = Repeat::None;
    double antiAliasThreshold = 0.2;
    bool reverse = false;
    bool dither = false;

    qsizetype byteCount() const { return rgba16.size() + noise.size(); }
    bool isValid() const
    {
        if (rgba16.size() < 8 || rgba16.size() % 8 || byteCount() > MaximumBytes ||
            noise.size() != (dither ? 64 * 64 * 4 : 0) ||
            quint32(shape) > quint32(Shape::ReverseSpiral) || quint32(repeat) > quint32(Repeat::Alternate) ||
            !std::isfinite(antiAliasThreshold) || antiAliasThreshold < 0 || antiAliasThreshold > 100) return false;
        for (const double value : {start.x(), start.y(), end.x(), end.y()})
            if (!std::isfinite(value) || std::abs(value) > RgbaTile::CoordinateLimit) return false;
        return start != end;
    }
};

/** Solid figure coverage, separate from the outline brush and its color. A
 * pattern replaces the solid fill color while retaining the same coverage,
 * selection, compositing and channel behavior. The fill blend mode applies to
 * fill-only figures and is frozen with the fill intent.
 */
struct FigureStyle
{
    bool outline = true;
    std::optional<RgbaPixel> fill;
    std::optional<PatternFill> pattern;
    LayerBlendMode fillBlendMode = LayerBlendMode::Normal;
    /** Selection-outline actions paint the whole outline across the selected boundary. */
    bool ignoreSelection = false;
    std::optional<GradientFill> gradient;
};

/** True when this figure contributes filled interior coverage. */
inline bool hasFigureFill(const FigureStyle &style)
{ return style.fill.has_value() || style.pattern.has_value() || style.gradient.has_value(); }

/**
 * The input and intent of one stroke. GPU dabs and pixels are derived values.
 * Immutable blocks preserve accepted input when previews and history share it.
 * Block boundaries carry no drawing semantics.
 */
struct KRITAIMAGEGPUOPERATIONS_EXPORT PaintOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    Brush brush;
    quint64 randomSeed = 0;
    Shape shape;
    QList<QSharedPointer<const QList<InputSample>>> sampleBlocks;
    qsizetype sampleCount = 0;
    FigureStyle figureStyle;
    /** Exact merge rectangle for a canvas fill; brush/figure merges use their allocated source regions. */
    std::optional<QRect> compositionBounds;
    /** GPU-generated contiguous coverage, intersected with the frozen selection. */
    RegionFillSP region;
    /** Native selected-area clear restores transparent default pixels after masking. */
    bool clearsSelectedPixels = false;
    /** Accepted input counts at which an idle timeout completed the current curve. */
    QList<qsizetype> smoothingFlushes{};
    bool smoothingFinished = false;
    /** Document rectangle that limited this operation; empty means no selection.
     * When inverted, the rectangle is a hole and the rest of the image is selected. */
    QRect selectionRect{};
    bool selectionInverted = false;
    SelectionKind selectionKind = SelectionKind::Rectangle;
    /** Frozen arbitrary coverage for SelectionKind::Freehand. */
    SelectionMaskSP selectionMask;

    QList<InputSample> inputSamples() const;
};

/** Whole-tile replacement; retained compressed pixels preserve external results. */
struct TileWriteOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    QList<QSharedPointer<const RasterTile>> tiles;
    /** One external write can preserve different neighbouring history pixels. */
    quint64 writeGroup = 0;
    /** Empty denotes complete tiles; otherwise these areas identify changed pixel blocks. */
    QList<QRect> writtenAreas;
};
using PaintSnapshot = QSharedPointer<const PaintOperation>;
using TileWriteSnapshot = QSharedPointer<const TileWriteOperation>;
/** Integer relocation of all preceding layer pixels, preserving their RGBA values. */
struct TranslationOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    QPoint offset;
    /** Selected-pixel moves capture, clear, translate and merge the frozen
     * source, while retaining the layer's own position and transparency mask.
     */
    bool selectedPixels = false;
    QRect copyRect, clearRect, selectionRect;
    SelectionKind selectionKind = SelectionKind::Rectangle;
    SelectionMaskSP selectionMask;
    bool selectionInverted = false;
};
using TranslationSnapshot = QSharedPointer<const TranslationOperation>;
/** Frozen, staged native resampling of preceding layer pixels. */
struct AffineTransformOperation
{
    OperationId id;
    quint64 version = 0;
    LayerId layer = 0;
    AffinePlanSP plan;
    /** Native 64-pixel allocation after transform, selection clearing and
     * merging. GPU summaries freeze nondefault blocks before publication;
     * selected merges also preserve the native destination extent and retained
     * allocation outside the cleared area.
     */
    std::optional<QList<QRect>> allocatedBlocks;
    bool transformPixels = true;
    bool transformMask = false;
    /** Null transforms the complete layer; otherwise captures and clears the
     * frozen selection, transforms its pixels and merges them normally.
     */
    TranslationSnapshot selection;
};
using AffineSnapshot = QSharedPointer<const AffineTransformOperation>;
struct AffineTransformTarget { LayerId layer = 0; bool pixels = true; bool mask = false; };
struct AffineLayerPlan {
    LayerId layer = 0;
    AffinePlanSP plan;
    SelectionMaskSP mask;
    /** A nonidentity tool gesture can round to an identity pixel map while
     * native selected-pixel capture/clear/merge still has an observable result.
     */
    bool recomposeSelection = false;
};

/** Resets preceding pixels outside the requested rectangle to the layer default. */
struct CropOperation
{
    OperationId id;
    quint64 version = 1;
    LayerId layer = 0;
    QRect rect;
};
using CropSnapshot = QSharedPointer<const CropOperation>;
/** Pixel filter kinds available to GPU paint layers.
 *
 * InvertColors complements stored RGB. Grayscale replaces RGB with the
 * Rec.709 luminance (0.2126 R + 0.7152 G + 0.0722 B) of the stored
 * sRGB-encoded bytes, rounded half up by the integer rule
 * `(2126 R + 7152 G + 722 B + 5000) / 10000`. HsvAdjustment applies the
 * product HSV/HSL Adjustment slider values to stored RGB. All kinds preserve
 * alpha.
 */
enum class FilterKind : quint8 { InvertColors, Grayscale, HsvAdjustment };

/** Product HSV/HSL Adjustment modes, matching the product filter's `type` key.
 *
 * HSV (0), HSL (1) and HSI (2) depend only on the stored RGB value. HSY (3)
 * and YUV (4), and the product's Colorize option, additionally use the luma
 * coefficients of the document color space. Those coefficients are frozen in
 * the accepted operation rather than read per tile.
 */
enum class HsvAdjustmentType : quint8 { Hsv = 0, Hsl = 1, Hsi = 2, Hsy = 3, Yuv = 4 };

/** Frozen product HSV/HSL Adjustment settings.
 *
 * The product keys are `h`, `s`, `v`, `type`, `colorize` and
 * `compatibilityMode`. Hue is degrees, saturation and value are percentages,
 * and zero is the product reset value for all three. The product
 * `defaultConfiguration()` writes type 1 (HSL) and the modern rule, so those
 * are the defaults here. The legacy compatibility mode reproduces the
 * product's pre-4.3 conversion and changes neutral-pixel behavior, so it is
 * frozen with the values.
 *
 * The product Colorize option keeps the `type` key only for slider scaling:
 * types 0 through 3 write the hue slider as 0..360 degrees and type 4 (YUV)
 * writes Yellow-Blue as 0..100 percent. It ignores the mode and the legacy
 * flag in its pixel rule. Colorize, HSY and YUV use the document luma
 * coefficients; `luma` holds the effective original qreal values as binary64,
 * and `lumaFrozen` records that the
 * caller resolved them from the document color space.
 */
struct HsvAdjustment
{
    qint32 hue = 0;
    qint32 saturation = 0;
    qint32 value = 0;
    HsvAdjustmentType type = HsvAdjustmentType::Hsl;
    bool compatibilityMode = false;
    bool colorize = false;
    std::array<double, 3> luma{0.2126, 0.7152, 0.0722};
    bool lumaFrozen = false;

    bool operator==(const HsvAdjustment &other) const
    {
        return hue == other.hue && saturation == other.saturation && value == other.value &&
            type == other.type && compatibilityMode == other.compatibilityMode &&
            colorize == other.colorize && luma == other.luma && lumaFrozen == other.lumaFrozen;
    }
    bool operator!=(const HsvAdjustment &other) const { return !(*this == other); }
    /** True when the mode, the product slider range and any required luma
     * coefficients make the settings reproducible by the GPU RGBA8 rule.
     */
    bool needsLuma() const
    {
        return colorize || type == HsvAdjustmentType::Hsy || type == HsvAdjustmentType::Yuv;
    }
    bool isValid() const
    {
        if (type > HsvAdjustmentType::Yuv) return false;
        if (colorize) {
            if (hue < 0 || saturation < 0 || saturation > 100 ||
                value < -100 || value > 100) return false;
            if (hue > (type == HsvAdjustmentType::Yuv ? 100 : 360)) return false;
        } else {
            if (saturation < -100 || saturation > 100 ||
                value < -100 || value > 100) return false;
            if (type == HsvAdjustmentType::Yuv) {
                if (hue < -100 || hue > 100) return false;
            } else if (hue < -180 || hue > 180) return false;
        }
        if (!needsLuma()) return true;
        if (!lumaFrozen) return false;
        // Preserve the coefficient range consumed by the binary32 rules.
        for (const float coefficient : luma) {
            if (!std::isfinite(coefficient) || coefficient <= 0.0f || coefficient >= 1.0f)
                return false;
        }
        return true;
    }
};

/** Freezes the original document luma coefficients in an HsvAdjustment and
 * validates that the product rules can use their binary32 conversions. The
 * product transformation substitutes Rec.709 when any supplied coefficient is
 * non-positive, so that fallback is applied here; a missing, non-finite or
 * non-positive-after-rounding coefficient is rejected instead. Coefficients
 * at or above one are rejected because the YUV rule divides by `1 - R` and
 * `1 - B`. Callers pass the document color space coefficients once at apply
 * time; no color-space object is kept by the operation.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT bool freezeLumaCoefficients(
    const std::array<qreal, 3> &coefficients, std::array<double, 3> *frozen);

/** Whole-layer pixel filter over preceding retained pixels.
 *
 * The selection coverage, its inversion and the filter kind are frozen when
 * the operation is accepted. An HSV/HSL Adjustment also freezes its raw
 * product values (`hue`, `saturation`, `value`), its color mode, the legacy
 * compatibility flag, the Colorize flag and the effective document luma
 * coefficients. Only RGB participates: alpha is preserved, and
 * transparent pixels keep alpha zero while their hidden RGB is filtered like
 * the corresponding channel-mask arithmetic. Coverage and settings are
 * retained for every document version and Undo/Redo step.
 */
struct FilterOperation
{
    OperationId id;
    quint64 version = 1;
    LayerId layer = 0;
    FilterKind kind = FilterKind::InvertColors;
    /** Frozen HSV/HSL Adjustment settings; meaningful when kind is HsvAdjustment. */
    qint32 hue = 0;
    qint32 saturation = 0;
    qint32 value = 0;
    HsvAdjustmentType hsvType = HsvAdjustmentType::Hsv;
    bool compatibilityMode = false;
    /** Product Colorize rule, frozen with its mode-dependent slider scaling. */
    bool colorize = false;
    /** Effective document luma coefficients for HSY, YUV and Colorize. */
    std::array<double, 3> luma{0.2126, 0.7152, 0.0722};
    QRect selectionRect{};
    bool selectionInverted = false;
    SelectionKind selectionKind = SelectionKind::Rectangle;
    SelectionMaskSP selectionMask;
};
using FilterSnapshot = QSharedPointer<const FilterOperation>;
using LayerOperation = std::variant<PaintSnapshot, TileWriteSnapshot, TranslationSnapshot, CropSnapshot, FilterSnapshot, AffineSnapshot>;

enum class LayerKind : quint8 { Paint, Group };

struct RasterLayer
{
    QString name;
    bool visible = true;
    QUuid uuid;
    QList<RgbaTile> tiles;
    RgbaPixel defaultPixel{};
    quint8 opacity = 255;
    bool userLocked = false;
    bool pinnedToTimeline = false;
    quint8 channelWriteMask = 15;
    /** Public device position; tile coordinates remain in image space. */
    QPoint position{};
    LayerKind kind = LayerKind::Paint;
    QUuid parentUuid{};
    /** Label identity; its displayed color is owned by the UI configuration. */
    int colorLabelIndex = 0;
    /** RGBA components contributed to the parent projection. */
    quint8 projectionChannelMask = 15;
    /** Groups contribute their descendants directly to the enclosing composition. */
    bool passThrough = false;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
    /** Explicitly written pixel regions, including regions containing only defaults. */
    QList<QRect> writtenAreas{};
    /** Optional document-coordinate alpha coverage applied to this paint
     * layer's pixels during composition. Null means no transparency mask.
     */
    SelectionMaskSP transparencyMask;
    /** False retains the coverage but ignores it during composition. */
    bool transparencyMaskVisible = true;
    bool transparencyMaskLocked = false;
    /** Stable public child identity and document-owned scalar properties. */
    QUuid transparencyMaskUuid{};
    QString transparencyMaskName = QStringLiteral("Transparency Mask");
    int transparencyMaskColorLabelIndex = 0;
    bool transparencyMaskPinnedToTimeline = false;
};
/** Validates parent identities, group parents and cycles. Null identities are
 * assigned during raster import and can only be referenced after assignment.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT bool validRasterLayerHierarchy(const QList<RasterLayer> &layers);
struct RasterImage
{
    QSize size;
    QList<RasterLayer> layers;
    /** Bottom-to-top list index; zero for an empty document. */
    qsizetype activeLayer = 0;
    RgbaPixel projectionBackground{};
    /** Rectangular document selection; empty means no selection. */
    QRect selection{};
    /** When true, the rectangle is a hole and the rest of the image is selected. */
    bool selectionInverted = false;
    SelectionKind selectionKind = SelectionKind::Rectangle;
    /** Arbitrary coverage for SelectionKind::Freehand. */
    SelectionMaskSP selectionMask;
    PixelFormat format = PixelFormat::Rgba8;
    QByteArray colorProfile;
};

struct ConvertedLayerPixels
{
    LayerId layer = 0;
    RgbaPixel defaultPixel{};
    QList<QSharedPointer<const RasterTile>> tiles;
    QList<QRect> writtenAreas;
};

struct KRITAIMAGEGPUOPERATIONS_EXPORT Layer
{
    LayerId id = 0;
    QString name;
    bool visible = true;
    QList<LayerOperation> operations;
    QUuid uuid;
    RgbaPixel defaultPixel{};
    quint8 opacity = 255;
    bool userLocked = false;
    bool pinnedToTimeline = false;
    quint8 channelWriteMask = 15;
    QPoint position{};
    LayerKind kind = LayerKind::Paint;
    QUuid parentUuid{};
    int colorLabelIndex = 0;
    quint8 projectionChannelMask = 15;
    /** Groups contribute their descendants directly to the enclosing composition. */
    bool passThrough = false;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
    /** Optional document-coordinate alpha coverage applied to this paint
     * layer's pixels during composition. Owned by the document version; a CPU
     * mask is never the permanent owner of GPU layer state.
     */
    SelectionMaskSP transparencyMask;
    /** False retains the coverage but ignores it during composition. */
    bool transparencyMaskVisible = true;
    bool transparencyMaskLocked = false;
    /** Stable public child identity and document-owned scalar properties. */
    QUuid transparencyMaskUuid{};
    QString transparencyMaskName = QStringLiteral("Transparency Mask");
    int transparencyMaskColorLabelIndex = 0;
    bool transparencyMaskPinnedToTimeline = false;
    FrozenLayerProjectionSP initialProjection;
    qsizetype inputSampleCount() const;
    qsizetype retainedRasterBytes() const;
};

/** True when this layer's retained transparency mask contributes to composition. */
inline bool hasEnabledTransparencyMask(const Layer &layer)
{ return layer.transparencyMask && layer.transparencyMaskVisible; }

/** One visible layer contributed to a composition boundary, after passing
 * ancestor opacity and immediate-parent channel selection through groups.
 */
struct LayerProjectionInput
{
    LayerId layer = 0;
    quint8 opacity = 255;
    quint8 channelMask = 15;
    LayerBlendMode blendMode = LayerBlendMode::Normal;
};

/** Immutable integer RGBA document version with paint layers and group ownership. */
class KRITAIMAGEGPUOPERATIONS_EXPORT DocumentVersion
{
public:
    QUuid documentId() const { return m_document; }
    quint64 revision() const { return m_revision; }
    /** Identity of this history state, preserved by direct writes outside undo. */
    quint64 historyRevision() const { return m_modifiedRevision; }
    /** Admission state is queried on the OperationDocument owner's thread. */
    bool isCommitted() const { return m_committed; }
    OperationId pendingOperation() const { return m_committed ? OperationId{} : m_pendingOperation; }
    /** Uniform projection values before each node's own opacity. Group values
     * derive from child defaults, order, visibility, opacity and projection components.
     */
    QHash<LayerId, RgbaPixel> projectionDefaultPixels() const;
    /** Bottom-to-top inputs for the root, or one group's isolated subtree.
     * A group boundary excludes its own attributes and all outside state.
     * Unavailable group identities throw std::invalid_argument.
     */
    QList<LayerProjectionInput> projectionInputs(LayerId group = 0) const;
    QSize size;
    PixelFormat format = PixelFormat::Rgba8;
    QByteArray colorProfile;
    RgbaPixel projectionBackground{};
    /** Identity of the public root tree, replaced by Flatten Image and restored by history. */
    QUuid rootUuid;
    QList<Layer> layers;
    LayerId activeLayer = 0;
    QList<LayerId> selectedLayers;
    /** Document selection for this history state; empty means no selection.
     * Inverted selections select the image outside the rectangle. */
    QRect selection{};
    bool selectionInverted = false;
    SelectionKind selectionKind = SelectionKind::Rectangle;
    /** Frozen arbitrary coverage; null for rectangular and elliptical kinds. */
    SelectionMaskSP selectionMask;
    qsizetype sampleCount = 0;
    qsizetype rasterBytes = 0;

    /** Transparent root projections reuse one full-opacity child, preserving hidden RGB. */
    LayerId directProjectionLayer() const
    {
        const auto opacity = format == PixelFormat::Rgba16 ?
            (projectionBackground[3] - (projectionBackground[3] >> 8) + 128) >> 8 : projectionBackground[3];
        if (opacity != 0) return 0;
        const Layer *only = nullptr;
        for (const auto &layer : layers) {
            if (!layer.parentUuid.isNull()) continue;
            if (only) return 0;
            only = &layer;
        }
        return only && only->blendMode == LayerBlendMode::Normal && !only->passThrough && only->visible && only->opacity == 255 && only->projectionChannelMask == 15 && !hasEnabledTransparencyMask(*only) ? only->id : 0;
    }

private:
    friend class OperationDocument;
    friend class PaintImage;
    DocumentVersion() = default;
    DocumentVersion(const DocumentVersion &) = default;
    QUuid m_document;
    quint64 m_revision = 0;
    quint64 m_modifiedRevision = 0;
    // Admission is owned by OperationDocument's thread; snapshot content is immutable.
    mutable bool m_committed = false;
    OperationId m_pendingOperation;
};

/** Frozen layer pixels and fresh destination properties for one prepared node. */
struct LayerInsertion
{
    Snapshot source;
    LayerId sourceLayer = 0;
    RasterLayer properties;
};

struct CanvasResize
{
    QRect rect;
    bool cropLayers = false;
};

enum class Result {
    Success,
    InvalidInput,
    CapacityExceeded,
    StaleOperation,
    StaleVersion,
    LayerUnavailable,
    EditInProgress,
    EmptyOperation,
    NothingToUndo,
    NothingToRedo,
    Closed,
    Busy,
    DeviceFailure
};

/** Consecutive panel adjustments form one undo step; independent edits retain separate steps. */
enum class PropertyHistory { Separate, MergeConsecutive };

/** Domain action for one retained undo step, independent of its current property values. */
enum class HistoryAction {
    None, AddLayer, RemoveLayer, MoveLayer, RenameLayer, ChangeOpacity,
    ChangeVisibility, ChangeLock, ChangeTimelinePin, PaintStroke, DrawLine,
    DrawEllipse, ReplacePixels, DrawRectangle, DrawPolyline, DrawPolygon, ChangeChannelLock, DuplicateLayer, DrawPath,
    ClearPixels, FillLayer, TranslateLayer, CropLayer, ResizeCanvas, CropImage, ChangeProjectionChannels, ChangeLayerProperties, ChangePassThrough, ChangeBlendMode,
    ChangeSelection, AddTransparencyMask, RemoveTransparencyMask, ChangeMaskVisibility, InvertColors,
    Grayscale, HsvAdjustment, ConvertColorSpace, MoveSelectionPixels, TransformLayer,
    GroupLayers, CreateClippingGroup, UngroupLayers,
    MergeDown, MergeSelected, FlattenLayer, FlattenImage, NewFromVisible
};

struct Limits
{
    /** Raw input count multiplied by the number of Multibrush hands. */
    qsizetype samplesPerStroke = 1024 * 1024;
    qsizetype samplesPerDocument = 4 * 1024 * 1024;
    qsizetype layerCount = 4096;
    /** Compressed checkpoints prepared for one atomic external pixel write. */
    qsizetype compressedWriteBytes = 512 * 1024 * 1024;
    qsizetype rasterBytes = 256 * 1024 * 1024;
};

/**
 * Owns retained painting operations, layer hierarchy and the document's history.
 *
 * Call on one owning thread. Snapshots are immutable and can outlive the owner;
 * consumers retain a snapshot for background work. Each accepted append or shape
 * update produces a new preview. Commit completes stationary or smoothed input
 * and adopts its final preview as one history entry; cancel preserves the
 * committed version and redo branch. Undo/redo only
 * select existing versions and execute no paint operation or external effect.
 *
 * Construction requires positive dimensions and limits; invalid values throw
 * std::invalid_argument. Requests validate the complete input before adoption.
 * Rejected requests preserve accepted input and both history branches.
 */
class KRITAIMAGEGPUOPERATIONS_EXPORT OperationDocument
{
public:
    static constexpr int HistoryLimit = 250;
    static constexpr qreal CoordinateLimit = RgbaTile::CoordinateLimit;

    explicit OperationDocument(QSize size, Limits limits = {});
    /** Validated raster import starts saved, with preserved layer UUIDs and empty history. */
    explicit OperationDocument(const RasterImage &image, Limits limits = {});
    /** Captured input is bounded before a producer allocates its retained copy. */
    Limits limits() const { return m_limits; }
    OperationDocument(const OperationDocument &) = delete;
    OperationDocument &operator=(const OperationDocument &) = delete;

    Snapshot snapshot() const;
    Snapshot preview() const;
    LayerId activeLayer() const;
    int undoCount() const;
    int redoCount() const;
    /** Shared immutable versions for the public history display, oldest first. */
    QList<Snapshot> historyVersions() const;
    /** Action reaching the given history position; None for the initial or invalid position. */
    HistoryAction historyAction(int index) const;
    /** Original rectangle and layer identity for resuming a retained crop action. */
    CropSnapshot historyCrop(int index) const;
    std::optional<CanvasResize> historyCanvasResize(int index) const;
    /** -1 when the saved version has left the retained history. */
    int savedHistoryIndex() const;
    bool isModified() const;
    bool hasActiveStroke() const;
    OperationId activeOperation() const;

    /** Limits subsequent painting to a document rectangle. Empty clears the
     * selection. Accepted operations retain the rectangle that was current
     * when their input began, so later selection changes do not rewrite history.
     */
    Result setSelectionRect(const QRect &rect, bool inverted = false);
    Result setSelectionEllipse(const QRect &rect, bool inverted = false);
    /** Freezes arbitrary binary or alpha coverage as one selection history
     * entry. A null mask clears the selection; the mask bounds become
     * selectionRect().
     */
    Result setSelectionMask(const SelectionMaskSP &mask, bool inverted = false);
    /** Applies a tool selection in one history entry; failure preserves the version. */
    Result combineSelectionMask(const SelectionMaskSP &mask, SelectionCombination action);
    Result clearSelection();
    QRect selectionRect() const;
    bool selectionInverted() const;
    SelectionKind selectionKind() const;
    SelectionMaskSP selectionMask() const;

    /** Returns an empty identifier for a closed/busy document or invalid brush/layer. */
    OperationId beginStroke(LayerId layer, const Brush &brush, quint64 randomSeed);
    Result appendSamples(OperationId id, const QList<InputSample> &samples);
    /** Unsmoothed native painter primitives retain independent line starts and
     * sensor times in call order; each line has nondecreasing endpoint times.
     * Each append adopts samples and commands atomically. Mixing a sampled
     * freehand stroke or filled figure into these primitives returns InvalidInput.
     */
    Result appendPoint(OperationId id, const InputSample &point);
    Result appendLine(OperationId id, const InputSample &first, const InputSample &last);
    /** Adopts an ordered batch of Point and MoveTo/LineTo primitives in one
     * preview. Each command consumes one sample; line times are nondecreasing
     * within each pair. Invalid or oversized batches leave all input unchanged.
     */
    Result appendPainterPrimitives(OperationId id, const QList<InputSample> &samples,
                                   const QList<PathCommand> &commands);
    /** Completes basic interpolation at the current input count. Repeated calls
     * preserve the version; other modes return InvalidInput and fewer than two
     * samples return EmptyOperation. Original sensor input remains unchanged.
     */
    Result flushSmoothing(OperationId id);
    Result updateShape(OperationId id, const Shape &shape);
    /** Freezes solid fill and outline intent together with the figure geometry. */
    Result updateFigure(OperationId id, const Shape &shape, const FigureStyle &style);
    /** Validates and commits one complete figure without exposing an intermediate preview. */
    Result paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                       const Shape &shape, const FigureStyle &style, quint64 randomSeed);
    Result commitStroke(OperationId id);
    Result cancelStroke(OperationId id);
    /** Replaces full tiles, including hidden layers, in one history entry. */
    Result replaceTiles(LayerId layer, const QList<RgbaTile> &tiles);
    /** A placement preview freezes its layer and starting version. Updates use
     * an integer offset from the start; commit records one history entry and
     * cancellation retains the committed version and redo branch.
     */
    OperationId beginAffineTransform(const QList<AffineTransformTarget> &layers, QRect copyRect = {}, QRect clearRect = {});
    Result updateAffineTransform(OperationId id, const QList<AffineLayerPlan> &plans,
                                 const SelectionMaskSP &selection = {}, bool inverted = false);
    Result commitAffineTransform(OperationId id);
    Result cancelAffineTransform(OperationId id);
    OperationId beginLayerTranslation(LayerId layer);
    OperationId beginLayerTranslation(const QList<LayerTranslationTarget> &targets);
    /** The view supplies its ordinary selection's coarse copy extent and exact
     * clear extent; coverage and shape are frozen from this document version.
     */
    OperationId beginSelectionTranslation(LayerId layer, QRect copyRect, QRect clearRect);
    Result updateLayerTranslation(OperationId id, QPoint offset);
    Result commitLayerTranslation(OperationId id);
    Result cancelLayerTranslation(OperationId id);
    /** Absolute device position outside undo. Updates the layer's shared
     * history range, retaining pinned versions, recorded placements and saved
     * state. Active input rejects the write without changing either version.
     */
    Result writeLayerPosition(LayerId layer, QPoint position);

    /** Adds and selects a layer in one history entry; returns zero on rejection. */
    LayerId addLayer(const QString &name);
    /** Inserts and selects a paint layer at a document-list index. A nonnull
     * parent UUID identifies an existing group; null places it at the root.
     */
    LayerId addLayer(const QString &name, qsizetype index, bool pinnedToTimeline, const QUuid &parentUuid = {});
    /** Copies one committed paint layer or empty group into a new identity and one history entry.
     * Properties supply a fresh UUID and attributes; pixels retain their source
     * default and immutable blocks. Zero reports rejected input or capacity.
     */
    LayerId insertLayer(const OperationDocument &source, LayerId sourceLayer,
                        const RasterLayer &properties, qsizetype index, bool activate = true);
    /** Adopts a prepared forest in list order as one history entry. Parent UUIDs
     * refer to existing groups or other prepared nodes. Selects its roots and
     * activates the first root; Undo restores the original selection and active
     * layer. clearPixelSelection clears the pixel selection in that same entry.
     * Rejected requests retain the complete document and input state.
     */
    Result insertLayers(const QList<LayerInsertion> &layers, qsizetype index, bool activate = true,
                        bool clearPixelSelection = false);
    /** Selection is screen state; selecting an existing layer adds no history entry. */
    Result selectLayer(LayerId layer);
    /** A nonempty screen selection and an independent existing active layer.
     * Preserves history, pending input and fixed snapshots.
     */
    Result selectLayers(const QList<LayerId> &layers, LayerId active);
    /** Duplicates selected subtrees above their highest root, climbing locked parents.
     * Input identities are in bottom-to-top list order. Selected descendants of another selected root are included once.
     * Names correspond to the selected identities. Copies have independent layer
     * and operation identities while retaining immutable input and pixel blocks.
     * A source outside the filtered selection selects the first copied root.
     */
    Result duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource);
    Result setLayerVisible(LayerId layer, bool visible);
    Result setLayerVisible(LayerId layer, bool visible, PropertyHistory history);
    Result setLayerOpacity(LayerId layer, quint8 opacity);
    Result setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history);
    Result setLayerLocked(LayerId layer, bool locked);
    Result setLayerLocked(LayerId layer, bool locked, PropertyHistory history);
    Result setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerChannelMask(LayerId layer, quint8 mask);
    Result setLayerProjectionMask(LayerId layer, quint8 mask, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerProjectionMask(LayerId layer, quint8 mask);
    Result setLayerPassThrough(LayerId layer, bool enabled, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerPassThrough(LayerId layer, bool enabled);
    Result setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history = PropertyHistory::Separate);
    Result writeLayerBlendMode(LayerId layer, LayerBlendMode mode);
    /** Adds one transparency mask to a paint layer from the current selection
     * coverage in one history entry. The layer keeps its pixels and the mask
     * becomes part of every retained document version and
     * saved document; no per-stroke synchronization occurs. A layer that
     * already owns a mask or a document without a selection is rejected.
     */
    Result addTransparencyMaskFromSelection(LayerId layer);
    /** Ordinary Add Mask copies and consumes the active selection, or creates
     * white default coverage when no selection is active, in one history item. */
    Result addTransparencyMask(LayerId layer);
    Result addTransparencyMask(LayerId layer, const QString &name);
    /** Imports the supplied mask identity, coverage and attributes atomically,
     * preserving the document's active selection. Other raster fields are unused. */
    Result insertTransparencyMask(LayerId layer, const RasterLayer &source);
    /** Toggles the retained mask's composition effect in one history entry. */
    Result setTransparencyMaskVisible(LayerId layer, bool visible, PropertyHistory history = PropertyHistory::Separate);
    Result setTransparencyMaskLocked(LayerId layer, bool locked, PropertyHistory history = PropertyHistory::Separate);
    Result writeTransparencyMaskVisible(LayerId layer, bool visible);
    Result writeTransparencyMaskLocked(LayerId layer, bool locked);
    Result renameTransparencyMask(LayerId layer, const QString &name, PropertyHistory history = PropertyHistory::Separate);
    Result writeTransparencyMaskName(LayerId layer, const QString &name);
    Result writeTransparencyMaskColorLabel(LayerId layer, int index);
    Result writeTransparencyMaskPinnedToTimeline(LayerId layer, bool pinned);
    /** Removes the retained mask in one history entry, preserving layer pixels. */
    Result removeTransparencyMask(LayerId layer);
    /** Public mask selection replacement preserves history count and retained
     * node identity; existing snapshots keep their original coverage. */
    Result writeTransparencyMask(LayerId layer, const SelectionMaskSP &mask);
    /** Replaces the retained mask coverage in one history entry, preserving
     * layer pixels and the mask's composition visibility. Null coverage removes
     * nothing; removeTransparencyMask() owns mask removal. Equal coverage
     * preserves the current version. `filled` marks a selection fill rather
     * than a brush stroke for the history action.
     */
    Result replaceTransparencyMask(LayerId layer, const SelectionMaskSP &mask, bool filled = false);
    SelectionMaskSP layerTransparencyMask(LayerId layer) const;
    bool layerTransparencyMaskVisible(LayerId layer) const;
    /** Direct background writes preserve history, pending input and fixed snapshots. */
    Result writeProjectionBackground(const RgbaPixel &rgba);
    /** Direct property writes preserve history transitions, pending input and fixed snapshots. */
    /** Direct default write preserves history positions and fixed reader versions. */
    Result writeLayerDefaultPixel(LayerId layer, const RgbaPixel &rgba);
    Result writeLayerName(LayerId layer, const QString &name);
    Result writeLayerVisible(LayerId layer, bool visible);
    Result writeLayerOpacity(LayerId layer, quint8 opacity);
    Result writeLayerLocked(LayerId layer, bool locked);
    Result writeLayerPinnedToTimeline(LayerId layer, bool pinned);
    Result writeLayerColorLabel(LayerId layer, int index);
    /** Records selected attributes from the fixed before version and current
     * after version as one edit. Both belong to this document and retain the
     * selected layer identities. Pixels, hierarchy, placement and other edits
     * completed during preview remain current. Busy, stale or rejected requests
     * preserve history, pending input and fixed versions.
     * Enabled attributes participate even when unchanged; unselected attributes
     * and components retain subsequent direct writes across this history step.
     */
    Result recordLayerProperties(const Snapshot &before, const Snapshot &after,
                                 const QList<LayerPropertiesSelection> &selection);
    /** Restores selected attributes from a fixed version outside undo, including
     * their exact component values. Preserves other edits, Redo, pending input
     * and fixed versions; rejects the complete request before adoption.
     */
    Result restoreLayerProperties(const Snapshot &before, const QList<LayerPropertiesSelection> &selection);
    Result renameLayer(LayerId layer, const QString &name);
    Result renameLayer(LayerId layer, const QString &name, PropertyHistory history);
    /** Moves within the bottom-to-top layer list, preserving the selected identity. */
    Result moveLayer(LayerId layer, qsizetype index);
    /** Adopts a complete bottom-to-top permutation in one history entry.
     * Rejects missing, repeated or foreign identities before changing any state.
     */
    Result reorderLayers(const QList<LayerId> &order);
    /** Moves selected roots into an existing group, or the root for a null parent.
     * A null sibling places them first; otherwise it must be a remaining child
     * of that parent. Roots follow native tree order and include their descendants.
     * Both sides of the history step select the filtered roots. A zero activeRoot
     * retains the independent active layer; a specified moving root becomes
     * active after the move. Undo retains the original active layer.
     * Fixed versions and pixel inputs remain unchanged.
     * Locked parents, cycles and invalid identities reject the complete request;
     * an unchanged hierarchy preserves the current version and history.
     */
    Result moveLayers(const QList<LayerId> &layers, const QUuid &parentUuid, const QUuid &aboveUuid,
                      LayerId activeRoot = 0);
    /** Groups filtered roots above activeRoot without replacing their pixels or
     * identities. An optional clipping name adds the native inherit-alpha paint
     * layer. The complete operation, including selection, is one undo step.
     */
    Result groupLayers(const QList<LayerId> &layers, LayerId activeRoot, const QString &groupName,
                       const QString &clippingLayerName = {});
    /** Ungroups the active group, or moves selected roots out of its parent.
     * A replacement name supplies the native last-layer removal behavior.
     * Admission failure preserves the entire hierarchy and both history branches.
     */
    Result ungroupLayers(const QList<LayerId> &layers, LayerId activeRoot, const QString &replacementName = {});
    /** Replaces the requested roots with native merged pixels, or retains them
     * for NewFromVisible. One accepted request is one history entry. Frozen
     * versions and rejected requests retain their pixels and identities.
     */
    Result mergeLayers(const LayerMergeRequest &request);
    /** Removes a layer and its descendants; selection follows the nearest remaining list position. */
    Result removeLayer(LayerId layer);
    /** Removes a set atomically in one history entry. Selection follows the
     * next remaining layer, or the preceding layer when no next layer remains.
     * A nonempty replacement name creates an empty layer when all are removed.
     */
    Result removeLayers(const QList<LayerId> &layers, const QString &replacementName = {});
    /** Clears retained paint and raster inputs in one undo step, preserving each
     * layer's default pixel, identity, properties and selection. The complete
     * request must name distinct, available, unlocked layers.
     */
    Result clearLayers(const QList<LayerId> &layers);
    /** Clears the current selection on distinct unlocked paint layers in one
     * history step, including native restoration of transparent default pixels.
     */
    Result clearSelectedPixels(const QList<LayerId> &layers);
    /** One undo step; normalizes the pixel rectangle while retaining its original
     * UI values. Defaults, position and properties stay unchanged. Programmatic
     * cropping accepts locked layers; interactive callers enforce editability.
     */
    Result cropLayer(LayerId layer, const QRect &rect);
    /** Moves the requested canvas origin to zero and optionally crops all layers.
     * Preserves layer properties and selection in one history step. An unchanged
     * canvas without layer cropping preserves the current version and history.
     */
    Result resizeCanvas(const QRect &rect, bool cropLayers);
    /** Fills canvas bounds on one layer as one history step, preserving pixels outside them.
     * The fill blend mode is frozen with the fill intent. Normal uses source-over
     * composition and erase ignores the blend mode. Opacity is finite in [0, 1].
     */
    Result fillLayer(LayerId layer, const std::array<quint8, 3> &rgb, qreal opacity = 1,
                     bool erase = false, bool alphaLocked = false,
                     LayerBlendMode blendMode = LayerBlendMode::Normal);
    /** Fills with a color expressed in the document's native channel precision.
     * Selection, opacity and component protection share the figure-paint contract.
     */
    Result fillLayerColor(LayerId layer, const RgbaPixel &color, qreal opacity = 1,
                          bool erase = false, bool alphaLocked = false,
                          LayerBlendMode blendMode = LayerBlendMode::Normal);
    /** Fills canvas bounds on one layer with a frozen pattern resource as one
     * history step. The pattern repeats from the document origin; selection
     * coverage clips and scales its alpha exactly as a solid fill. The fill
     * blend mode is frozen with the fill intent.
     */
    Result fillLayerPattern(LayerId layer, const PatternFill &pattern, qreal opacity = 1,
                            bool erase = false, bool alphaLocked = false,
                            LayerBlendMode blendMode = LayerBlendMode::Normal);
    /** Fills the canvas/selection intersection from a frozen gradient resource. */
    Result paintGradient(LayerId layer, const Brush &brush, const GradientFill &gradient);
    /** Complements RGB on one paint layer inside the current selection coverage
     * as one history entry. The selection rectangle, inversion, kind and mask
     * are frozen in the operation. Alpha is preserved and pixels outside the
     * coverage retain their exact values. Locked layers and active input are
     * rejected without changing history.
     */
    Result invertColors(LayerId layer);
    /** Replaces RGB with the documented Rec.709 luminance of the stored
     * sRGB-encoded bytes inside the current selection coverage as one history
     * entry. The synthetic luminance is computed with integer coefficients
     * `(2126 R + 7152 G + 722 B + 5000) / 10000`, so neutral gray survives
     * exactly and the rule is deterministic across devices. Alpha is preserved
     * and pixels outside the coverage retain their exact values. The selection
     * rectangle, inversion, kind and mask are frozen in the operation.
     */
    Result grayscale(LayerId layer);
    /** Applies the product HSV/HSL Adjustment settings to one paint layer
     * inside the current selection coverage as one history entry. The raw
     * product slider values, the color mode, the Colorize flag and the
     * document luma coefficients required by HSY, YUV and Colorize are frozen
     * in the operation. Invalid modes, out-of-range values and unfrozen or
     * unusable coefficients are rejected without changing history. Alpha is
     * preserved and pixels outside the coverage retain their exact values.
     * The selection rectangle, inversion, kind and mask are frozen in the
     * operation.
     */
    Result hsvAdjustment(LayerId layer, const HsvAdjustment &settings);
    OperationId beginColorConversion();
    Result cancelColorConversion(OperationId id);
    Result commitColorConversion(OperationId id, const Snapshot &before, PixelFormat format,
        const QByteArray &profile, const RgbaPixel &background, const QList<ConvertedLayerPixels> &layers);
    Result undo();
    Result redo();
    /** Selects one retained history state without executing intermediate edits. */
    Result setHistoryIndex(int index);

    /** Call after successful atomic saving of this snapshot, including an older pinned version. */
    Result markSaved(const Snapshot &saved);
    /** Releases uncommitted input and history; the last committed snapshot remains available. */
    void close();

private:
    friend class PaintImage;
    enum class LayerProperty : quint8 { None, Name, Opacity, Visible, Locked, PinnedToTimeline, ChannelMask, Position, ProjectionMask, ColorLabel, PassThrough, BlendMode, MaskVisible, MaskLocked, MaskName, MaskColorLabel, MaskPinnedToTimeline };
    struct LayerPropertyEdit {
        LayerProperty property;
        LayerId layer;
        QVariant before, after;
        quint8 componentMask = 15;
    };
    struct LayerHistoryChange {
        LayerProperty property = LayerProperty::None;
        LayerId layer = 0;
        QVariant before, after;
        bool mergeable = false;
        bool duplicatedLayers = false;
        bool clearedLayers = false;
        bool filledLayer = false;
        /** One paint layer gained an RGB filter operation in its retained version. */
        bool convertedColorSpace = false;
        bool filteredLayer = false;
        /** Filter applied by the retained operation. */
        FilterKind filterKind = FilterKind::InvertColors;
        CropSnapshot crop;
        std::optional<CanvasResize> canvasResize;
        QList<LayerPropertyEdit> propertyEdits;
        bool changedLayerProperties = false;
        bool changedSelection = false;
        bool movedSelectionPixels = false;
        bool translatedLayers = false;
        bool transformedLayers = false;
        HistoryAction layerStructureAction = HistoryAction::None;
        /** Adds or removes one layer's transparency mask in its retained version. */
        bool addedTransparencyMask = false;
        bool removedTransparencyMask = false;
    };
    std::unique_ptr<OperationDocument> cloneCurrent(bool preserveLayerUuids) const;
    std::unique_ptr<OperationDocument> layerGroupingDraft() const;
    LayerId insertVersion(const Snapshot &source, LayerId sourceLayer,
                          const RasterLayer &properties, qsizetype index, bool activate = true);
    std::unique_ptr<OperationDocument> cloneVersion(const Snapshot &source, bool preserveLayerUuids,
                                                  LayerId onlyLayer = 0) const;
    struct VersionPixelWrite
    {
        Snapshot before;
        LayerId layer = 0;
        QList<QSharedPointer<const RasterTile>> tiles;
        QList<QRect> areas;
    };
    Result applyPixelVersions(const QList<VersionPixelWrite> &writes, bool recordHistory,
                              Snapshot *retainedVersion = nullptr);
    /** Preparation retains bounded immutable reference inputs without changing
     * history. GPU admission checks precede adopting the prepared fill. */
    Result prepareRegionFill(LayerId layer, const RegionFillSettings &settings, RegionFillSP *region) const;
    bool selectionFitsRetainedInputs(const SelectionMaskSP &mask) const;
    Result fillRegion(LayerId layer, const RegionFillSP &region, const RgbaPixel &color,
                      const std::optional<PatternFill> &pattern, qreal opacity, bool erase,
                      bool alphaLocked, LayerBlendMode blendMode);
    Result fillLayerStyle(LayerId layer, const RgbaPixel &color, const FigureStyle &style,
                          qreal opacity, bool erase, bool alphaLocked);
    Result paintFigureWithin(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                             const Shape &shape, const FigureStyle &style, quint64 randomSeed,
                             const std::optional<QRect> &compositionBounds, const RegionFillSP &region = {});
    Result appendSamplesWithin(OperationId id, const QList<InputSample> &samples, const Shape *painterShape);
    Result addTransparencyMaskWithin(LayerId layer, bool requireSelection, const SelectionMaskSP &prepared = {},
                                     const QString &name = QStringLiteral("Transparency Mask"),
                                     const RasterLayer *imported = nullptr);
    Result applyFilter(LayerId layer, FilterKind kind, const HsvAdjustment *settings = nullptr);
    QList<Snapshot> layerPositionVersions(const Snapshot &source, LayerId layer) const;
    Result applyLayerPosition(LayerId layer, QPoint position, Snapshot *retainedVersion);
    Result applyLayerDefaultPixel(LayerId layer, const RgbaPixel &rgba, Snapshot *retainedVersion);
    template<class T> Result writeLayerProperty(LayerId layer, LayerProperty property, T Layer::*member,
                                              const T &value, quint8 componentMask = 15);
    Result writeLayerMask(LayerId layer, LayerProperty property, quint8 Layer::*member, quint8 value);
    Result restoreLayerHistoryChange(const LayerHistoryChange &change, bool forward);
    Result restoreLayerPropertyEdit(const LayerPropertyEdit &change, bool forward);
    Result collectLayerPropertyEdits(const Snapshot &before, const Snapshot &after,
                                    const QList<LayerPropertiesSelection> &selection,
                                    QList<LayerPropertyEdit> &edits) const;
    Result restoreLayerPropertyEdits(const QList<LayerPropertyEdit> &edits);
    Result checkOperation(OperationId id) const;
    Result checkMutation() const;
    Result updateSelectionTranslation(OperationId id, QPoint offset, const SelectionMaskSP &preparedSelection);
    Result updateLayerTranslationWithMasks(OperationId id, QPoint offset,
        const QHash<LayerId, SelectionMaskSP> &masks, const SelectionMaskSP &selection = {});
    Result cropLayerWithMask(LayerId layer, const QRect &rect, const SelectionMaskSP &mask);
    Result resizeCanvasWithMasks(const QRect &rect, bool cropLayers, const QHash<LayerId, SelectionMaskSP> &masks);
    /** An external edit (a mask stroke on a derived GPU image) borrows this
     * document's active-input state so saving and mutation stay blocked without
     * recording the derived operation as document history.
     */
    Result beginExternalStroke(const OperationId &id);
    Result endExternalStroke(const OperationId &id);
    Result updateExternalMask(const OperationId &id, LayerId layer, const SelectionMaskSP &mask);
    Result commitExternalMask(const OperationId &id, LayerId layer, const SelectionMaskSP &mask);
    qsizetype layerIndex(LayerId layer) const;
    void replacePreview(const QSharedPointer<const PaintOperation> &operation);
    void commitVersion(const Snapshot &version);
    void commitVersion(const Snapshot &version, const LayerHistoryChange &change);
    void commitPropertyVersion(const Snapshot &version, LayerHistoryChange change, PropertyHistory history);

    Limits m_limits;
    QUuid m_identity;
    QList<Snapshot> m_history;
    QList<LayerHistoryChange> m_layerHistoryChanges;
    int m_index = 0;
    quint64 m_nextRevision = 0;
    quint64 m_nextOperation = 0;
    LayerId m_nextLayer = 1;
    quint64 m_savedRevision = 0;
    QSharedPointer<const PaintOperation> m_edit;
    QList<AffineSnapshot> m_affine;
    TranslationSnapshot m_translation;
    QList<LayerTranslationTarget> m_translationTargets;
    QHash<LayerId, OperationId> m_translationIds;
    /** Operation identity of a derived mask edit; carries no document history. */
    OperationId m_externalOperation;
    Snapshot m_preview;
    bool m_closed = false;
};

}
#endif
