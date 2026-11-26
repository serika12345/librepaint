// SPDX-License-Identifier: GPL-3.0-or-later
//
// Test application for embedding a wgpu surface in a Qt Widgets window
// (Issue #108, phase G preparation). It checks three things before the
// display path of the LibrePaint canvas is moved to wgpu:
//
// 1. Pen tablet input reaches a native child widget that wgpu draws into.
// 2. Tool decorations painted with QPainter can be laid over the canvas, and
//    ordinary widgets placed over the canvas stay visible (--overlay-widgets).
// 3. The pen-to-display delay of the wgpu canvas is shorter than that of the
//    QOpenGLWidget canvas the product uses today.
//
// Both canvases do the same work, the way the product keeps its pixels on
// the CPU: the ink is drawn into a CPU image, the hover mark and the tool
// decoration into a second CPU image, and only the changed rectangles are
// uploaded to two GPU textures that one draw composes. The marks match the
// reference applications of scripts/perf/pen-latency: a black 21 px square at
// the hover position and a 20 px black line while the button is held, on
// white. The decoration is an orange circle around the pointer; its colour is
// light enough that the capture tools do not count it as ink.
//
//   --canvas gl          QOpenGLWidget, composed by Qt into the window
//   --canvas wgpu        native child widget with its own wgpu surface
//   --present MODE       wgpu present mode: fifo (default), mailbox, immediate
//   --demo               draw a stroke and a hover mark at start, for
//                        snapshots without pointer input
//   --overlay-widgets    show a child widget of the canvas and a sibling
//                        widget raised over the canvas
//   --native-siblings    keep Qt's default that makes the siblings of a
//                        native widget native too
//
// With REFERENCE_LOG set (run-linux.sh sets it), every event is written there
// with CLOCK_MONOTONIC times, the clock of mutter_capture.py:
//   input,<time_ns>,<kind>,<x>,<y>       kind: hover, press, drag, release
//   tablet,<time_ns>,<type>,<x>,<y>,<pressure>,<xtilt>,<ytilt>
//   frame,<time_ns>,<oldest shown input ns>   after the frame is handed over
//   info,<text>
// Any last argument that does not start with "--" is ignored (run-linux.sh
// passes a canvas path).
//
// Build and run: build.sh, then the launcher it writes.

#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPainter>
#include <QScreen>
#include <QStatusBar>
#include <QSurfaceFormat>
#include <QTabletEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QtGui/qguiapplication_platform.h>
#include <qpa/qplatformnativeinterface.h>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <memory>

#ifdef Q_OS_MACOS
void *wgpuEmbedMetalLayer(void *nsView); // metal_layer.mm
#endif

