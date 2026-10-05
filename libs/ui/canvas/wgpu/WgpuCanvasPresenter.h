/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KRITA_CANVAS_WGPU_CANVAS_PRESENTER_H
#define KRITA_CANVAS_WGPU_CANVAS_PRESENTER_H

#include <QImage>
#include <QObject>
#include <memory>
#include <array>

class QWindow;
namespace Krita::Canvas
{
/** Presents CPU-composed pixels on a borrowed native Qt window.
 * The window must outlive this object. Its normal Qt input and focus processing
 * remains authoritative. All updates and destruction run on the GUI thread.
 */
class WgpuCanvasPresenter : public QObject
{
public:
    explicit WgpuCanvasPresenter(QWindow &window);
    ~WgpuCanvasPresenter() override;
    bool setImage(const QImage &image, const QRect &dirty);
    void setProjectionGeometry(const std::array<float,16> &geometry);
    void setProjectionGeneration(quint64 generation);
    void queueProjectionPatch(const QImage &patch, const QRect &destination);
    QImage readback();
    quint64 submittedFrames() const;
    quint64 uploadedBytes() const;
    QString error() const;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void requestFrame();
    void render();
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
