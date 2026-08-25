/*
 *  SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_manager_p.h"

#include <QMap>
#include <QApplication>
#include <QScopedPointer>
#include <QTimer>
#include <QtGlobal>

#include <boost/preprocessor/repeat_from_to.hpp>

#include "kis_input_manager.h"
#include "application/kis_config.h"
#include "kis_abstract_input_action.h"
#include <KisInputAction.h>
#include "kis_tool_invocation_action.h"
#include "kis_stroke_shortcut.h"
#include "kis_touch_shortcut.h"
#include "kis_native_gesture_shortcut.h"
#include "kis_input_profile_manager.h"
#include "kis_extended_modifiers_mapper.h"

#include "resources/kis_popup_palette.h"
#include "config-qt-patches-present.h"

#include <memory>

namespace {

class KisUiInputActionAdapter final : public KisInputAction
{
public:
    explicit KisUiInputActionAdapter(KisAbstractInputAction *action)
        : m_action(action)
    {
    }

    void activate(int shortcut) override
    {
        m_action->activate(shortcut);
    }

    void deactivate(int shortcut) override
    {
        m_action->deactivate(shortcut);
    }

    void begin(int shortcut, QEvent *event) override
    {
        m_action->begin(shortcut, event);
    }

    void end(QEvent *event) override
    {
        m_action->end(event);
    }

    void inputEvent(QEvent *event) override
    {
        m_action->inputEvent(event);
    }

    bool supportsHiResInputEvents(int shortcut) const override
    {
        return m_action->supportsHiResInputEvents(shortcut);
    }

    KisInputActionGroup inputActionGroup(int shortcut) const override
    {
        return m_action->inputActionGroup(shortcut);
    }

    int priority() const override
    {
        return m_action->priority();
    }

    bool canIgnoreModifiers() const override
    {
        return m_action->canIgnoreModifiers();
    }

    bool isAvailable() const override
    {
        return m_action->isAvailable();
    }

private:
    KisAbstractInputAction *m_action;
};

constexpr bool supportsSyntheticMouseSuppression()
{
#ifdef Q_OS_MAC
    return false;
#else
    return true;
#endif
}

KisInputEventSuppressor::Button normalizedButton(Qt::MouseButton button)
{
    if (button == Qt::LeftButton) {
        return KisInputEventSuppressor::Button::Left;
    }
    if (button == Qt::NoButton) {
        return KisInputEventSuppressor::Button::None;
    }
    return KisInputEventSuppressor::Button::Other;
}

KisInputEventSuppressor::Event normalizedSuppressionEvent(QEvent *event)
{
    using EventType = KisInputEventSuppressor::EventType;

    KisInputEventSuppressor::Event result;
    switch (event->type()) {
    case QEvent::MouseMove:
        result.type = EventType::MouseMove;
        break;
    case QEvent::MouseButtonPress:
        result.type = EventType::MousePress;
        break;
    case QEvent::MouseButtonRelease:
        result.type = EventType::MouseRelease;
        break;
    case QEvent::MouseButtonDblClick:
        result.type = EventType::MouseDoubleClick;
        break;
    case QEvent::TabletPress:
        result.type = EventType::TabletPress;
        break;
    case QEvent::TabletRelease:
        result.type = EventType::TabletRelease;
        break;
    case QEvent::TouchBegin:
        result.type = EventType::TouchBegin;
        break;
    default:
        return result;
    }

    if (result.type == EventType::MouseMove ||
        result.type == EventType::MousePress ||
        result.type == EventType::MouseRelease ||
        result.type == EventType::MouseDoubleClick) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        result.button = normalizedButton(mouseEvent->button());
        result.synthesized = mouseEvent->source() != Qt::MouseEventNotSynthesized;
    } else if (result.type == EventType::TabletPress ||
               result.type == EventType::TabletRelease) {
        result.button = normalizedButton(static_cast<QTabletEvent *>(event)->button());
    }

    return result;
}

void debugSuppressedInputEvent(
    QEvent *event,
    KisInputEventSuppressor::SuppressionReason reason)
{
    if (!KisTabletDebugger::instance()->debugEnabled()) {
        return;
    }

    int diagnostic = 0;
    switch (reason) {
    case KisInputEventSuppressor::SuppressionReason::DelayedMousePress:
        diagnostic = 1;
        break;
    case KisInputEventSuppressor::SuppressionReason::MouseEvent:
        diagnostic = 2;
        break;
    case KisInputEventSuppressor::SuppressionReason::SecondaryTabletButton:
    case KisInputEventSuppressor::SuppressionReason::TouchBegin:
        diagnostic = 3;
        break;
    case KisInputEventSuppressor::SuppressionReason::None:
        return;
    }

    const QString prefix = QStringLiteral("[BLOCKED %1:]").arg(diagnostic);
    switch (reason) {
    case KisInputEventSuppressor::SuppressionReason::DelayedMousePress:
    case KisInputEventSuppressor::SuppressionReason::MouseEvent:
        dbgTablet << KisTabletDebugger::instance()->eventToString(
            *static_cast<QMouseEvent *>(event), prefix);
        break;
    case KisInputEventSuppressor::SuppressionReason::SecondaryTabletButton:
        dbgTablet << KisTabletDebugger::instance()->eventToString(
            *static_cast<QTabletEvent *>(event), prefix);
        break;
    case KisInputEventSuppressor::SuppressionReason::TouchBegin:
        dbgTablet << KisTabletDebugger::instance()->eventToString(
            *static_cast<QTouchEvent *>(event), prefix);
        break;
    case KisInputEventSuppressor::SuppressionReason::None:
        break;
    }
}

}

bool KisInputManager::Private::ignoringQtCursorEvents()
{
    return eventSuppressor.isBlockingMouseEvents();
}

bool KisInputManager::Private::filterSuppressedEvent(QEvent *event)
{
    const KisInputEventSuppressor::SuppressionReason reason =
        eventSuppressor.filter(normalizedSuppressionEvent(event));
    if (reason == KisInputEventSuppressor::SuppressionReason::None) {
        return false;
    }

    debugSuppressedInputEvent(event, reason);
    if (reason == KisInputEventSuppressor::SuppressionReason::TouchBegin) {
        // Ignoring TouchBegin stops Qt from delivering the rest of this touch sequence.
        event->ignore();
    }
    return true;
}

void KisInputManager::Private::setMaskSyntheticEvents(bool value)
{
    eventSuppressor.setSuppressSyntheticMouseEvents(value);
}

KisInputManager::Private::Private(KisInputManager *qq)
    : q(qq)
    , moveEventCompressor(10 /* ms */,
                          KisSignalCompressor::FIRST_ACTIVE,
                          KisSignalCompressor::ADDITIVE_INTERVAL)
    , priorityEventFilterSeqNo(0)
    , popupWidget(nullptr)
    , touchHoldTimer(new QTimer(qq))
    , canvasSwitcher(this, qq)
    , eventSuppressor(KisConfig(true).useRightMiddleTabletButtonWorkaround(),
                      supportsSyntheticMouseSuppression())
{
    KisConfig cfg(true);

    moveEventCompressor.setDelay(cfg.tabletEventsDelay());
    testingAcceptCompressedTabletEvents = cfg.testingAcceptCompressedTabletEvents();
    testingCompressBrushEvents = cfg.testingCompressBrushEvents();

    if (cfg.trackTabletEventLatency()) {
        tabletLatencyTracker = new TabletLatencyTracker();
    }

    matcher.setInputActionGroupsMaskCallback(
        [this] () {
            return this->canvas ? this->canvas->inputActionGroupsMaskInterface()->inputActionGroupsMask() : AllActionGroup;
        });

    /**
     * On Windows and Linux we have a proper fix for this bug
     * patched into our local version of Qt. We don't have a fix
     * for macOS
     */
#ifdef Q_OS_MACOS
    useUnbalancedKeyPressEventWorkaround = true;
#endif

    /**
     * In Linux distributions Qt is not patched, so we should
     * use workaround for them
     */
#if defined Q_OS_LINUX &&  !KRITA_QT_HAS_UNBALANCED_KEY_PRESS_RELEASE_PATCH
    useUnbalancedKeyPressEventWorkaround = true;
#endif

    if (qEnvironmentVariableIsSet("KRITA_FIX_UNBALANCED_KEY_EVENTS")) {
        useUnbalancedKeyPressEventWorkaround = qEnvironmentVariableIntValue("KRITA_FIX_UNBALANCED_KEY_EVENTS");
    }

    touchHoldTimer->setTimerType(Qt::CoarseTimer);
    touchHoldTimer->setSingleShot(true);
    touchHoldTimer->setInterval(TOUCH_HOLD_DELAY_MS);
    connect(touchHoldTimer, &QTimer::timeout, qq, &KisInputManager::slotTouchHoldTriggered);
}