namespace {

qint64 nowNs()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return qint64(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

FILE *s_log = nullptr;

void logLine(const char *format, ...) __attribute__((format(printf, 1, 2)));
void logLine(const char *format, ...)
{
    if (!s_log) {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(s_log, format, args);
    va_end(args);
    fputc('\n', s_log);
}

// The CPU images both canvases show, in device pixels.
class Scene
{
public:
    QImage ink;
    QImage overlay;
    QRect inkDirty;
    QRect overlayDirty;
    qint64 oldestInputNs = 0;

    void resize(const QSize &logical, qreal dpr)
    {
        m_dpr = dpr;
        const QSize device = (QSizeF(logical) * dpr).toSize();
        ink = QImage(device, QImage::Format_ARGB32_Premultiplied);
        ink.fill(Qt::white);
        ink.setDevicePixelRatio(dpr);
        overlay = QImage(device, QImage::Format_ARGB32_Premultiplied);
        overlay.fill(Qt::transparent);
        overlay.setDevicePixelRatio(dpr);
        inkDirty = overlayDirty = ink.rect();
        m_overlayShown = QRectF();
        redrawOverlay();
    }

    void input(const char *kind, const QPointF &position, bool pressed)
    {
        const qint64 now = nowNs();
        logLine("input,%lld,%s,%.1f,%.1f", now, kind, position.x(), position.y());
        if (!oldestInputNs) {
            oldestInputNs = now;
        }
        if (pressed && m_pressed) {
            drawLine(m_last, position);
        } else if (pressed) {
            drawLine(position, position);
        }
        m_pressed = pressed;
        m_last = position;
        m_hover = position;
        m_hasHover = true;
        redrawOverlay();
    }

    // --demo: a stroke and a hover position without input, so a snapshot
    // shows the ink, the hover mark and the decoration.
    void demo()
    {
        const QSizeF size = ink.deviceIndependentSize();
        input("press", QPointF(size.width() * 0.3, size.height() * 0.6), true);
        input("drag", QPointF(size.width() * 0.6, size.height() * 0.6), true);
        input("release", QPointF(size.width() * 0.6, size.height() * 0.6), false);
        input("hover", QPointF(size.width() * 0.45, size.height() * 0.4), false);
    }

    void takeDirty() { inkDirty = overlayDirty = QRect(); oldestInputNs = 0; }

private:
    QRect deviceRect(const QRectF &logical) const
    {
        const QRectF r(logical.topLeft() * m_dpr, logical.size() * m_dpr);
        return r.toAlignedRect().adjusted(-1, -1, 1, 1).intersected(ink.rect());
    }

    void drawLine(const QPointF &from, const QPointF &to)
    {
        QPainter painter(&ink);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(Qt::black, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(from, to);
        inkDirty |= deviceRect(QRectF(from, to).normalized().adjusted(-11, -11, 11, 11));
    }

    void redrawOverlay()
    {
        QPainter painter(&overlay);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        if (!m_overlayShown.isNull()) {
            painter.fillRect(m_overlayShown, Qt::transparent);
            overlayDirty |= deviceRect(m_overlayShown);
        }
        if (!m_hasHover) {
            return;
        }
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setRenderHint(QPainter::Antialiasing);
        if (!m_pressed) {
            painter.fillRect(QRectF(m_hover.x() - 10, m_hover.y() - 10, 21, 21), Qt::black);
        }
        painter.setPen(QPen(QColor(255, 150, 40), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_hover, 32, 32);
        m_overlayShown = QRectF(m_hover.x() - 36, m_hover.y() - 36, 72, 72);
        overlayDirty |= deviceRect(m_overlayShown);
    }

    qreal m_dpr = 1;
    QPointF m_last;
    QPointF m_hover;
    QRectF m_overlayShown;
    bool m_pressed = false;
    bool m_hasHover = false;
};

class DemoCanvas
{
public:
    virtual ~DemoCanvas() = default;
    virtual void runDemo() = 0;
};

// Pointer and tablet handling shared by both canvases.
template<typename Base>
class InputCanvas : public Base, public DemoCanvas
{
public:
    explicit InputCanvas(QWidget *parent) : Base(parent)
    {
        this->setMouseTracking(true);
        this->setTabletTracking(true);
        this->setCursor(Qt::BlankCursor);
    }

    void runDemo() override
    {
        m_scene.demo();
        requestFrame();
    }

protected:
    virtual void requestFrame() = 0;

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const bool pressed = event->buttons() & Qt::LeftButton;
        m_scene.input(pressed ? "drag" : "hover", event->position(), pressed);
        requestFrame();
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        m_scene.input("press", event->position(), true);
        requestFrame();
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        m_scene.input("release", event->position(), false);
        requestFrame();
    }
    void tabletEvent(QTabletEvent *event) override
    {
        logLine("tablet,%lld,%d,%.1f,%.1f,%.3f,%.1f,%.1f", nowNs(), int(event->type()), event->position().x(),
                event->position().y(), event->pressure(), event->xTilt(), event->yTilt());
        const bool pressed = event->buttons() & Qt::LeftButton;
        const char *kind = event->type() == QEvent::TabletPress ? "press"
            : event->type() == QEvent::TabletRelease            ? "release"
            : pressed                                           ? "drag"
                                                                : "hover";
        m_scene.input(kind, event->position(), event->type() != QEvent::TabletRelease && pressed);
        event->accept();
        requestFrame();
    }

    Scene m_scene;
};

// The canvas of the product today: a QOpenGLWidget that Qt composes into the
// window's backing store.
class GlCanvas : public InputCanvas<QOpenGLWidget>
{
public:
    explicit GlCanvas(QWidget *parent) : InputCanvas(parent)
    {
        connect(this, &QOpenGLWidget::frameSwapped, this, [this] {
            logLine("frame,%lld,%lld", nowNs(), m_paintedInputNs);
            m_paintedInputNs = 0;
        });
    }

protected:
    void requestFrame() override { update(); }

    void initializeGL() override
    {
        m_gl = context()->extraFunctions();
        m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
            "attribute vec2 position; varying vec2 uv;\n"
            "void main() { uv = vec2(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);"
            " gl_Position = vec4(position, 0.0, 1.0); }\n");
        m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
            "uniform sampler2D ink; uniform sampler2D over; varying vec2 uv;\n"
            "void main() { vec4 i = texture2D(ink, uv); vec4 o = texture2D(over, uv);"
            " gl_FragColor = vec4(o.rgb + i.rgb * (1.0 - o.a), 1.0); }\n");
        m_program.bindAttributeLocation("position", 0);
        m_program.link();
        m_gl->glGenTextures(2, m_textures);
        logLine("info,canvas gl %s", reinterpret_cast<const char *>(m_gl->glGetString(GL_RENDERER)));
    }

    void resizeGL(int, int) override
    {
        m_scene.resize(size(), devicePixelRatioF());
        for (GLuint texture : m_textures) {
            m_gl->glBindTexture(GL_TEXTURE_2D, texture);
            m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_scene.ink.width(), m_scene.ink.height(), 0, GL_BGRA,
                               GL_UNSIGNED_BYTE, nullptr);
        }
    }

    void paintGL() override
    {
        upload(m_textures[0], m_scene.ink, m_scene.inkDirty);
        upload(m_textures[1], m_scene.overlay, m_scene.overlayDirty);
        m_paintedInputNs = m_scene.oldestInputNs;
        m_scene.takeDirty();

        static const GLfloat quad[] = {-1, -1, 1, -1, -1, 1, 1, 1};
        m_gl->glDisable(GL_BLEND);
        m_program.bind();
        m_gl->glActiveTexture(GL_TEXTURE0);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_textures[0]);
        m_gl->glActiveTexture(GL_TEXTURE1);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_textures[1]);
        m_program.setUniformValue("ink", 0);
        m_program.setUniformValue("over", 1);
        m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_gl->glEnableVertexAttribArray(0);
        m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
        m_gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_gl->glDisableVertexAttribArray(0);
        m_program.release();
        m_gl->glActiveTexture(GL_TEXTURE0);
    }

