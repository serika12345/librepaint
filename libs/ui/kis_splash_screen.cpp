/*
 *  SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2021 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_splash_screen.h"

#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QCheckBox>
#include <kis_debug.h>
#include <QFile>
#include <QScreen>
#include <QWindow>
#include <QSvgWidget>

#include <KisPart.h>
#include <KisApplication.h>

#include <kis_icon.h>

#include <klocalizedstring.h>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>

KisSplashScreen::KisSplashScreen(bool themed, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint | f)
      , m_versionHtml(qApp->applicationVersion().toHtmlEscaped())
{
    Q_UNUSED(themed);

    setupUi(this);
#ifndef Q_OS_MACOS
    setWindowIcon(KisIconUtils::loadIcon("krita-branding"));
#endif

    m_loadingTextLabel = new QLabel(lblSplash);
    m_loadingTextLabel->setTextFormat(Qt::RichText);
    m_loadingTextLabel->setStyleSheet(QStringLiteral("QLabel { color: #171A21; background-color: transparent; }"));
    m_loadingTextLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);

    m_brandingSvg = new QSvgWidget(QStringLiteral(":/krita-branding.svgz"), lblSplash);
    m_bannerSvg = new QSvgWidget(QStringLiteral(":/splash/banner.svg"), lblSplash);
    // The LibrePaint splash is a complete mark-and-wordmark composition.
    // Keep the legacy resources available to other callers, but do not draw
    // the old split mark/banner overlay on top of the complete splash.
    m_brandingSvg->hide();
    m_bannerSvg->hide();

    m_artCreditsLabel = new QLabel(lblSplash);
    m_artCreditsLabel->setTextFormat(Qt::PlainText);
    m_artCreditsLabel->setStyleSheet(QStringLiteral("QLabel { color: #171A21; background-color: transparent; font: 10pt; }"));
    m_artCreditsLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    updateSplashImage();
    setLoadingText(QString());

    bnClose->hide();
    connect(bnClose, SIGNAL(clicked()), this, SLOT(close()));
    chkShowAtStartup->hide();
    connect(chkShowAtStartup, SIGNAL(toggled(bool)), this, SLOT(toggleShowAtStartup(bool)));

    KConfigGroup cfg( KSharedConfig::openConfig(), "SplashScreen");
    bool hideSplash = cfg.readEntry("HideSplashAfterStartup", false);
    chkShowAtStartup->setChecked(hideSplash);

    connect(lblRecent, SIGNAL(linkActivated(QString)), SLOT(linkClicked(QString)));
    connect(&m_timer, SIGNAL(timeout()), SLOT(raise()));

    // hide these labels by default
    displayLinks(false);
    displayRecentFiles(false);

    m_timer.setSingleShot(true);
    m_timer.start(10);
}

void KisSplashScreen::updateSplashImage()
{
    constexpr int SPLASH_HEIGHT_LOADING = 480;
    constexpr int SPLASH_HEIGHT_ABOUT = 320;

    int splashHeight;
    if (m_displayLinks) {
        splashHeight = SPLASH_HEIGHT_ABOUT;
    } else {
        splashHeight = SPLASH_HEIGHT_LOADING;
    }
    Source source = getImageSource();
    QPixmap img(source.resourcePath);

    if (img.isNull() || img.height() == 0) return;

    // Preserve aspect ratio of splash.
    int height = splashHeight;
    int width = height * img.width() / img.height();

#ifdef Q_OS_IOS
    // The first QScreen geometry can still describe the launch orientation.
    // Its short edge is nevertheless valid for either orientation, so fitting
    // inside it prevents a landscape-sized splash from being clipped when the
    // application starts in portrait (and also covers narrow Split View).
    if (QScreen *splashScreen = screen()) {
        constexpr int SCREEN_MARGIN = 24;
        const QSize availableSize = splashScreen->availableGeometry().size();
        const int shortEdge = qMin(availableSize.width(), availableSize.height());
        if (shortEdge > 2 * SCREEN_MARGIN) {
            const int maximumEdge = shortEdge - 2 * SCREEN_MARGIN;
            const QSize fittedSize = QSize(width, height).scaled(
                QSize(maximumEdge, maximumEdge), Qt::KeepAspectRatio);
            width = fittedSize.width();
            height = fittedSize.height();
        }
    }
#endif

    const int bannerHeight = height * 0.16875;
    const int marginTop = height * 0.05;
    const int marginRight = height * 0.1;

    setFixedWidth(width);
    setFixedHeight(height);
    lblSplash->setFixedWidth(width);
    lblSplash->setFixedHeight(height);

    // Get a downscaled pixmap of the splash.
    const int pixelWidth = width * devicePixelRatioF();
    const int pixelHeight = height * devicePixelRatioF();
    img = img.scaled(pixelWidth, pixelHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    img.setDevicePixelRatio(devicePixelRatioF());
    lblSplash->setPixmap(img);

    // Align banner to top-left with margin.
    m_bannerSvg->setFixedHeight(bannerHeight);
    m_bannerSvg->setFixedWidth(bannerHeight * m_bannerSvg->sizeHint().width() / m_bannerSvg->sizeHint().height());
    m_bannerSvg->move(width - m_bannerSvg->width() - marginRight, marginTop);

    // Place logo to the left of banner.
    m_brandingSvg->setFixedSize(bannerHeight, bannerHeight);
    m_brandingSvg->move(m_bannerSvg->x() - m_brandingSvg->width(), marginTop);

    // Place loading text immediately below.
    m_loadingTextLabel->move(marginRight, m_brandingSvg->geometry().bottom());
    m_loadingTextLabel->setFixedWidth(m_bannerSvg->geometry().right() - marginRight);

    // Place credits text on bottom right with similar margins.
    m_artCreditsLabel->setText(source.artistCredit);
    m_artCreditsLabel->setFixedWidth(m_loadingTextLabel->width());
    m_artCreditsLabel->setFixedHeight(20);
    m_artCreditsLabel->move(m_loadingTextLabel->x(), height - marginTop - m_artCreditsLabel->height());

    if (m_displayLinks) {
        setFixedSize(sizeHint());
    }
}

void KisSplashScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateText();
}

void KisSplashScreen::updateText()
{
    QString color = colorString();

    KConfigGroup cfg2( KSharedConfig::openConfig(), "RecentFiles");
    int i = 1;

    QString recent = i18n("<html>"
                          "<head/>"
                          "<body>"
                          "<p><b><span style=\" color:%1;\">Recent Files</span></b></p>", color);

    QString path;
    QStringList recentfiles;

    QFontMetrics metrics(lblRecent->font());

    do {
        path = cfg2.readPathEntry(QString("File%1").arg(i), QString());
        if (!path.isEmpty()) {
            QString name = cfg2.readPathEntry(QString("Name%1").arg(i), QString());
            QUrl url(path);
            if (name.isEmpty()) {
                name = url.fileName();
            }

            name = metrics.elidedText(name, Qt::ElideMiddle, lblRecent->width());

            if (!url.isLocalFile() || QFile::exists(url.toLocalFile())) {
                recentfiles.insert(0, QString("<p><a href=\"%1\"><span style=\"color:%3;\">%2</span></a></p>").arg(path).arg(name).arg(color));
            }
        }

        i++;
    } while (!path.isEmpty() || i <= 8);

    recent += recentfiles.join("\n");
    recent += "</body>"
        "</html>";
    lblRecent->setText(recent);
}

void KisSplashScreen::displayLinks(bool show) {

    if (show) {
        QString color = colorString();
        QStringList lblLinksText;
        lblLinksText    << "<html>"
                        << "<head/>"
                        << "<body><table style=\"width:100%\" cellpadding=\"30\"><tr><td>"
                        << i18n("<p><span style=\" color:%1;\"><b>Using LibrePaint</b></span></p>", color)
                        << i18n("<p><span style=\" color:%1;\">LibrePaint is the free paint app.</span></p>", color)
                        << "</td></tr></table></body>"
                        << "</html>";


        lblLinks->setTextFormat(Qt::RichText);
        lblLinks->setText(lblLinksText.join(""));

        filesLayout->setContentsMargins(10,10,10,10);
        actionControlsLayout->setContentsMargins(5,5,5,5);

    } else {
        // eliminating margins here allows for the splash screen image to take the entire area with nothing underneath
        filesLayout->setContentsMargins(0,0,0,0);
        actionControlsLayout->setContentsMargins(0,0,0,0);
    }

    lblLinks->setVisible(show);

    updateText();

    if (m_displayLinks != show) {
        m_displayLinks = show;
        updateSplashImage();
    }
}


void KisSplashScreen::displayRecentFiles(bool show) {
    lblRecent->setVisible(show);
    line->setVisible(show);
}

void KisSplashScreen::setLoadingText(QString text)
{
    int larger = 12;
    int notAsLarge = larger - 1;
    QString htmlText = QStringLiteral("<span style='font: %3pt;'><span style='font: bold %4pt;'>%1</span><br><i>%2</i></span>")
            .arg(m_versionHtml, text.toHtmlEscaped(), QString::number(notAsLarge), QString::number(larger));
    m_loadingTextLabel->setText(htmlText);
}

KisSplashScreen::Source KisSplashScreen::getImageSource()
{
    // LibrePaint deliberately ships a neutral placeholder instead of
    // third-party splash artwork, so there is no artwork credit to display.
    QString artistCredit;
    // Loading the ginormous 4K PNG splash image increases the startup time on
    // Android by several seconds and at the same time looks really bad when
    // scaled down to a dinky size. Instead of overengineering this into an
    // Enterprise Splash Screen Solution where we choose the image based on
    // screen size or something, we'll just use a HD JPEG instead. It's fine.
#ifdef Q_OS_ANDROID
    QString resourcePath = QStringLiteral(":/splash/hd.jpg");
#else
    QString resourcePath = QStringLiteral(":/splash/0.png");
    // TODO: Re-add the holiday splash...
#if 0
    QDate currentDate = QDate::currentDate();
    if (currentDate > QDate(currentDate.year(), 12, 4) ||
            currentDate < QDate(currentDate.year(), 1, 9)) {
        resourcePath = QStringLiteral(":/splash/1.png");
        artistCredit = QStringLiteral("???")};
    }
#endif
#endif
    if (!artistCredit.isEmpty()) {
        artistCredit = i18nc("splash image credit", "Artwork by: %1", artistCredit);
    }
    return Source{resourcePath, artistCredit};
}


QString KisSplashScreen::colorString() const
{
    // The bundled LibrePaint splash has a fixed white background.
    return QStringLiteral("#171A21");
}


void KisSplashScreen::repaint()
{
    QWidget::repaint();
    qApp->sendPostedEvents();
}

void KisSplashScreen::centerOnScreen()
{
    QWindow *windowHandle = this->windowHandle();
    QScreen *screen = windowHandle ? windowHandle->screen() : nullptr;
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect splashRect(QPoint(), size());
    move(screen->availableGeometry().center() - splashRect.center());
}

void KisSplashScreen::show()
{
    if (!this->parentWidget()) {
        this->winId(); // Force creation of native window
        QWindow *windowHandle = this->windowHandle();
        QScreen *screen = windowHandle ? windowHandle->screen() : nullptr;
        if (windowHandle && !screen) {
            // At least on Windows, the window may be created on a non-primary
            // screen with a different scale factor. If we don't explicitly
            // move it to the primary screen, the position will be scaled with
            // the wrong factor and the splash will be offset.
            // XXX: In theory this branch should be unreachable, but leaving
            //      this here just in case.
            windowHandle->setScreen(QApplication::primaryScreen());
        }
        if (!screen) {
            screen = QApplication::primaryScreen();
        }
        // Reinitialize the splash image as the screen may have a different
        // devicePixelRatio.
        updateSplashImage();
        centerOnScreen();

#ifdef Q_OS_IOS
        // UIKit reports the initial screen geometry before the application
        // scene has settled into its requested orientation. Recenter when Qt
        // receives the final landscape/split-view geometry instead of leaving
        // the splash at coordinates calculated for the portrait screen.
        connect(screen, &QScreen::geometryChanged,
                this, &KisSplashScreen::centerOnScreen, Qt::UniqueConnection);
        connect(screen, &QScreen::availableGeometryChanged,
                this, &KisSplashScreen::centerOnScreen, Qt::UniqueConnection);
#endif
    }
    if (isVisible()) {
        repaint();
    }
    m_timer.setSingleShot(true);
    m_timer.start(1);
    QWidget::show();

#ifdef Q_OS_IOS
    // The native view is attached by QWidget::show(). Recenter once more on
    // the next event-loop turn so the first visible frame uses that geometry.
    QTimer::singleShot(0, this, &KisSplashScreen::centerOnScreen);
#endif
}

void KisSplashScreen::toggleShowAtStartup(bool toggle)
{
    KConfigGroup cfg( KSharedConfig::openConfig(), "SplashScreen");
    cfg.writeEntry("HideSplashAfterStartup", toggle);
}

void KisSplashScreen::linkClicked(const QString &link)
{
    KisPart::instance()->openExistingFile(link);
    if (isWindow()) {
        close();
    }
}