static const int InputWidgetsThreshold = 2000;
static const int OtherWidgetsThreshold = 400;

KisInputManager::Private::CanvasSwitcher::CanvasSwitcher(Private *_d, QObject *p)
    : QObject(p),
      d(_d),
      eatOneMouseStroke(false),
      focusSwitchThreshold(InputWidgetsThreshold)
{
}

void KisInputManager::Private::CanvasSwitcher::setupFocusThreshold(QObject* object)
{
    QWidget *widget = qobject_cast<QWidget*>(object);
    KIS_SAFE_ASSERT_RECOVER_RETURN(widget);

    thresholdConnections.clear();
    thresholdConnections.addConnection(&focusSwitchThreshold, SIGNAL(timeout()), widget, SLOT(setFocus()));
}

void KisInputManager::Private::CanvasSwitcher::addCanvas(KisCanvas2 *canvas)
{
    if (!canvas) return;

    QObject *canvasWidget = canvas->canvasWidget();

    if (!canvasResolver.contains(canvasWidget)) {
        canvasResolver.insert(canvasWidget, canvas);
    } else {
        // just a sanity cheek to find out if we are
        // trying to add two canvases concurrently.
        KIS_SAFE_ASSERT_RECOVER_NOOP(d->canvas == canvas);
    }

    if (canvas != d->canvas) {
        d->q->setupAsEventFilter(canvasWidget);
        canvasWidget->installEventFilter(this);

        setupFocusThreshold(canvasWidget);
        focusSwitchThreshold.setEnabled(false);

        d->canvas = canvas;
        d->toolProxy = qobject_cast<KisToolProxy*>(canvas->toolProxy());
    }
}