private:
    void upload(GLuint texture, const QImage &image, const QRect &dirty)
    {
        if (dirty.isEmpty()) {
            return;
        }
        m_gl->glBindTexture(GL_TEXTURE_2D, texture);
        m_gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, image.bytesPerLine() / 4);
        m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, dirty.x(), dirty.y(), dirty.width(), dirty.height(), GL_BGRA,
                              GL_UNSIGNED_BYTE, image.constScanLine(dirty.y()) + dirty.x() * 4);
        m_gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    QOpenGLExtraFunctions *m_gl = nullptr;
    QOpenGLShaderProgram m_program;
    GLuint m_textures[2] = {0, 0};
    qint64 m_paintedInputNs = 0;
};

WGPUStringView view(const char *text)
{
    return WGPUStringView{text, WGPU_STRLEN};
}

QByteArray text(WGPUStringView message)
{
    return message.data ? QByteArray(message.data, message.length == WGPU_STRLEN ? qstrlen(message.data) : int(message.length))
                        : QByteArray();
}

const char *const k_shader = R"(
@group(0) @binding(0) var ink: texture_2d<f32>;
@group(0) @binding(1) var over: texture_2d<f32>;
@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f {
    var p = array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0));
    return vec4f(p[i], 0.0, 1.0);
}
@fragment fn fs(@builtin(position) position: vec4f) -> @location(0) vec4f {
    let c = vec2i(position.xy);
    let i = textureLoad(ink, c, 0);
    let o = textureLoad(over, c, 0);
    return vec4f(o.rgb + i.rgb * (1.0 - o.a), 1.0);
}
)";

