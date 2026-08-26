/*
 *  SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2021 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_SPLASH_SCREEN_H
#define KIS_SPLASH_SCREEN_H

#include <QWidget>
#include <QTimer>

#include "ui_wdgsplash.h"

#include "kritaui_export.h"

class KRITAUI_EXPORT KisSplashScreen : public QWidget, public Ui::WdgSplash
{
    Q_OBJECT
public:
    explicit KisSplashScreen(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void repaint();

    void show();
    void displayLinks(bool show);
    void displayRecentFiles(bool show);

    void setLoadingText(QString text);

    static QString imageResourcePath();

private Q_SLOTS:
    void linkClicked(const QString &link);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void centerOnScreen();
    void updateText();
    QString colorString() const;
    void updateSplashImage();

private:

    QTimer m_timer;
    bool m_displayLinks { false };
    QLabel *m_loadingTextLabel;
    QString m_versionHtml;
};

#endif // KIS_SPLASH_SCREEN_H
