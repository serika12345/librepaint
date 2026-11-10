// SPDX-License-Identifier: GPL-3.0-or-later
//
// Capture-based pen latency measurement for Windows.
//
//   windows_capture --out DIR --snapshot
//       Saves first-frame.bmp and canvas.json (the white canvas rectangle in
//       desktop pixels of the primary output).
//   windows_capture --out DIR --snapshot-frame NAME
//       Saves one full desktop frame as <OUT>\<NAME>.bmp and exits. The
//       display check uses it to compare the screen with the original image.
//   windows_capture --out DIR --plan plan.csv [--mouse]
//       Takes the snapshot, applies plan.csv (trajectory.py, relative to the
//       canvas) to the detected canvas, replays it as synthetic pen input
//       (PT_PEN; --mouse uses SendInput instead) while DXGI Desktop
//       Duplication captures the primary output, and writes inject.csv and
//       capture.csv in the format of mutter_capture.py.
//
// Times are QueryPerformanceCounter values converted to nanoseconds, the
// clock of PresentMon --qpc_time and of the LibrePaint latency probe.
// frame_ns is DXGI_OUTDUPL_FRAME_INFO::LastPresentTime. Run inside the
// interactive desktop session (run-surface.ps1).
//
// Build: x86_64-w64-mingw32-g++ -std=c++17 -O2 -static windows_capture.cpp
//        -ld3d11 -ldxgi -lwinmm -o windows_capture.exe
// A cross wrapper that links iconv/intl statically may also need their -L
// directories.

#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

LARGE_INTEGER frequency;
long long toNs(long long qpc) { return (long long)((double)qpc * 1e9 / (double)frequency.QuadPart); }
long long nowNs() { LARGE_INTEGER v; QueryPerformanceCounter(&v); return toNs(v.QuadPart); }

struct Band { std::string name; int x0, x1, y0, y1; };
struct Sample { double t; std::string phase, band; double x, y; int pressed; };

template <typename T> void release(T *&p) { if (p) { p->Release(); p = nullptr; } }

class Duplicator {
public:
    bool open()
    {
        D3D_FEATURE_LEVEL level;
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                     &device, &level, &context))) return fail("D3D11CreateDevice");
        IDXGIDevice *dxgiDevice = nullptr; IDXGIAdapter *adapter = nullptr; IDXGIOutput *output = nullptr;
        device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
        dxgiDevice->GetAdapter(&adapter);
        if (FAILED(adapter->EnumOutputs(0, &output))) return fail("EnumOutputs");
        output->QueryInterface(__uuidof(IDXGIOutput1), (void **)&output1);
        DXGI_OUTPUT_DESC desc; output->GetDesc(&desc);
        origin = {desc.DesktopCoordinates.left, desc.DesktopCoordinates.top};
        width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
        height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
        release(output); release(adapter); release(dxgiDevice);
        if (FAILED(output1->DuplicateOutput(device, &duplication))) return fail("DuplicateOutput");
        return true;
    }

    // Waits for the next desktop update and maps the region; returns false on timeout.
    bool next(const RECT &region, long long *frameNs, D3D11_MAPPED_SUBRESOURCE *mapped, unsigned timeoutMs)
    {
        if (held) { duplication->ReleaseFrame(); held = false; }
        DXGI_OUTDUPL_FRAME_INFO info; IDXGIResource *resource = nullptr;
        HRESULT hr = duplication->AcquireNextFrame(timeoutMs, &info, &resource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
        if (FAILED(hr)) { fail("AcquireNextFrame"); return false; }
        held = true;
        if (info.LastPresentTime.QuadPart == 0) { release(resource); return false; } // pointer-only update
        ID3D11Texture2D *texture = nullptr;
        resource->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&texture);
        release(resource);
        int w = region.right - region.left, h = region.bottom - region.top;
        if (!staging || stagingWidth != w || stagingHeight != h) {
            release(staging);
            D3D11_TEXTURE2D_DESC desc; texture->GetDesc(&desc);
            desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
            device->CreateTexture2D(&desc, nullptr, &staging);
            stagingWidth = w; stagingHeight = h;
        }
        D3D11_BOX box{(UINT)region.left, (UINT)region.top, 0, (UINT)region.right, (UINT)region.bottom, 1};
        context->CopySubresourceRegion(staging, 0, 0, 0, 0, texture, 0, &box);
        release(texture);
        if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, mapped))) return false;
        *frameNs = toNs(info.LastPresentTime.QuadPart);
        return true;
    }

    void unmap() { context->Unmap(staging, 0); }

    POINT origin{0, 0};
    int width = 0, height = 0;

private:
    bool fail(const char *what) { fprintf(stderr, "%s failed\n", what); return false; }
    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    IDXGIOutput1 *output1 = nullptr;
    IDXGIOutputDuplication *duplication = nullptr;
    ID3D11Texture2D *staging = nullptr;
    int stagingWidth = 0, stagingHeight = 0;
    bool held = false;
};

bool dark(const unsigned char *p, int threshold) { return p[0] < threshold && p[1] < threshold && p[2] < threshold; }
bool white(const unsigned char *p) { return p[0] >= 245 && p[1] >= 245 && p[2] >= 245; }

void saveBmp(const std::string &path, const unsigned char *data, int pitch, int w, int h)
{
    std::ofstream file(path, std::ios::binary);
    BITMAPFILEHEADER fh{}; BITMAPINFOHEADER ih{};
    fh.bfType = 0x4D42; fh.bfOffBits = sizeof(fh) + sizeof(ih); fh.bfSize = fh.bfOffBits + w * h * 4;
    ih.biSize = sizeof(ih); ih.biWidth = w; ih.biHeight = -h; ih.biPlanes = 1; ih.biBitCount = 32;
    file.write((const char *)&fh, sizeof(fh)); file.write((const char *)&ih, sizeof(ih));
    for (int y = 0; y < h; ++y) file.write((const char *)data + y * pitch, w * 4);
}

// Same rule as mutter_capture.find_canvas.
bool findCanvas(const unsigned char *data, int pitch, int w, int h, int rect[4])
{
    auto at = [&](int x, int y) { return data + y * pitch + 4 * x; };
    long long best = -1; int cx = 0, cy = 0;
    for (int y = 0; y < h; y += 2)
        for (int x = 0; x < w; x += 2)
            if (white(at(x, y))) {
                long long d = (long long)(x - w / 2) * (x - w / 2) + (long long)(y - h / 2) * (y - h / 2);
                if (best < 0 || d < best) { best = d; cx = x; cy = y; }
            }
    if (best < 0) return false;
    int left = cx, right = cx, top = cy, bottom = cy;
    while (left > 0 && white(at(left - 1, cy))) left--;
    while (right < w - 1 && white(at(right + 1, cy))) right++;
    while (top > 0 && white(at(cx, top - 1))) top--;
    while (bottom < h - 1 && white(at(cx, bottom + 1))) bottom++;
    rect[0] = left + 8; rect[1] = top + 8; rect[2] = right - left + 1 - 16; rect[3] = bottom - top + 1 - 16;
    return true;
}

// Reads a plan relative to the canvas and maps it to the canvas rectangle in pixels.
std::vector<Sample> readPlan(const std::string &path, std::vector<Band> &bands, const int rect[4])
{
    auto px = [&](double f) { return rect[0] + f * rect[2]; };
    auto py = [&](double f) { return rect[1] + f * rect[3]; };
    std::vector<Sample> samples;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("# bands=", 0) == 0) {
            std::stringstream entries(line.substr(8));
            std::string entry;
            while (std::getline(entries, entry, ';')) {
                char name[64]; double x0, x1, y0, y1;
                if (sscanf(entry.c_str(), "%63[^:]:%lf:%lf:%lf:%lf", name, &x0, &x1, &y0, &y1) == 5)
                    bands.push_back({name, (int)px(x0), (int)px(x1), (int)py(y0), (int)py(y1) + 1});
            }
            continue;
        }
        Sample s; char phase[32], band[32];
        if (sscanf(line.c_str(), "%lf,%31[^,],%31[^,],%lf,%lf,%d", &s.t, phase, band, &s.x, &s.y, &s.pressed) == 6) {
            s.phase = phase; s.band = band; s.x = px(s.x); s.y = py(s.y); samples.push_back(s);
        }
    }
    return samples;
}

// Synthetic pen, matching PenInject.cs.
const UINT32 FLAG_INRANGE = 0x2, FLAG_INCONTACT = 0x4, FLAG_FIRSTBUTTON = 0x10;
const UINT32 FLAG_DOWN = 0x10000, FLAG_UPDATE = 0x20000, FLAG_UP = 0x40000;

