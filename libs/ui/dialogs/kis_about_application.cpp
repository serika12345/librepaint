/*
 *  SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_about_application.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QFile>
#include <QStandardPaths>

#include <kis_debug.h>
#include <kis_global.h>

#include "workspace/kis_splash_screen.h"
#include "ui_wdgaboutapplication.h"
#include <KisPortingUtils.h>

class Q_DECL_HIDDEN WdgAboutApplication : public QWidget, public Ui::WdgAboutApplication
{
public:
    WdgAboutApplication(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

KisAboutApplication::KisAboutApplication(QWidget *parent)
    : KoDialog(parent)
{
    setWindowTitle(i18n("About LibrePaint"));
    setButtons(KoDialog::Close);

    WdgAboutApplication *wdgTab = new WdgAboutApplication(this);

    KisSplashScreen *splash = new KisSplashScreen();
    splash->setWindowFlags(Qt::Widget);
    splash->displayLinks(true);

    wdgTab->aboutTab->layout()->addWidget(splash);

    // LIBREPAINT-BRAND-AUDIT: upstream-attribution
    QString authors = i18n("<html>"
                           "<head/>"
                           "<body>"
                           "<h1 align=\"center\">Upstream Krita Developers</h1>"
                           "<p>");

    QFile fileDevelopers(":/developers.txt");
    Q_ASSERT(fileDevelopers.exists());
    if (fileDevelopers.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream developersText(&fileDevelopers);
        KisPortingUtils::setUtf8OnStream(developersText);
        authors.append(developersText.readAll().split("\n", Qt::SkipEmptyParts).join(", "));
    }
    authors.append(".</p></body></html>");
    wdgTab->lblAuthors->setText(authors);

    // Translators
    // TODO: move to KisApplication after string freeze is lifted
    KAboutData aboutData(KAboutData::applicationData());
    if (aboutData.translators().isEmpty()) {
        aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                                i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    }

    QString translatorHtml = i18n(
        "<html>"
        "<head/>"
        "<body>"
        "<h1 align=\"center\"><b>Translators</b></h1>"
        "<p><ul>");

    Q_FOREACH (const KAboutPerson &person, aboutData.translators()) {
        translatorHtml.append(QString("<li>%1</li>").arg(person.name()));
    }

    translatorHtml.append("<ul></p>");
    translatorHtml.append(
        // LIBREPAINT-BRAND-AUDIT: upstream-attribution
        i18n("<p>The translations bundled with LibrePaint are inherited from Krita and "
             "provided by KDE translation teams around the world.</p><p>For more information "
             "about KDE internationalization, visit <a href=\"https://l10n.kde.org\">https://"
             "l10n.kde.org</a>.</p>"));
    translatorHtml.append("</body></html>");

    wdgTab->lblTranslators->setText(translatorHtml);

    // LIBREPAINT-BRAND-AUDIT: upstream-attribution
    QString credits = i18n("<html>"
                           "<head/>"
                           "<body>"
                           "<h1 align=\"center\">Upstream Krita Credits</h1>"
                           "<p>");

    QFile fileCredits(":/credits.txt");
    Q_ASSERT(fileCredits.exists());
    if (fileCredits.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream creditsText(&fileCredits);
        KisPortingUtils::setUtf8OnStream(creditsText);

        Q_FOREACH (const QString &credit, creditsText.readAll().split('\n', Qt::SkipEmptyParts)) {

            if (credit.contains(":")) {
                QList<QString> creditSplit = credit.split(':');
                credits.append(creditSplit.at(0));
                credits.append(" (<i>" + creditSplit.at(1) + "</i>)");
                credits.append(", ");
            }
        }
        credits.chop(2);
    }
    // LIBREPAINT-BRAND-AUDIT: upstream-attribution
    credits.append(i18n(".</p><p><i>For supporting upstream Krita development with advice, icons, brush sets and more.</i></p></body></html>"));

    wdgTab->lblCredits->setText(credits);

    // LIBREPAINT-BRAND-AUDIT: upstream-attribution
    QString license = i18n("<html>"
                           "<head/>"
                           "<body>"
                           "<h1 align=\"center\"><b>Legal Notice</b></h1>"
                           "<p>LibrePaint is a modified work based on Krita.</p>"
                           "<p>LibrePaint is distributed under version 3 of the GNU General Public License, without any warranty.</p>"
                           "<p>Individual files, bundled assets, and third-party components remain subject to their own copyright and license notices.</p>"
                           "<p>The complete GNU General Public License version 3 follows.</p>"
                           "<hr/><pre>");

    QFile licenseFile(":/LICENSE");
    Q_ASSERT(licenseFile.exists());
    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream licenseText(&licenseFile);
        KisPortingUtils::setUtf8OnStream(licenseText);
        license.append(licenseText.readAll());
    }
    license.append("</pre></body></html>");
    wdgTab->lblLicense->setText(license);

    QFile thirdPartyFile(":/libraries.txt");
    if (thirdPartyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream thirdPartyText(&thirdPartyFile);
        KisPortingUtils::setUtf8OnStream(thirdPartyText);

        QString thirdPartyHtml = i18n("<html>"
                                      "<head/>"
                                      "<body>"
                                      "<h1 align=\"center\"><b>Third-party Software Reference</b></h1>"
                                      "<p>The source tree and platform-specific builds use or may include components from the following projects. "
                                      "The exact component set and license terms are identified by the notices supplied with each distribution.</p><p><ul>");

        Q_FOREACH (const QString &lib, thirdPartyText.readAll().split('\n', Qt::SkipEmptyParts)) {

            if (!lib.startsWith("#")) {
                QStringList parts = lib.split(',');
                if (parts.size() >= 3) {
                    thirdPartyHtml.append(QString("<li><a href=\"%2\">%1</a>: %3</li>").arg(parts[0], parts[1], parts[2]));
                }
            }
        }
        thirdPartyHtml.append("<ul></p></body></html>");
        wdgTab->lblThirdParty->setText(thirdPartyHtml);
    }

    setMainWidget(wdgTab);
    setMinimumSize(sizeHint());
    Q_ASSERT(layout());
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}