// The candidate canvas: a native child widget whose window wgpu presents to
// directly (a wl_subsurface on Wayland, a child X window on X11).
class WgpuCanvas : public InputCanvas<QWidget>
{
public:
    WgpuCanvas(QWidget *parent, WGPUPresentMode presentMode) : InputCanvas(parent), m_presentMode(presentMode)
    {
        setAttribute(Qt::WA_NativeWindow);
        // Without it Qt's backing store also attaches buffers to the child
        // surface, and the compositor ends the connection on Wayland:
        // "Explicit Sync only supported on dmabuf buffers".
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    ~WgpuCanvas() override
    {
        if (m_surface) {
            wgpuSurfaceRelease(m_surface);
        }
    }

    QPaintEngine *paintEngine() const override { return nullptr; }

protected:
    void requestFrame() override
    {
        if (m_framePending) {
            return;
        }
        m_framePending = true;
        QMetaObject::invokeMethod(this, [this] { render(); }, Qt::QueuedConnection);
    }

    void paintEvent(QPaintEvent *) override
    {
        m_scene.inkDirty = m_scene.ink.rect();
        m_scene.overlayDirty = m_scene.overlay.rect();
        requestFrame();
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        m_scene.resize(size(), devicePixelRatioF());
        m_configured = false;
        requestFrame();
    }

private:
    bool ensureDevice()
    {
        if (m_device) {
            return true;
        }
        QWindow *window = windowHandle();
        if (!window || !window->handle()) {
            return false;
        }
        m_instance = wgpuCreateInstance(nullptr);
        WGPUSurfaceDescriptor surfaceDescriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
        const QString platform = QGuiApplication::platformName();
#ifdef Q_OS_MACOS
        WGPUSurfaceSourceMetalLayer metal = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
        if (platform == QLatin1String("cocoa")) {
            metal.layer = wgpuEmbedMetalLayer(reinterpret_cast<void *>(winId()));
            surfaceDescriptor.nextInChain = &metal.chain;
        } else
#else
        WGPUSurfaceSourceWaylandSurface wayland = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
        WGPUSurfaceSourceXlibWindow xlib = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
        if (platform == QLatin1String("wayland")) {
            wayland.display = qApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display();
            wayland.surface = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window);
            surfaceDescriptor.nextInChain = &wayland.chain;
        } else if (platform == QLatin1String("xcb")) {
            xlib.display = qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
            xlib.window = winId();
            surfaceDescriptor.nextInChain = &xlib.chain;
        } else
#endif
        {
            qFatal("unsupported platform %s", qPrintable(platform));
        }
        m_surface = wgpuInstanceCreateSurface(m_instance, &surfaceDescriptor);

        WGPURequestAdapterOptions adapterOptions = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
        adapterOptions.compatibleSurface = m_surface;
        // The GPU that drives the display, as the OpenGL canvas uses on a
        // machine with an integrated and a discrete GPU.
        adapterOptions.powerPreference = WGPUPowerPreference_LowPower;
        WGPURequestAdapterCallbackInfo adapterCallback = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
        adapterCallback.mode = WGPUCallbackMode_AllowSpontaneous;
        adapterCallback.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
                                      void *userdata, void *) {
            if (status != WGPURequestAdapterStatus_Success) {
                qFatal("no adapter: %s", text(message).constData());
            }
            *static_cast<WGPUAdapter *>(userdata) = adapter;
        };
        adapterCallback.userdata1 = &m_adapter;
        wgpuInstanceRequestAdapter(m_instance, &adapterOptions, adapterCallback);
        while (!m_adapter) {
            wgpuInstanceProcessEvents(m_instance);
        }
        WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
        wgpuAdapterGetInfo(m_adapter, &info);
        logLine("info,canvas wgpu %s %s backend %d", text(info.device).constData(), text(info.description).constData(),
                int(info.backendType));
        wgpuAdapterInfoFreeMembers(info);