class Pen {
public:
    bool open()
    {
        device = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);
        return device != nullptr;
    }
    bool inject(int x, int y, bool pressed, bool wasPressed)
    {
        POINTER_TYPE_INFO info{};
        info.type = PT_PEN;
        POINTER_PEN_INFO &pen = info.penInfo;
        pen.pointerInfo.pointerType = PT_PEN;
        pen.pointerInfo.frameId = ++frame;
        UINT32 flags = FLAG_INRANGE;
        if (pressed) flags |= FLAG_INCONTACT | FLAG_FIRSTBUTTON | (wasPressed ? FLAG_UPDATE : FLAG_DOWN);
        else flags |= wasPressed ? FLAG_UP : FLAG_UPDATE;
        pen.pointerInfo.pointerFlags = flags;
        pen.pointerInfo.ptPixelLocation = {x, y};
        pen.penMask = PEN_MASK_PRESSURE;
        pen.pressure = pressed ? 512 : 0;
        return InjectSyntheticPointerInput(device, &info, 1);
    }
    ~Pen() { if (device) DestroySyntheticPointerDevice(device); }
private:
    HSYNTHETICPOINTERDEVICE device = nullptr;
    UINT32 frame = 0;
};

bool injectMouse(int x, int y, bool pressed, bool wasPressed)
{
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)((x * 65535.0) / (GetSystemMetrics(SM_CXSCREEN) - 1));
    input.mi.dy = (LONG)((y * 65535.0) / (GetSystemMetrics(SM_CYSCREEN) - 1));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    if (pressed && !wasPressed) input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
    if (!pressed && wasPressed) input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
    return SendInput(1, &input, sizeof(input)) == 1;
}

} // namespace

