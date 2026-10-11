/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuWindowSurface_p.h"
#include "SurfaceRenderer.h"
#include <QGuiApplication>
#include <QWindow>
#include <qguiapplication_platform.h>
#include <stdexcept>
#include <limits>
#include <cstring>
#ifdef LIBREPAINT_GPU_WAYLAND
#include <QBackingStore>
#include <QPainter>
#include <qpa/qplatformwindow_p.h>
#include <wayland-client.h>
#include "presentation-time-client-protocol.h"
#include "viewporter-client-protocol.h"
#include <ctime>
#include <cerrno>
#include <list>
#endif
#include <X11/Xlib.h>
#undef Status
#undef None
namespace Krita::Canvas {
#ifdef LIBREPAINT_GPU_WAYLAND
namespace {
/** GUI-thread owner; Qt reads the shared connection and this owner dispatches its own queue. */
struct WaylandPresentation {
    struct Feedback {
        struct wp_presentation_feedback *object = nullptr;
        std::shared_ptr<GpuPresentationData> state;
    };
    wl_display *display = nullptr;
    QWindow *target = nullptr;
    wl_surface *window = nullptr;
    wl_subsurface *subsurface = nullptr;
    wl_subcompositor *subcompositor = nullptr;
    wp_viewporter *viewporter = nullptr;
    wp_viewport *viewport = nullptr;
    wl_event_queue *queue = nullptr;
    wl_registry *registry = nullptr;
    wp_presentation *protocol = nullptr;
    std::unique_ptr<QBackingStore> backingStore;
    std::optional<clockid_t> clock;
    std::list<Feedback> feedback;