        WGPUDeviceDescriptor deviceDescriptor = WGPU_DEVICE_DESCRIPTOR_INIT;
        deviceDescriptor.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const *, WGPUErrorType type,
                                                                   WGPUStringView message, void *, void *) {
            qFatal("wgpu error %d: %s", int(type), text(message).constData());
        };
        WGPURequestDeviceCallbackInfo deviceCallback = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
        deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
        deviceCallback.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message,
                                     void *userdata, void *) {
            if (status != WGPURequestDeviceStatus_Success) {
                qFatal("no device: %s", text(message).constData());
            }
            *static_cast<WGPUDevice *>(userdata) = device;
        };
        deviceCallback.userdata1 = &m_device;
        wgpuAdapterRequestDevice(m_adapter, &deviceDescriptor, deviceCallback);
        while (!m_device) {
            wgpuInstanceProcessEvents(m_instance);
        }
        m_queue = wgpuDeviceGetQueue(m_device);

        WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);
        m_format = capabilities.formats[0];
        for (size_t i = 0; i < capabilities.formatCount; ++i) {
            if (capabilities.formats[i] == WGPUTextureFormat_BGRA8Unorm) {
                m_format = WGPUTextureFormat_BGRA8Unorm;
            }
        }
        bool presentModeSupported = false;
        for (size_t i = 0; i < capabilities.presentModeCount; ++i) {
            presentModeSupported |= capabilities.presentModes[i] == m_presentMode;
        }
        if (!presentModeSupported) {
            qFatal("present mode %d is not supported by this surface", int(m_presentMode));
        }
        m_alphaMode = capabilities.alphaModes[0];
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        logLine("info,surface format %d present mode %d alpha mode %d", int(m_format), int(m_presentMode),
                int(m_alphaMode));

        WGPUShaderSourceWGSL wgsl = WGPU_SHADER_SOURCE_WGSL_INIT;
        wgsl.code = view(k_shader);
        WGPUShaderModuleDescriptor shaderDescriptor = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        shaderDescriptor.nextInChain = &wgsl.chain;
        WGPUShaderModule shader = wgpuDeviceCreateShaderModule(m_device, &shaderDescriptor);

        WGPUColorTargetState target = WGPU_COLOR_TARGET_STATE_INIT;
        target.format = m_format;
        WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
        fragment.module = shader;
        fragment.entryPoint = view("fs");
        fragment.targetCount = 1;
        fragment.targets = &target;
        WGPURenderPipelineDescriptor pipelineDescriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
        pipelineDescriptor.vertex.module = shader;
        pipelineDescriptor.vertex.entryPoint = view("vs");
        pipelineDescriptor.fragment = &fragment;
        m_pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDescriptor);
        wgpuShaderModuleRelease(shader);
        return true;
    }

    void configure()
    {
        WGPUSurfaceConfiguration configuration = WGPU_SURFACE_CONFIGURATION_INIT;
        configuration.device = m_device;
        configuration.format = m_format;
        configuration.usage = WGPUTextureUsage_RenderAttachment;
        configuration.width = uint32_t(m_scene.ink.width());
        configuration.height = uint32_t(m_scene.ink.height());
        configuration.alphaMode = m_alphaMode;
        configuration.presentMode = m_presentMode;
        wgpuSurfaceConfigure(m_surface, &configuration);

        for (WGPUTexture &texture : m_textures) {
            if (texture) {
                wgpuTextureRelease(texture);
            }
            WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
            descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
            descriptor.size = {configuration.width, configuration.height, 1};
            descriptor.format = WGPUTextureFormat_BGRA8Unorm;
            texture = wgpuDeviceCreateTexture(m_device, &descriptor);
        }
        WGPUBindGroupEntry entries[2] = {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
        WGPUTextureView views[2];
        for (int i = 0; i < 2; ++i) {
            views[i] = wgpuTextureCreateView(m_textures[i], nullptr);
            entries[i].binding = uint32_t(i);
            entries[i].textureView = views[i];
        }
        if (m_bindGroup) {
            wgpuBindGroupRelease(m_bindGroup);
        }
        WGPUBindGroupLayout layout = wgpuRenderPipelineGetBindGroupLayout(m_pipeline, 0);
        WGPUBindGroupDescriptor bindGroupDescriptor = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
        bindGroupDescriptor.layout = layout;
        bindGroupDescriptor.entryCount = 2;
        bindGroupDescriptor.entries = entries;
        m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &bindGroupDescriptor);
        wgpuBindGroupLayoutRelease(layout);
        for (WGPUTextureView textureView : views) {
            wgpuTextureViewRelease(textureView);
        }
        m_scene.inkDirty = m_scene.ink.rect();
        m_scene.overlayDirty = m_scene.overlay.rect();
        m_configured = true;
    }

    void upload(WGPUTexture texture, const QImage &image, const QRect &dirty)
    {
        if (dirty.isEmpty()) {
            return;
        }
        WGPUTexelCopyTextureInfo destination = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
        destination.texture = texture;
        destination.origin = {uint32_t(dirty.x()), uint32_t(dirty.y()), 0};
        WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
        layout.bytesPerRow = uint32_t(image.bytesPerLine());
        layout.rowsPerImage = uint32_t(dirty.height());
        const WGPUExtent3D extent = {uint32_t(dirty.width()), uint32_t(dirty.height()), 1};
        const size_t bytes = size_t(dirty.height() - 1) * image.bytesPerLine() + size_t(dirty.width()) * 4;
        wgpuQueueWriteTexture(m_queue, &destination, image.constScanLine(dirty.y()) + dirty.x() * 4, bytes, &layout,
                              &extent);
    }

    void render()
    {
        m_framePending = false;
        if (m_scene.ink.isNull() || !ensureDevice()) {
            return;
        }
        if (!m_configured) {
            configure();
        }
        WGPUSurfaceTexture current = WGPU_SURFACE_TEXTURE_INIT;
        wgpuSurfaceGetCurrentTexture(m_surface, &current);
        if (current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
            && current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            logLine("info,surface texture status %d", int(current.status));
            if (current.texture) {
                wgpuTextureRelease(current.texture);
            }
            if (current.status == WGPUSurfaceGetCurrentTextureStatus_Occluded) {
                // macOS reports a covered window; draw again once it may show.
                QTimer::singleShot(100, this, [this] { requestFrame(); });
                return;
            }
            m_configured = false;
            requestFrame();
            return;
        }
        upload(m_textures[0], m_scene.ink, m_scene.inkDirty);
        upload(m_textures[1], m_scene.overlay, m_scene.overlayDirty);
        const qint64 shownInputNs = m_scene.oldestInputNs;
        m_scene.takeDirty();

        WGPUTextureView target = wgpuTextureCreateView(current.texture, nullptr);
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);
        WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        attachment.view = target;
        attachment.loadOp = WGPULoadOp_Clear;
        attachment.storeOp = WGPUStoreOp_Store;
        attachment.clearValue = {1, 1, 1, 1};
        WGPURenderPassDescriptor pass = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
        pass.colorAttachmentCount = 1;
        pass.colorAttachments = &attachment;
        WGPURenderPassEncoder encoderPass = wgpuCommandEncoderBeginRenderPass(encoder, &pass);
        wgpuRenderPassEncoderSetPipeline(encoderPass, m_pipeline);
        wgpuRenderPassEncoderSetBindGroup(encoderPass, 0, m_bindGroup, 0, nullptr);
        wgpuRenderPassEncoderDraw(encoderPass, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(encoderPass);
        wgpuRenderPassEncoderRelease(encoderPass);
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_queue, 1, &commands);
        wgpuSurfacePresent(m_surface);
        logLine("frame,%lld,%lld", nowNs(), shownInputNs);
        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(target);
        wgpuTextureRelease(current.texture);
    }

    WGPUPresentMode m_presentMode;
    WGPUInstance m_instance = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUQueue m_queue = nullptr;
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    WGPUCompositeAlphaMode m_alphaMode = WGPUCompositeAlphaMode_Auto;
    WGPURenderPipeline m_pipeline = nullptr;
    WGPUBindGroup m_bindGroup = nullptr;
    WGPUTexture m_textures[2] = {nullptr, nullptr};
    bool m_configured = false;
    bool m_framePending = false;
};

