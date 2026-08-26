/*
 *  SPDX-FileCopyrightText: 2015 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */


#include <QList>
#include <QPointer>
#include <QEvent>
#include <QHash>
#include <QSharedPointer>
#include <QTouchEvent>
#include <QScopedPointer>
#include <QQueue>

#include "kis_input_manager.h"
#include <KisInputEventSuppressor.h>
#include "kis_shortcut_matcher.h"
#include "kis_shortcut_configuration.h"
#include <KoCanvasBase.h>
#include <KisToolCanvas.h>
#include "kis_tool_proxy.h"
#include "kis_signal_compressor.h"
#include "input/ui/kis_tablet_debugger.h"
#include <KisTimedSignalThreshold.h>
#include "kis_signal_auto_connection.h"
#include "kis_latency_tracker.h"

class KisToolInvocationAction;
class KisAbstractInputAction;
class KisInputAction;


class KisInputManager::Private
{
public:
    static constexpr int TOUCH_HOLD_DELAY_MS = 400;

    Private(KisInputManager *qq);
    void addStrokeShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, Qt::MouseButtons buttons);
    void addKeyShortcut(KisAbstractInputAction* action, int index,const QList<Qt::Key> &keys);
    void addTouchShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture );
    bool addNativeGestureShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture );
    void addWheelShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, KisShortcutConfiguration::MouseWheelMovement wheelAction);
    bool processUnhandledEvent(QEvent *event);
    void setupActions();
    KisInputAction *inputAction(KisAbstractInputAction *action);
    void installShortcut(const KisShortcutConfiguration &shortcut);
    bool handleCompressedTabletEvent(QEvent *event);
    void fixShortcutMatcherModifiersState();
    void fixShortcutMatcherModifiersState(QVector<Qt::Key> newKeys, Qt::KeyboardModifiers modifiers);

    KisInputManager *q;

    QPointer<KoCanvasBase> canvas;
    QPointer<KisToolProxy> toolProxy;

    bool forwardAllEventsToTool = false;
    bool ignoringQtCursorEvents();
    bool filterSuppressedEvent(QEvent *event);

    bool touchHasBlockedPressEvents = false;

    QHash<KisAbstractInputAction*, QSharedPointer<KisInputAction>> inputActionAdapters;
    KisShortcutMatcher matcher;

    KisToolInvocationAction *defaultInputAction = 0;

    QObject *eventsReceiver = 0;
    KisSignalCompressor moveEventCompressor;
    QScopedPointer<QEvent> compressedMoveEvent;
    bool testingAcceptCompressedTabletEvents = false;
    bool testingCompressBrushEvents = false;

    typedef QPair<int, QPointer<QObject> > PriorityPair;
    typedef QList<PriorityPair> PriorityList;
    PriorityList priorityEventFilter;
    int priorityEventFilterSeqNo;

    bool touchStrokeStarted = false;
    bool touchStrokeBlocked = false;
    bool popupWasActive = false;
    int lastPointCount = 0;

    QPointF startingPos;
    QPointF previousPos;
    QScopedPointer<QEvent> originatingTouchBeginEvent;

    bool useUnbalancedKeyPressEventWorkaround = false;
    bool shouldSynchronizeOnNextKeyPress = false;

    KisPopupWidgetInterface *popupWidget;

    QTimer *touchHoldTimer;
    QVector<QTouchEvent *> bufferedTouchEvents;

    void blockMouseEvents();
    void allowMouseEvents();
    void eatOneMousePress();
    void setMaskSyntheticEvents(bool value);
    void resetCompressor();
    void startBlockingTouch();
    void stopBlockingTouch();
    void restartTouchHoldTimer();
    void cancelTouchHoldTimer();
    bool isPendingTouchHold() const;
    bool isWithinTouchHoldSlopRange(const QPointF &currentPos) const;
    void bufferTouchEvent(QTouchEvent *event);
    void flushBufferedTouchEvents();
    void clearBufferedTouchEvents();

    template <class Event, bool useBlocking>
    void debugEvent(QEvent *event)
    {
      if (!KisTabletDebugger::instance()->debugEnabled()) return;

      QString msg1 = useBlocking && ignoringQtCursorEvents() ? "[BLOCKED] " : "[       ]";
      Event *specificEvent = static_cast<Event*>(event);
      dbgTablet << KisTabletDebugger::instance()->eventToString(*specificEvent, msg1);
    }

    class ProximityNotifier : public QObject
    {
    public:
        ProximityNotifier(Private *_d, QObject *p);
        bool eventFilter(QObject* object, QEvent* event ) override;
    private:
        KisInputManager::Private *d;
    };

    class CanvasSwitcher : public QObject
    {
    public:
        CanvasSwitcher(Private *_d, QObject *p);
        void addCanvas(KoCanvasBase *canvas);
        void removeCanvas(KoCanvasBase *canvas);
        bool eventFilter(QObject* object, QEvent* event ) override;

    private:
        void setupFocusThreshold(QObject *object);

    private:
        KisInputManager::Private *d;
        QMap<QObject*, QPointer<KoCanvasBase>> canvasResolver;
        int eatOneMouseStroke;
        KisTimedSignalThreshold focusSwitchThreshold;
        KisSignalAutoConnectionsStore thresholdConnections;
    };
    CanvasSwitcher canvasSwitcher;

    KisInputEventSuppressor eventSuppressor;

    bool containsPointer = false;

    int accumulatedScrollDelta = 0;

    class TabletLatencyTracker : public KisLatencyTracker {
    protected:
        virtual qint64 currentTimestamp() const override;
        virtual void print(const QString &message) override;
    };

    KisSharedPtr<TabletLatencyTracker> tabletLatencyTracker;
#ifdef Q_OS_WIN
    bool ignoreHighFunctionKeys = false;
#endif
};