    ~WaylandPresentation() {
        for (auto &item : feedback) {
            item.state->abandon();
            if (item.object) wp_presentation_feedback_destroy(item.object);
        }
        if (protocol) wp_presentation_destroy(protocol);
        if (viewport) wp_viewport_destroy(viewport);
        if (subsurface) wl_subsurface_destroy(subsurface);
        if (window) wl_surface_destroy(window);
        if (viewporter) wp_viewporter_destroy(viewporter);
        if (subcompositor) wl_subcompositor_destroy(subcompositor);
        if (registry) wl_registry_destroy(registry);
        if (queue) wl_event_queue_destroy(queue);
    }
    void initializeConnection(wl_display *connection) {
        display = connection;
        queue = wl_display_create_queue(display);
        if (!queue) throw std::runtime_error("Cannot create Wayland presentation queue");
        registry = wl_display_get_registry(display);
        if (!registry) throw std::runtime_error("Cannot create Wayland presentation registry");
        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(registry), queue);
        static const wl_registry_listener globals{
            [](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t) {
                auto &self = *static_cast<WaylandPresentation *>(data);
                if (!std::strcmp(interface, wl_subcompositor_interface.name)) {
                    self.subcompositor = static_cast<wl_subcompositor *>(wl_registry_bind(registry, name, &wl_subcompositor_interface, 1));
                    return;
                }
                if (!std::strcmp(interface, wp_viewporter_interface.name)) {
                    self.viewporter = static_cast<wp_viewporter *>(wl_registry_bind(registry, name, &wp_viewporter_interface, 1));
                    return;
                }
                if (std::strcmp(interface, wp_presentation_interface.name) || self.protocol) return;
                self.protocol = static_cast<wp_presentation *>(wl_registry_bind(registry, name, &wp_presentation_interface, 1));
                static const wp_presentation_listener clock{
                    [](void *data, wp_presentation *, uint32_t id) {
                        auto &self = *static_cast<WaylandPresentation *>(data);
                        timespec now{};
                        if (clock_gettime(clockid_t(id), &now) == 0) self.clock = clockid_t(id);
                    }
                };
                wp_presentation_add_listener(self.protocol, &clock, &self);
            },
            [](void *, wl_registry *, uint32_t) {}
        };
        wl_registry_add_listener(registry, &globals, this);
        if (wl_display_roundtrip_queue(display, queue) < 0
            || wl_display_roundtrip_queue(display, queue) < 0)
            throw std::runtime_error("Cannot initialize Wayland presentation feedback");
    }
    void initialize(QWindow &qtWindow) {
        const auto app = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
        const auto nativeWindow = qtWindow.nativeInterface<QNativeInterface::Private::QWaylandWindow>();
        if (!app || !nativeWindow || !app->display() || !app->compositor() || !nativeWindow->surface())
            throw std::runtime_error("GPU window requires a Wayland display and surface");
        target = &qtWindow;
        initializeConnection(app->display());
        if (!subcompositor || !viewporter)
            throw std::runtime_error("GPU Wayland surface requires subsurfaces and viewports");
        // Qt retains its parent buffers and input; only Vulkan attaches buffers to this child.
        window = wl_compositor_create_surface(app->compositor());
        if (!window) throw std::runtime_error("Cannot create GPU Wayland child surface");
        wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(window), queue);
        subsurface = wl_subcompositor_get_subsurface(subcompositor, window, nativeWindow->surface());
        viewport = wp_viewporter_get_viewport(viewporter, window);
        if (!subsurface || !viewport) throw std::runtime_error("Cannot configure GPU Wayland child surface");
        wl_subsurface_set_desync(subsurface);
        // The transparent Qt parent stays above the child so its frame callbacks remain visible.
        wl_subsurface_place_below(subsurface, nativeWindow->surface());
        const auto inputRegion = wl_compositor_create_region(app->compositor());
        if (!inputRegion) throw std::runtime_error("Cannot create GPU Wayland input region");
        wl_surface_set_input_region(window, inputRegion);
        wl_region_destroy(inputRegion);
        backingStore = std::make_unique<QBackingStore>(&qtWindow);
        resize();
    }
    void resize() {
        wp_viewport_set_destination(viewport, target->width(), target->height());
        if (backingStore->size() == target->size()) return;
        backingStore->resize(target->size());
        const QRegion region(QRect(QPoint(), target->size()));
        backingStore->beginPaint(region);
        {
            QPainter painter(backingStore->paintDevice());
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(QRect(QPoint(), target->size()), Qt::transparent);
        }
        backingStore->endPaint();
        backingStore->flush(region);
    }
    std::shared_ptr<GpuPresentationData> request() {
        poll();
        if (!protocol || !clock) return {};
        feedback.push_back({wp_presentation_feedback(protocol, window), std::make_shared<GpuPresentationData>()});
        auto &item = feedback.back();
        if (!item.object) { feedback.pop_back(); return {}; }
        static const wp_presentation_feedback_listener listener{
            [](void *, struct wp_presentation_feedback *, wl_output *) {},
            [](void *data, struct wp_presentation_feedback *object, uint32_t hi, uint32_t lo, uint32_t ns,
               uint32_t, uint32_t, uint32_t, uint32_t) {
                auto &item = *static_cast<Feedback *>(data);
                const quint64 seconds = (quint64(hi) << 32) | lo;
                if (ns >= 1000000000 || seconds > (std::numeric_limits<quint64>::max() - ns) / 1000000000)
                    item.state->abandon();
                else item.state->finish(seconds * 1000000000 + ns);
                wp_presentation_feedback_destroy(object);
                item.object = nullptr;
            },
            [](void *data, struct wp_presentation_feedback *object) {
                auto &item = *static_cast<Feedback *>(data);
                item.state->finish(0);
                wp_presentation_feedback_destroy(object);
                item.object = nullptr;
            }
        };
        wp_presentation_feedback_add_listener(item.object, &listener, &item);
        return item.state;
    }
    void poll() {
        if (wl_display_dispatch_queue_pending(display, queue) < 0
            || (wl_display_flush(display) < 0 && errno != EAGAIN)) {
            for (auto &item : feedback) item.state->abandon();
        }
        for (auto it = feedback.begin(); it != feedback.end();) {
            if (it->state->status.load(std::memory_order_acquire) == GpuPresentationData::Status::Pending) { ++it; continue; }
            if (it->object) wp_presentation_feedback_destroy(it->object);
            it = feedback.erase(it);
        }
    }
};
}
#endif
struct GpuWindowSurface::Private {
    WGPUSurface surface = nullptr;
    Display *display = nullptr;
    WId windowId = 0;
#ifdef LIBREPAINT_GPU_WAYLAND
    std::unique_ptr<WaylandPresentation> wayland;
#endif
    Private(KisGpuDevice &device, QWindow &window) {
#ifdef LIBREPAINT_GPU_WAYLAND
        if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
            wayland = std::make_unique<WaylandPresentation>();
            wayland->initialize(window);
            WGPUSurfaceSourceWaylandSurface source{};
            source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
            source.display = wayland->display;
            source.surface = wayland->window;
            WGPUSurfaceDescriptor descriptor{};
            descriptor.nextInChain = &source.chain;
            surface = wgpuInstanceCreateSurface(device.instance(), &descriptor);
            if (!surface) throw std::runtime_error("Cannot create GPU Wayland surface");
            return;
        }
#endif
        if (QGuiApplication::platformName() != QStringLiteral("xcb"))
            throw std::runtime_error("GPU window requires the X11 Qt platform");
        const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
        if (!native || !native->display()) throw std::runtime_error("GPU window requires an X11 display");
        WGPUSurfaceSourceXlibWindow xlib{};
        xlib.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        display = native->display();
        windowId = window.winId();
        xlib.display = display;
        xlib.window = windowId;
        WGPUSurfaceDescriptor descriptor{};
        descriptor.nextInChain = &xlib.chain;
        surface = wgpuInstanceCreateSurface(device.instance(), &descriptor);
        if (!surface) throw std::runtime_error("Cannot create GPU window surface");
    }
    ~Private() {
        // Release Vulkan's surface before the owned Wayland child is destroyed.
        wgpuSurfaceUnconfigure(surface);
        wgpuSurfaceRelease(surface);
    }
};
void SurfaceRenderer::prepareWindow(QWindow &window) {
    if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
        window.setSurfaceType(QSurface::RasterSurface);
        auto format = window.requestedFormat();
        format.setAlphaBufferSize(8);
        window.setFormat(format);
        window.setFlag(Qt::FramelessWindowHint);
    }
}
GpuWindowSurface::GpuWindowSurface(KisGpuDevice &device, QWindow &window) : d(new Private(device, window)) {}
GpuWindowSurface::~GpuWindowSurface() = default;
WGPUSurface GpuWindowSurface::surface() const { return d->surface; }
WGPUPresentMode GpuWindowSurface::presentationMode() const {
#ifdef LIBREPAINT_GPU_WAYLAND
    if (d->wayland) return WGPUPresentMode_Mailbox;
#endif
    return WGPUPresentMode_Fifo;
}
void GpuWindowSurface::poll() {
#ifdef LIBREPAINT_GPU_WAYLAND
    if (d->wayland) d->wayland->poll();
#endif
}
SurfaceRenderer::Presentation GpuWindowSurface::presentation() const {
#ifdef LIBREPAINT_GPU_WAYLAND
    if (d->wayland) return SurfaceRenderer::Presentation(d->wayland->request());
#endif
    return {};
}
std::optional<quint64> GpuWindowSurface::presentationClockNanoseconds() {
#ifdef LIBREPAINT_GPU_WAYLAND
    if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
        // Cache the immutable advertised clock value independently of any surface instance.
        static const auto clock = []() -> std::optional<clockid_t> {
            const auto app = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
            if (!app || !app->display()) return {};
            try {
                WaylandPresentation connection;
                connection.initializeConnection(app->display());
                return connection.clock;
            } catch (const std::runtime_error &) { return {}; }
        }();
        if (!clock) return {};
        timespec now{};
        if (clock_gettime(*clock, &now) == 0)
            return quint64(now.tv_sec) * 1000000000 + quint64(now.tv_nsec);
    }
#endif
    return {};
}
bool GpuWindowSurface::resize(QSize size) {
#ifdef LIBREPAINT_GPU_WAYLAND
    if (d->wayland) { d->wayland->resize(); return true; }
#endif
    XWindowAttributes attributes{};
    return XGetWindowAttributes(d->display, d->windowId, &attributes)
        && attributes.width == size.width() && attributes.height == size.height();
}
}
