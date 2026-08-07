#include "probe.h"

#include <KAboutData>
#include <KCharsets>
#include <KColorScheme>
#include <KCompletion>
#include <KCountry>
#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KTreeWidgetSearchLine>
#include <QApplication>
#include <QPalette>
#include <QStringList>
#include <QTreeWidget>

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    KAboutData about(QStringLiteral("krita-ios-frameworks-probe"),
                     QStringLiteral("LibrePaint iOS Frameworks Probe"),
                     QStringLiteral("1"));
    KAboutData::setApplicationData(about);

    ProbeSettings settings;
    KCompletion completion;
    completion.setItems({QStringLiteral("LibrePaint"), QStringLiteral("iPad")});
    const QString completed = completion.makeCompletion(QStringLiteral("Kr"));
    const QString translated = i18n("LibrePaint iOS Frameworks Probe");
    const QString codec = KCharsets::charsets()->encodingForName(QStringLiteral("UTF-8"));
    const KCountry country = KCountry::fromAlpha2("JP");
    const KGuiItem guiItem = KStandardGuiItem::ok();
    const KColorScheme colors(QPalette::Active, KColorScheme::View);

    QTreeWidget tree;
    KTreeWidgetSearchLine search(&tree);
    search.setText(completed);

    return settings.probeEnabled() && !translated.isEmpty() && !codec.isEmpty()
        && country.isValid() && !guiItem.text().isEmpty() && colors.foreground().color().isValid()
        ? 0
        : 1;
}
