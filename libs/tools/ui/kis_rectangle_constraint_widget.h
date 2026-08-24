/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISRECTANGLECONSTRAINTWIDGET_H
#define KISRECTANGLECONSTRAINTWIDGET_H

#include <QScopedPointer>
#include <QWidget>

#include <kritatoolsui_export.h>

class QRectF;
class QString;

class KRITATOOLSUI_EXPORT KisRectangleConstraintWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KisRectangleConstraintWidget(const QString &configGroup,
                                          bool showRoundCornersGui,
                                          QWidget *parent = nullptr);
    ~KisRectangleConstraintWidget() override;

public Q_SLOTS:
    void setRectangle(const QRectF &rect);
    void reloadConfig();

Q_SIGNALS:
    void constraintsChanged(bool forceRatio,
                            bool forceWidth,
                            bool forceHeight,
                            float ratio,
                            float width,
                            float height);
    void roundCornersChanged(int xRadius, int yRadius);

private Q_SLOTS:
    void inputsChanged();
    void inputWidthChanged();
    void inputHeightChanged();
    void inputRatioChanged();
    void slotRoundCornersChanged();
    void slotRoundCornersAspectLockChanged();

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif
