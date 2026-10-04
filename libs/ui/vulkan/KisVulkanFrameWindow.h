/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_VULKAN_FRAME_WINDOW_H
#define KIS_VULKAN_FRAME_WINDOW_H

#include <QImage>
#include <QPointer>
#include <QVulkanWindow>
#include <QWidget>

/** Presents opaque, display-encoded RGBA frames. Instance and receiver are borrowed.
 * The instance outlives the window. Calls and input delivery use the GUI thread.
 */
class KisVulkanFrameWindow : public QVulkanWindow
{
    Q_OBJECT
public:
    explicit KisVulkanFrameWindow(QVulkanInstance *instance, QWidget *inputReceiver = nullptr);
    void setFrame(const QImage &frame);
    QImage pendingFrame() const { return m_frame; }
    void reportFailure(const QString &message);

Q_SIGNALS:
    void renderFailed(const QString &message);
    void frameSubmitted();

protected:
    QVulkanWindowRenderer *createRenderer() override;
    bool event(QEvent *event) override;

private:
    QImage m_frame;
    QPointer<QWidget> m_inputReceiver;
};

#endif
