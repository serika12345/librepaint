/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisCompositeOpListConnectionHelper.h"

#include <QMetaObject>
#include <QMetaProperty>
#include <KoCompositeOpRegistry.h>
#include <kis_assert.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <kis_cmb_composite.h>

namespace KisWidgetConnectionUtils
{
class ConnectCompositeOpListWidgetHelper : public QObject
{
    Q_OBJECT
public:

    ConnectCompositeOpListWidgetHelper(KisCompositeOpListWidget *parent)
        : QObject(parent),
          m_listWidget(parent)
    {
        connect(parent, &KisCompositeOpListWidget::clicked,
                this, &ConnectCompositeOpListWidgetHelper::slotWidgetChanged);
    }
public Q_SLOTS:
    void slotWidgetChanged() {
        Q_EMIT sigWidgetChanged(m_listWidget->selectedCompositeOp().id());
    }

    void slotPropertyChanged(const QString &id) {
        m_listWidget->setCompositeOp(KoCompositeOpRegistry::instance().getKoID(id));
    }

Q_SIGNALS:
    void sigWidgetChanged(const QString &id);

private:
    KisCompositeOpListWidget *m_listWidget;
};

void connectControl(KisCompositeOpListWidget *widget, QObject *source, const char *property)
{
    const QMetaObject* meta = source->metaObject();
    QMetaProperty prop = meta->property(meta->indexOfProperty(property));

    KIS_SAFE_ASSERT_RECOVER_RETURN(prop.hasNotifySignal());

    QMetaMethod signal = prop.notifySignal();

    KIS_SAFE_ASSERT_RECOVER_RETURN(signal.parameterCount() >= 1);
    KIS_SAFE_ASSERT_RECOVER_RETURN(signal.parameterType(0) == QMetaType::type("QString"));

    ConnectCompositeOpListWidgetHelper *helper = new ConnectCompositeOpListWidgetHelper(widget);

    const QMetaObject* dstMeta = helper->metaObject();

    QMetaMethod updateSlot = dstMeta->method(
                dstMeta->indexOfSlot("slotPropertyChanged(QString)"));
    QObject::connect(source, signal, helper, updateSlot);

    helper->slotPropertyChanged(prop.read(source).value<QString>());

    if (prop.isWritable()) {
        QObject::connect(helper, &ConnectCompositeOpListWidgetHelper::sigWidgetChanged, [prop, source] (const QString &value) { prop.write(source, QVariant::fromValue(value)); });
    }
}

namespace {

void connectBlendModeAction(KisActionManager *manager,
                            const char *actionName,
                            KisCompositeOpComboBox *widget,
                            const char *slot)
{
    KisAction *action = manager->createAction(actionName);
    QObject::connect(action, SIGNAL(triggered()), widget, slot);
}

}

void connectBlendModeActions(KisCompositeOpComboBox *widget, KisActionManager *manager)
{
    connectBlendModeAction(manager, "Next Blending Mode", widget, SLOT(slotNextBlendingMode()));
    connectBlendModeAction(manager, "Previous Blending Mode", widget, SLOT(slotPreviousBlendingMode()));
    connectBlendModeAction(manager, "Select Normal Blending Mode", widget, SLOT(slotNormal()));
    connectBlendModeAction(manager, "Select Dissolve Blending Mode", widget, SLOT(slotDissolve()));
    connectBlendModeAction(manager, "Select Behind Blending Mode", widget, SLOT(slotBehind()));
    connectBlendModeAction(manager, "Select Clear Blending Mode", widget, SLOT(slotClear()));
    connectBlendModeAction(manager, "Select Darken Blending Mode", widget, SLOT(slotDarken()));
    connectBlendModeAction(manager, "Select Multiply Blending Mode", widget, SLOT(slotMultiply()));
    connectBlendModeAction(manager, "Select Color Burn Blending Mode", widget, SLOT(slotColorBurn()));
    connectBlendModeAction(manager, "Select Linear Burn Blending Mode", widget, SLOT(slotLinearBurn()));
    connectBlendModeAction(manager, "Select Lighten Blending Mode", widget, SLOT(slotLighten()));
    connectBlendModeAction(manager, "Select Screen Blending Mode", widget, SLOT(slotScreen()));
    connectBlendModeAction(manager, "Select Color Dodge Blending Mode", widget, SLOT(slotColorDodge()));
    connectBlendModeAction(manager, "Select Linear Dodge Blending Mode", widget, SLOT(slotLinearDodge()));
    connectBlendModeAction(manager, "Select Overlay Blending Mode", widget, SLOT(slotOverlay()));
    connectBlendModeAction(manager, "Select Hard Overlay Blending Mode", widget, SLOT(slotHardOverlay()));
    connectBlendModeAction(manager, "Select Soft Light Blending Mode", widget, SLOT(slotSoftLight()));
    connectBlendModeAction(manager, "Select Hard Light Blending Mode", widget, SLOT(slotHardLight()));
    connectBlendModeAction(manager, "Select Vivid Light Blending Mode", widget, SLOT(slotVividLight()));
    connectBlendModeAction(manager, "Select Linear Light Blending Mode", widget, SLOT(slotLinearLight()));
    connectBlendModeAction(manager, "Select Pin Light Blending Mode", widget, SLOT(slotPinLight()));
    connectBlendModeAction(manager, "Select Hard Mix Blending Mode", widget, SLOT(slotHardMix()));
    connectBlendModeAction(manager, "Select Difference Blending Mode", widget, SLOT(slotDifference()));
    connectBlendModeAction(manager, "Select Exclusion Blending Mode", widget, SLOT(slotExclusion()));
    connectBlendModeAction(manager, "Select Hue Blending Mode", widget, SLOT(slotHue()));
    connectBlendModeAction(manager, "Select Saturation Blending Mode", widget, SLOT(slotSaturation()));
    connectBlendModeAction(manager, "Select Color Blending Mode", widget, SLOT(slotColor()));
    connectBlendModeAction(manager, "Select Luminosity Blending Mode", widget, SLOT(slotLuminosity()));
}

}

#include "KisCompositeOpListConnectionHelper.moc"