int main(int argc, char **argv)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
    QueryPerformanceFrequency(&frequency);
    std::string out, planPath;
    bool snapshot = false, mouse = false;
    std::string snapshotFrame;
    int threshold = 128, dense = 6;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--out" && i + 1 < argc) out = argv[++i];
        else if (a == "--plan" && i + 1 < argc) planPath = argv[++i];
        else if (a == "--snapshot-frame" && i + 1 < argc) snapshotFrame = argv[++i];
        else if (a == "--dark" && i + 1 < argc) threshold = atoi(argv[++i]);
        else if (a == "--dense" && i + 1 < argc) dense = atoi(argv[++i]);
        else if (a == "--snapshot") snapshot = true;
        else if (a == "--mouse") mouse = true;
    }
    if (out.empty() || (!snapshot && snapshotFrame.empty() && planPath.empty())) {
        fprintf(stderr, "usage: windows_capture --out DIR (--snapshot | --snapshot-frame NAME | --plan plan.csv [--mouse])\n");
        return 2;
    }
    CreateDirectoryA(out.c_str(), nullptr);

    Duplicator dup;
    if (!dup.open()) return 1;
    DEVMODEA mode{}; mode.dmSize = sizeof(mode);
    EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &mode);
    char meta[512];
    snprintf(meta, sizeof(meta), "{\"frame\": [%d, %d], \"origin\": [%ld, %ld], \"refresh\": %lu", dup.width, dup.height,
             dup.origin.x, dup.origin.y, mode.dmDisplayFrequency);

    int rect[4];
    {
        // Park the pointer at the corner, then wait for a desktop update.
        SetCursorPos(dup.origin.x + 1, dup.origin.y + dup.height - 2);
        RECT full{0, 0, dup.width, dup.height};
        long long frameNs; D3D11_MAPPED_SUBRESOURCE mapped;
        bool got = false;
        for (int i = 0; i < 50 && !got; ++i) got = dup.next(full, &frameNs, &mapped, 100);
        if (!got) {
            // An idle desktop may produce no update; force one with a tiny cursor move.
            SetCursorPos(dup.origin.x + 2, dup.origin.y + dup.height - 3);
            for (int i = 0; i < 50 && !got; ++i) got = dup.next(full, &frameNs, &mapped, 100);
        }
        if (!got) { fprintf(stderr, "no frame captured\n"); return 1; }
        if (!snapshotFrame.empty()) {
            saveBmp(out + "\\" + snapshotFrame + ".bmp", (const unsigned char *)mapped.pData, mapped.RowPitch,
                    dup.width, dup.height);
            dup.unmap();
            printf("%s\\%s.bmp\n", out.c_str(), snapshotFrame.c_str());
            return 0;
        }
        saveBmp(out + "\\first-frame.bmp", (const unsigned char *)mapped.pData, mapped.RowPitch, dup.width, dup.height);
        bool found = findCanvas((const unsigned char *)mapped.pData, mapped.RowPitch, dup.width, dup.height, rect);
        dup.unmap();
        if (!found) { fprintf(stderr, "no white canvas visible\n"); return 1; }
        std::ofstream(out + "\\canvas.json") << meta << ", \"canvas\": [" << rect[0] << ", " << rect[1] << ", " << rect[2]
                                            << ", " << rect[3] << "]}\n";
        printf("%s, \"canvas\": [%d, %d, %d, %d]}\n", meta, rect[0], rect[1], rect[2], rect[3]);
        if (snapshot) return 0;
    }

    std::vector<Band> bands;
    std::vector<Sample> samples = readPlan(planPath, bands, rect);
    if (bands.empty() || samples.empty()) { fprintf(stderr, "empty plan\n"); return 1; }
    RECT region{bands[0].x0, bands[0].y0, bands[0].x1, bands[0].y1};
    for (const Band &b : bands) {
        region.left = std::min<LONG>(region.left, b.x0); region.right = std::max<LONG>(region.right, b.x1);
        region.top = std::min<LONG>(region.top, b.y0); region.bottom = std::max<LONG>(region.bottom, b.y1);
    }

    std::atomic<bool> stop{false};
    std::string rows = "frame_ns,arrival_ns,band,xmin,xmax,dense_xmin,dense_xmax,ymin,ymax\n";
    std::thread capture([&] {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        char line[256];
        while (!stop) {
            long long frameNs; D3D11_MAPPED_SUBRESOURCE mapped;
            if (!dup.next(region, &frameNs, &mapped, 50)) continue;
            long long arrival = nowNs();
            const unsigned char *data = (const unsigned char *)mapped.pData;
            for (const Band &b : bands) {
                int xmin = -1, xmax = -1, dxmin = -1, dxmax = -1, ymin = -1, ymax = -1;
                std::vector<int> columns(b.x1 - b.x0, 0);
                for (int y = b.y0; y < b.y1; ++y) {
                    const unsigned char *row = data + (y - region.top) * mapped.RowPitch;
                    bool any = false;
                    for (int x = b.x0; x < b.x1; ++x)
                        if (dark(row + 4 * (x - region.left), threshold)) { columns[x - b.x0]++; any = true; }
                    if (any) { if (ymin < 0) ymin = y; ymax = y; }
                }
                for (int i = 0; i < (int)columns.size(); ++i) {
                    if (columns[i] > 0) { if (xmin < 0) xmin = b.x0 + i; xmax = b.x0 + i; }
                    if (columns[i] >= dense) { if (dxmin < 0) dxmin = b.x0 + i; dxmax = b.x0 + i; }
                }
                if (xmin < 0) snprintf(line, sizeof(line), "%lld,%lld,%s,,,,,,\n", frameNs, arrival, b.name.c_str());
                else if (dxmin < 0) snprintf(line, sizeof(line), "%lld,%lld,%s,%d,%d,,,%d,%d\n", frameNs, arrival, b.name.c_str(), xmin, xmax, ymin, ymax);
                else snprintf(line, sizeof(line), "%lld,%lld,%s,%d,%d,%d,%d,%d,%d\n", frameNs, arrival, b.name.c_str(), xmin, xmax, dxmin, dxmax, ymin, ymax);
                rows += line;
            }
            dup.unmap();
        }
    });

    Pen pen;
    if (!mouse && !pen.open()) { fprintf(stderr, "CreateSyntheticPointerDevice failed: %lu\n", GetLastError()); stop = true; capture.join(); return 1; }
    timeBeginPeriod(1);
    std::string injected = "time_ns,phase,band,x,y,pressed\n";
    char line[160];
    int pressed = 0;
    long long start = nowNs() + 300000000LL;
    int failures = 0;
    for (const Sample &s : samples) {
        long long target = start + (long long)(s.t * 1e9);
        while (true) {
            long long remaining = target - nowNs();
            if (remaining <= 0) break;
            if (remaining > 2000000) Sleep(1);
        }
        int x = dup.origin.x + (int)(s.x + 0.5), y = dup.origin.y + (int)(s.y + 0.5);
        long long t = nowNs();
        bool ok = mouse ? injectMouse(x, y, s.pressed, pressed) : pen.inject(x, y, s.pressed, pressed);
        if (!ok) failures++;
        pressed = s.pressed;
        snprintf(line, sizeof(line), "%lld,%s,%s,%.2f,%.2f,%d\n", t, s.phase.c_str(), s.band.c_str(), s.x, s.y, s.pressed);
        injected += line;
    }
    timeEndPeriod(1);
    Sleep(500);
    stop = true;
    capture.join();
    std::ofstream(out + "\\capture.csv", std::ios::binary) << rows;
    std::ofstream(out + "\\inject.csv", std::ios::binary) << injected;
    std::ofstream(out + "\\capture-meta.json") << meta << ", \"inject_failures\": " << failures << "}\n";
    printf("samples=%zu failures=%d\n", samples.size(), failures);
    return failures ? 1 : 0;
}