void KisInputManager::Private::CanvasSwitcher::removeCanvas(KisCanvas2 *canvas)
{
    QObject *widget = canvas->canvasWidget();

    canvasResolver.remove(widget);

    if (d->eventsReceiver == widget) {
        d->q->setupAsEventFilter(0);
    }

    widget->removeEventFilter(this);

    if (d->canvas == canvas) {
        d->canvas = 0;
        d->toolProxy = 0;
    }
}

bool isInputWidget(QWidget *w)
{
    if (!w) return false;


    QList<QLatin1String> types;
    types << QLatin1String("QAbstractSlider");
    types << QLatin1String("QAbstractSpinBox");
    types << QLatin1String("QLineEdit");
    types << QLatin1String("QTextEdit");
    types << QLatin1String("QPlainTextEdit");
    types << QLatin1String("QComboBox");
    types << QLatin1String("QKeySequenceEdit");

    Q_FOREACH (const QLatin1String &type, types) {
        if (w->inherits(type.data())) {
            return true;
        }
    }

    return false;
}

bool KisInputManager::Private::CanvasSwitcher::eventFilter(QObject* object, QEvent* event )
{
    if (canvasResolver.contains(object)) {
        switch (event->type()) {
        case QEvent::FocusIn: {
            QFocusEvent *fevent = static_cast<QFocusEvent*>(event);
            KisCanvas2 *canvas = canvasResolver.value(object);

            // only relevant canvases from the same main window should be
            // registered in the switcher
            KIS_SAFE_ASSERT_RECOVER_BREAK(canvas);

            if (canvas != d->canvas) {
                eatOneMouseStroke = 2 * (fevent->reason() == Qt::MouseFocusReason);
            }

            d->canvas = canvas;
            d->toolProxy = qobject_cast<KisToolProxy*>(canvas->toolProxy());

            d->q->setupAsEventFilter(object);

            object->removeEventFilter(this);
            object->installEventFilter(this);

            setupFocusThreshold(object);
            focusSwitchThreshold.setEnabled(false);

            const QPoint globalPos = QCursor::pos();
            const QPoint localPos = d->canvas->canvasWidget()->mapFromGlobal(globalPos);
            QWidget *canvasWindow = d->canvas->canvasWidget()->window();
            const QPoint windowsPos = canvasWindow ? canvasWindow->mapFromGlobal(globalPos) : localPos;

            QEnterEvent event(localPos, windowsPos, globalPos);
            d->q->eventFilter(object, &event);
            break;
        }
        case QEvent::FocusOut: {
            focusSwitchThreshold.setEnabled(true);
            break;
        }
        case QEvent::Enter: {
            break;
        }
        case QEvent::Leave: {
            focusSwitchThreshold.stop();
            break;
        }
        case QEvent::Wheel: {
            QWidget *widget = static_cast<QWidget*>(object);
            widget->setFocus();
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
            focusSwitchThreshold.forceDone();

            if (eatOneMouseStroke) {
                eatOneMouseStroke--;
                return true;
            }
            break;
        case QEvent::MouseButtonDblClick:
            focusSwitchThreshold.forceDone();
            if (eatOneMouseStroke) {
                return true;
            }
            break;
        case QEvent::MouseMove:
        case QEvent::TabletMove: {
            QWidget *widget = static_cast<QWidget*>(object);

            if (!widget->hasFocus()) {
                const int delay =
                    isInputWidget(QApplication::focusWidget()) ?
                    InputWidgetsThreshold : OtherWidgetsThreshold;

                focusSwitchThreshold.setDelayThreshold(delay);
                focusSwitchThreshold.start();
            }
        }
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(object, event);
}

KisInputManager::Private::ProximityNotifier::ProximityNotifier(KisInputManager::Private *_d, QObject *p)
    : QObject(p), d(_d)
{}

bool KisInputManager::Private::ProximityNotifier::eventFilter(QObject* object, QEvent* event )
{
    switch (event->type()) {
    case QEvent::TabletEnterProximity:
        d->debugEvent<QEvent, false>(event);
        // Tablet proximity events are unreliable AND fake mouse events do not
        // necessarily come after tablet events, so this is insufficient.
        // d->eventSuppressor.suppressNextLeftMousePress();

        // Qt sends fake mouse events instead of hover events, so not very useful.
        // Don't block mouse events on tablet since tablet move events are not generated until
        // after tablet press.
#ifndef Q_OS_MACOS
        d->blockMouseEvents();
#endif
        break;
    case QEvent::TabletLeaveProximity:
        d->debugEvent<QEvent, false>(event);
        d->allowMouseEvents();
        break;
#ifdef Q_OS_WIN
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        if (d->ignoreHighFunctionKeys) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();

            if (key >= Qt::Key_F13 && key <= Qt::Key_F35) {
                if (KisTabletDebugger::instance()->debugEnabled()) {
                    const QString pre = "[BLOCKED HIGH F-KEY]";
                    dbgTablet << KisTabletDebugger::instance()->eventToString(*keyEvent, pre);
                }
                return true;
            }
            break;
        }
#endif /* Q_OS_WIN */
    default:
        break;
    }
    return QObject::eventFilter(object, event);
}

#define EXTRA_BUTTON(z, n, _) \
    if(buttons & Qt::ExtraButton##n) { \
        buttonSet << Qt::ExtraButton##n; \
    }

KisInputAction *KisInputManager::Private::inputAction(
    KisAbstractInputAction *action)
{
    auto adapter = inputActionAdapters.find(action);
    if (adapter == inputActionAdapters.end()) {
        adapter = inputActionAdapters.insert(
            action,
            QSharedPointer<KisInputAction>(
                new KisUiInputActionAdapter(action)));
    }
    return adapter.value().data();
}

void KisInputManager::Private::installShortcut(const KisShortcutConfiguration &shortcut)
{
    KisAbstractInputAction *action =
        KisInputProfileManager::instance()->action(shortcut.actionId());
    if (!action) {
        qWarning() << "Input action is unavailable for shortcut:" << shortcut.actionId();
        return;
    }

    dbgUI << "Adding shortcut" << shortcut.keys() << "for action" << action->name();
    switch (shortcut.type()) {
    case KisShortcutConfiguration::KeyCombinationType:
        addKeyShortcut(action, shortcut.mode(), shortcut.keys());
        break;
    case KisShortcutConfiguration::MouseButtonType:
        addStrokeShortcut(action, shortcut.mode(), shortcut.keys(), shortcut.buttons());
        break;
    case KisShortcutConfiguration::MouseWheelType:
        addWheelShortcut(action, shortcut.mode(), shortcut.keys(), shortcut.wheel());
        break;
    case KisShortcutConfiguration::GestureType:
        if (!addNativeGestureShortcut(action, shortcut.mode(), shortcut.gesture())) {
            addTouchShortcut(action, shortcut.mode(), shortcut.gesture());
        }
        break;
    default:
        break;
    }
}

void KisInputManager::Private::addStrokeShortcut(KisAbstractInputAction* action, int index,
                                                 const QList<Qt::Key> &modifiers,
                                                 Qt::MouseButtons buttons)
{
    KisStrokeShortcut *strokeShortcut =
        new KisStrokeShortcut(inputAction(action), index);

    QSet<Qt::MouseButton> buttonSet;
    if(buttons & Qt::LeftButton) {
        buttonSet << Qt::LeftButton;
    }
    if(buttons & Qt::RightButton) {
        buttonSet << Qt::RightButton;
    }
    if(buttons & Qt::MiddleButton) {
        buttonSet << Qt::MiddleButton;
    }

BOOST_PP_REPEAT_FROM_TO(1, 25, EXTRA_BUTTON, _)

    if (!buttonSet.empty()) {
        strokeShortcut->setButtons(QSet<Qt::Key>(modifiers.cbegin(), modifiers.cend()), buttonSet);
        matcher.addShortcut(strokeShortcut);
    }
    else {
        delete strokeShortcut;
    }
}

void KisInputManager::Private::addKeyShortcut(KisAbstractInputAction* action, int index,
                                              const QList<Qt::Key> &keys)
{
    if (keys.size() == 0) return;

    KisSingleActionShortcut *keyShortcut =
        new KisSingleActionShortcut(inputAction(action), index);

    //Note: Ordering is important here, Shift + V is different from V + Shift,
    //which is the reason we use the last key here since most users will enter
    //shortcuts as "Shift + V". Ideally this should not happen, but this is
    //the way the shortcut matcher is currently implemented.
    QList<Qt::Key> allKeys = keys;
    Qt::Key key = allKeys.takeLast();
    QSet<Qt::Key> modifiers = QSet<Qt::Key>(allKeys.begin(), allKeys.end());
    keyShortcut->setKey(modifiers, key);
    matcher.addShortcut(keyShortcut);
}

void KisInputManager::Private::addWheelShortcut(KisAbstractInputAction* action, int index,
                                                const QList<Qt::Key> &modifiers,
                                                KisShortcutConfiguration::MouseWheelMovement wheelAction)
{
    std::unique_ptr<KisSingleActionShortcut> keyShortcut(
        new KisSingleActionShortcut(inputAction(action), index));

    KisSingleActionShortcut::WheelAction a;
    switch(wheelAction) {
    case KisShortcutConfiguration::WheelUp:
        a = KisSingleActionShortcut::WheelUp;
        break;
    case KisShortcutConfiguration::WheelDown:
        a = KisSingleActionShortcut::WheelDown;
        break;
    case KisShortcutConfiguration::WheelLeft:
        a = KisSingleActionShortcut::WheelLeft;
        break;
    case KisShortcutConfiguration::WheelRight:
        a = KisSingleActionShortcut::WheelRight;
        break;
    case KisShortcutConfiguration::WheelTrackpad:
        a = KisSingleActionShortcut::WheelTrackpad;
        break;
    default:
        return;
    }
    keyShortcut->setWheel(QSet<Qt::Key>(modifiers.begin(), modifiers.end()), a);
    matcher.addShortcut(keyShortcut.release());
}

void KisInputManager::Private::addTouchShortcut(KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture)
{
    KisTouchGestureType type = KisTouchGestureType::Unsupported;
    int touchPoints = 0;
    bool disabledWhenTouchPaintingActive = false;

    switch(gesture) {
#ifndef Q_OS_MACOS
    case KisShortcutConfiguration::OneFingerTap:
        type = KisTouchGestureType::Tap;
        touchPoints = 1;
        disabledWhenTouchPaintingActive = true;
        break;
    case KisShortcutConfiguration::OneFingerDrag:
        type = KisTouchGestureType::Drag;
        touchPoints = 1;
        disabledWhenTouchPaintingActive = true;
        break;
    case KisShortcutConfiguration::OneFingerHold:
        type = KisTouchGestureType::Hold;
        touchPoints = 1;
        break;
    case KisShortcutConfiguration::TwoFingerTap:
        type = KisTouchGestureType::Tap;
        touchPoints = 2;
        break;
    case KisShortcutConfiguration::TwoFingerDrag:
        type = KisTouchGestureType::Drag;
        touchPoints = 2;
        break;
    case KisShortcutConfiguration::ThreeFingerTap:
        type = KisTouchGestureType::Tap;
        touchPoints = 3;
        break;
    case KisShortcutConfiguration::ThreeFingerDrag:
        type = KisTouchGestureType::Drag;
        touchPoints = 3;
        break;
    case KisShortcutConfiguration::FourFingerTap:
        type = KisTouchGestureType::Tap;
        touchPoints = 4;
        break;
    case KisShortcutConfiguration::FourFingerDrag:
        type = KisTouchGestureType::Drag;
        touchPoints = 4;
        break;
    case KisShortcutConfiguration::FiveFingerTap:
        type = KisTouchGestureType::Tap;
        touchPoints = 5;
        break;
    case KisShortcutConfiguration::FiveFingerDrag:
        type = KisTouchGestureType::Drag;
        touchPoints = 5;
        break;
#endif
    default:
        break;
    }

    auto *shortcut = new KisTouchShortcut(inputAction(action), index, type);
    dbgKrita << "TouchAction:" << action->name();

    if (touchPoints > 0) {
        shortcut->setMinimumTouchPoints(touchPoints);
        shortcut->setMaximumTouchPoints(touchPoints);
    }

    if (disabledWhenTouchPaintingActive) {
        // Touch painting takes precedence over one-finger touch shortcuts, so
        // disable this type of shortcut when touch painting is active. Except
        // touch hold shortcuts, since touching and holding in one spot does
        // nothing otherwise and is therefore unambiguous.
        shortcut->setDisabledWhenTouchPaintingActive(true);
        shortcut->setTouchPaintingActiveCallback([]() {
            return !KisConfig(true).disableTouchOnCanvas();
        });
    }

    matcher.addShortcut(shortcut);
}

bool KisInputManager::Private::addNativeGestureShortcut(KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture)
{
    // Qt5 only implements QNativeGestureEvent for macOS
    Qt::NativeGestureType type;
    switch (gesture) {
#ifdef Q_OS_MACOS
        case KisShortcutConfiguration::PinchGesture:
            type = Qt::ZoomNativeGesture;
            break;
        case KisShortcutConfiguration::PanGesture:
            type = Qt::PanNativeGesture;
            break;
        case KisShortcutConfiguration::RotateGesture:
            type = Qt::RotateNativeGesture;
            break;
        case KisShortcutConfiguration::SmartZoomGesture:
            type = Qt::SmartZoomNativeGesture;
            break;
#endif
        default:
            return false;
    }

    KisNativeGestureShortcut *shortcut =
        new KisNativeGestureShortcut(inputAction(action), index, type);
    matcher.addShortcut(shortcut);
    return true;
}

void KisInputManager::Private::setupActions()
{
    QList<KisAbstractInputAction*> actions = KisInputProfileManager::instance()->actions();
    Q_FOREACH (KisAbstractInputAction *action, actions) {
        KisToolInvocationAction *toolAction =
            dynamic_cast<KisToolInvocationAction*>(action);

        if(toolAction) {
            defaultInputAction = toolAction;
        }
    }

    connect(KisInputProfileManager::instance(), SIGNAL(currentProfileChanged()), q, SLOT(profileChanged()));
    if(KisInputProfileManager::instance()->currentProfile()) {
        q->profileChanged();
    }
}

bool KisInputManager::Private::processUnhandledEvent(QEvent *event)
{
    bool retval = false;

    if (forwardAllEventsToTool ||
        event->type() == QEvent::KeyPress ||
        event->type() == QEvent::KeyRelease) {

        defaultInputAction->processUnhandledEvent(event);
        retval = true;
    }

    return retval && !forwardAllEventsToTool;
}

#ifdef HAVE_X11
inline QPointF dividePoints(const QPointF &pt1, const QPointF &pt2) {
    return QPointF(pt1.x() / pt2.x(), pt1.y() / pt2.y());
}

inline QPointF multiplyPoints(const QPointF &pt1, const QPointF &pt2) {
    return QPointF(pt1.x() * pt2.x(), pt1.y() * pt2.y());
}
#endif

void KisInputManager::Private::blockMouseEvents()
{
    if (!eventSuppressor.isBlockingMouseEvents() &&
        KisTabletDebugger::instance()->debugEnabled()) {
        dbgTablet << "Start blocking mouse events";
    }
    eventSuppressor.startBlockingMouseEvents();
}

void KisInputManager::Private::allowMouseEvents()
{
    /**
     * On Windows tablet events may arrive asynchronously to the
     * mouse events (in WinTab mode). The problem is that Qt
     * generates Enter/Leave and FocusIn/Out events via mouse
     * events only. It means that TabletPress may come much before
     * Enter and FocusIn event and start the stroke. In such a case
     * we shouldn't unblock mouse events.
     *
     * See https://bugs.kde.org/show_bug.cgi?id=417040
     *
     * PS:
     * Ideally, we should fix Qt to generate Enter/Leave and
     * FocusIn/Out events based on tablet events as well, but
     * it is a lot of work.
     */
#ifdef Q_OS_WIN32
    if (eventSuppressor.isBlockingMouseEvents() && matcher.hasRunningShortcut()) {
        return;
    }
#endif

    if (eventSuppressor.isBlockingMouseEvents() &&
        KisTabletDebugger::instance()->debugEnabled()) {
        dbgTablet << "Stop blocking mouse events";
    }
    eventSuppressor.stopBlockingMouseEvents();
}

void KisInputManager::Private::eatOneMousePress()
{
    eventSuppressor.suppressNextLeftMousePress();
}

void KisInputManager::Private::resetCompressor() {
    compressedMoveEvent.reset();
    moveEventCompressor.stop();
}

void KisInputManager::Private::startBlockingTouch()
{
    eventSuppressor.startBlockingTouchEvents();
}

void KisInputManager::Private::stopBlockingTouch()
{
    eventSuppressor.stopBlockingTouchEvents();
}

void KisInputManager::Private::restartTouchHoldTimer()
{
    touchHoldTimer->start();
}

void KisInputManager::Private::cancelTouchHoldTimer()
{
    touchHoldTimer->stop();
}

bool KisInputManager::Private::isPendingTouchHold() const
{
    return touchHoldTimer->isActive();
}

bool KisInputManager::Private::isWithinTouchHoldSlopRange(const QPointF &currentPos) const
{
    QPointF d = startingPos - currentPos;
    qreal distanceSquared = (d.x() * d.x()) + (d.y() * d.y());
    return distanceSquared <= KisShortcutMatcher::TOUCH_SLOP_SQUARED;
}

void KisInputManager::Private::bufferTouchEvent(QTouchEvent *touchEvent)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QScopedPointer<QEvent> dst;
    KoPointerEvent::copyQtPointerEvent(touchEvent, dst);
    bufferedTouchEvents.append(static_cast<QTouchEvent *>(dst.take()));
#else
    bufferedTouchEvents.append(touchEvent->clone());
#endif
}

void KisInputManager::Private::flushBufferedTouchEvents()
{
    for (QTouchEvent *touchEvent : bufferedTouchEvents) {
        switch (touchEvent->type()) {
        case QEvent::TouchBegin:
            q->handleTouchBegin(touchEvent);
            break;
        case QEvent::TouchUpdate:
            q->handleTouchUpdate(touchEvent);
            break;
        default:
            qWarning("Unhandled buffered touch event type %d", int(touchEvent->type()));
            break;
        }
        delete touchEvent;
    }
    bufferedTouchEvents.clear();
}

void KisInputManager::Private::clearBufferedTouchEvents()
{
    for (QTouchEvent *event : bufferedTouchEvents) {
        delete event;
    }
    bufferedTouchEvents.clear();
}

bool KisInputManager::Private::handleCompressedTabletEvent(QEvent *event)
{
    bool retval = false;

    if (event->type() == QTouchEvent::TouchUpdate && touchHasBlockedPressEvents) {
        matcher.touchUpdateEvent((QTouchEvent *)event);
    } else if (!matcher.pointerMoved(event) && toolProxy && event->type() != QTouchEvent::TouchUpdate) {
        toolProxy->forwardHoverEvent(event);
    }
    retval = true;
    event->setAccepted(true);

    return retval;
}

void KisInputManager::Private::fixShortcutMatcherModifiersState()
{
    KisExtendedModifiersMapper mapper;

    QVector<Qt::Key> newKeys;
    Qt::KeyboardModifiers modifiers = mapper.queryStandardModifiers();
    Q_FOREACH (Qt::Key key, mapper.queryExtendedModifiers()) {
        QKeyEvent kevent(QEvent::ShortcutOverride, key, modifiers);
        newKeys << KisExtendedModifiersMapper::workaroundShiftAltMetaHell(&kevent);
    }

    fixShortcutMatcherModifiersState(newKeys, modifiers);
}

void KisInputManager::Private::fixShortcutMatcherModifiersState(QVector<Qt::Key> newKeys, Qt::KeyboardModifiers modifiers)
{
    QVector<Qt::Key> danglingKeys = matcher.debugPressedKeys();

    matcher.handlePolledKeys(newKeys);

    for (auto it = danglingKeys.begin(); it != danglingKeys.end();) {
        if (newKeys.contains(*it)) {
            newKeys.removeOne(*it);
            it = danglingKeys.erase(it);
        } else {
            ++it;
        }
    }

    Q_FOREACH (Qt::Key key, danglingKeys) {
        QKeyEvent kevent(QEvent::KeyRelease, key, modifiers);
        processUnhandledEvent(&kevent);
    }

    Q_FOREACH (Qt::Key key, newKeys) {
        // just replay the whole sequence
        {
            QKeyEvent kevent(QEvent::ShortcutOverride, key, modifiers);
            processUnhandledEvent(&kevent);
        }
        {
            QKeyEvent kevent(QEvent::KeyPress, key, modifiers);
            processUnhandledEvent(&kevent);
        }
    }
}

qint64 KisInputManager::Private::TabletLatencyTracker::currentTimestamp() const
{
    // on OS X, we need to compute the timestamp that compares correctly against the native event timestamp,
    // which seems to be the msecs since system startup. On Linux with WinTab, we produce the timestamp that
    // we compare against ourselves in QWindowSystemInterface.

    QElapsedTimer elapsed;
    elapsed.start();
    return elapsed.msecsSinceReference();
}

void KisInputManager::Private::TabletLatencyTracker::print(const QString &message)
{
    dbgTablet << qUtf8Printable(message);
}
