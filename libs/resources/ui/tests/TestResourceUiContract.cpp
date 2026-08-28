/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointer>

#include <KisResourceItemChooser.h>
#include <KisResourceItemChooser_p.h>
#include <KisResourceTypes.h>
#include <KisResourceUiDescriptor.h>

namespace
{
int constructionCount = 0;
QString constructedResourceType;
bool constructedPreview = false;
}

void KisResourceItemChooser::constructPresentation()
{
    ++constructionCount;
    constructedResourceType = d->resourceType;
    constructedPreview = d->usePreview;
}

void KisResourceItemChooser::slotButtonClicked(int)
{
}

void KisResourceItemChooser::slotScrollerStateChanged(QScroller::State)
{
}

void KisResourceItemChooser::updateView()
{
}

void KisResourceItemChooser::scrollBackwards()
{
}

void KisResourceItemChooser::scrollForwards()
{
}

void KisResourceItemChooser::activate(const QModelIndex &)
{
}

void KisResourceItemChooser::clicked(const QModelIndex &)
{
}

void KisResourceItemChooser::contextMenuRequested(const QPoint &)
{
}

void KisResourceItemChooser::baseLengthChanged(int)
{
}

void KisResourceItemChooser::afterFilterChanged()
{
}

void KisResourceItemChooser::slotSaveSplitterState()
{
}

bool KisResourceItemChooser::eventFilter(QObject *, QEvent *)
{
    return false;
}

void KisResourceItemChooser::showEvent(QShowEvent *)
{
}

void KisResourceItemChooser::resizeEvent(QResizeEvent *)
{
}

QString KoResource::storageLocation() const
{
    return {};
}

bool KoResource::valid() const
{
    return false;
}

QString KoResource::md5Sum(bool) const
{
    return {};
}

int KoResource::version() const
{
    return 0;
}

QString KoResource::filename() const
{
    return {};
}

class TestResourceUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void descriptorPreservesCatalogIdentity();
    void chooserTypesKeepStableValues();
    void chooserOwnsDescriptorAndDelegatesPresentation();
    void chooserDestructionInvalidatesGuard();
};

void TestResourceUiContract::descriptorPreservesCatalogIdentity()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Palettes, true);

    QCOMPARE(descriptor.resourceType(), ResourceType::Palettes);
    QVERIFY(descriptor.previewEnabled());
}

void TestResourceUiContract::chooserTypesKeepStableValues()
{
    QCOMPARE(int(KisResourceItemChooser::Button_Import), 0);
    QCOMPARE(int(KisResourceItemChooser::Button_Remove), 1);
    QCOMPARE(int(KisResourceItemChooser::Layout::NotSet), 0);
    QCOMPARE(int(KisResourceItemChooser::Layout::Vertical), 1);
    QCOMPARE(int(KisResourceItemChooser::Layout::Horizontal2Rows), 2);
    QCOMPARE(int(KisResourceItemChooser::Layout::Horizontal1Row), 3);
}

void TestResourceUiContract::chooserOwnsDescriptorAndDelegatesPresentation()
{
    const KisResourceUiDescriptor descriptor(ResourceType::Patterns, false);
    QWidget parent;
    constructionCount = 0;
    constructedResourceType.clear();
    constructedPreview = true;
    KisResourceItemChooser chooser(descriptor, &parent);

    QCOMPARE(chooser.parentWidget(), &parent);
    QCOMPARE(constructionCount, 1);
    QCOMPARE(constructedResourceType, ResourceType::Patterns);
    QVERIFY(!constructedPreview);
    QCOMPARE(chooser.descriptor().resourceType(), ResourceType::Patterns);
    QVERIFY(!chooser.descriptor().previewEnabled());
}

void TestResourceUiContract::chooserDestructionInvalidatesGuard()
{
    constructionCount = 0;
    QPointer<KisResourceItemChooser> chooser = new KisResourceItemChooser(
        KisResourceUiDescriptor(ResourceType::Palettes, true));

    QCOMPARE(constructionCount, 1);
    QVERIFY(chooser);
    delete chooser.data();
    QVERIFY(chooser.isNull());
}

QTEST_MAIN(TestResourceUiContract)

#include "TestResourceUiContract.moc"