QFrame *overlayWidget(QWidget *parent, const QString &label)
{
    auto *frame = new QFrame(parent);
    frame->setFrameShape(QFrame::Box);
    frame->setAutoFillBackground(true);
    QPalette palette = frame->palette();
    palette.setColor(QPalette::Window, QColor(200, 230, 255));
    frame->setPalette(palette);
    auto *layout = new QVBoxLayout(frame);
    layout->addWidget(new QLabel(label, frame));
    frame->resize(220, 60);
    return frame;
}

} // namespace

int main(int argc, char **argv)
{
    if (const char *path = getenv("REFERENCE_LOG")) {
        s_log = fopen(path, "w");
        if (s_log) {
            setvbuf(s_log, nullptr, _IOLBF, 0);
        }
    }
    // The product does not wait for the vertical refresh in the swap
    // (KisOpenGL sets the swap interval to 0); the OpenGL canvas does the
    // same, so it compares with the wgpu canvas in the mailbox present mode.
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);
    // Without it, making the canvas native also makes every sibling widget
    // and every sibling of its ancestors native (QWidgetPrivate::createWinId),
    // and those widgets then show the Wayland flush offset described in
    // check.sh. --native-siblings keeps Qt's default to reproduce that.
    bool nativeSiblings = false;
    for (int i = 1; i < argc; ++i) {
        nativeSiblings |= qstrcmp(argv[i], "--native-siblings") == 0;
    }
    if (!nativeSiblings) {
        QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    }
    QApplication application(argc, argv);
    QString canvasKind = QStringLiteral("gl");
    WGPUPresentMode presentMode = WGPUPresentMode_Fifo;
    QString presentName = QStringLiteral("fifo");
    bool overlayWidgets = false;
    bool demo = false;
    const QStringList arguments = application.arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments[i] == QLatin1String("--canvas") && i + 1 < arguments.size()) {
            canvasKind = arguments[++i];
        } else if (arguments[i] == QLatin1String("--present") && i + 1 < arguments.size()) {
            presentName = arguments[++i];
            presentMode = presentName == QLatin1String("mailbox") ? WGPUPresentMode_Mailbox
                : presentName == QLatin1String("immediate")       ? WGPUPresentMode_Immediate
                                                                  : WGPUPresentMode_Fifo;
        } else if (arguments[i] == QLatin1String("--demo")) {
            demo = true;
        } else if (arguments[i] == QLatin1String("--overlay-widgets")) {
            overlayWidgets = true;
        } else if (arguments[i] == QLatin1String("--native-siblings")) {
        } else if (arguments[i].startsWith(QLatin1String("--")) || i + 1 < arguments.size()) {
            qFatal("unknown argument %s", qPrintable(arguments[i]));
        }
    }

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("wgpu embed check"));
    QWidget *canvas = nullptr;
    if (canvasKind == QLatin1String("wgpu")) {
        canvas = new WgpuCanvas(&window, presentMode);
    } else if (canvasKind == QLatin1String("gl")) {
        canvas = new GlCanvas(&window);
    } else {
        qFatal("unknown canvas %s", qPrintable(canvasKind));
    }
    window.setCentralWidget(canvas);
    const QString status = QStringLiteral("platform %1, canvas %2%3")
                               .arg(QGuiApplication::platformName(), canvasKind,
                                    canvasKind == QLatin1String("wgpu") ? QStringLiteral(" ") + presentName : QString());
    window.statusBar()->showMessage(status);
    logLine("info,%s", qPrintable(status));

    const QRect screen = window.screen()->availableGeometry();
    window.resize(screen.size() * 0.8);
    window.move(screen.center() - window.rect().center());
    window.show();

    if (demo) {
        QTimer::singleShot(500, canvas, [canvas] { dynamic_cast<DemoCanvas *>(canvas)->runDemo(); });
    }
    if (overlayWidgets) {
        QFrame *child = overlayWidget(canvas, QStringLiteral("child of the canvas"));
        child->move(canvas->width() - child->width() - 20, 20);
        child->show();
        QFrame *sibling = overlayWidget(&window, QStringLiteral("sibling over the canvas"));
        sibling->move(window.width() - sibling->width() - 20, 100);
        sibling->raise();
        sibling->show();
        QTimer::singleShot(1000, [&window, canvas, child, sibling] {
            logLine("info,native window: status bar %d, canvas %d, child %d, sibling %d, central %d",
                    window.statusBar()->internalWinId() != 0, canvas->internalWinId() != 0,
                    child->internalWinId() != 0, sibling->internalWinId() != 0,
                    window.centralWidget()->internalWinId() != 0);
        });
    }
    const int result = application.exec();
    if (s_log) {
        fclose(s_log);
    }
    return result;
}
