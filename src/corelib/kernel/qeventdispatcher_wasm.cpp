// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_wasm_p.h"

#include <QtCore/private/qabstracteventdispatcher_p.h> // for qGlobalPostedEventsCount()
#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qstdweb_p.h>
#include <QtCore/private/qwasmsocket_p.h>

#include "emscripten.h"
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>

using namespace std::chrono;
using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

// using namespace emscripten;

Q_LOGGING_CATEGORY(lcEventDispatcher, "qt.eventdispatcher");
Q_LOGGING_CATEGORY(lcEventDispatcherTimers, "qt.eventdispatcher.timers");

#if QT_CONFIG(thread)
#define LOCK_GUARD(M) std::lock_guard<std::mutex> lock(M)
#else
#define LOCK_GUARD(M)
#endif

// Emscripten asyncify currently supports one level of suspend -
// recursion is not permitted. We track the suspend state here
// on order to fail (more) gracefully, but we can of course only
// track Qts own usage of asyncify.
static bool g_is_asyncify_suspended = false;

#if defined(QT_STATIC)

static bool useAsyncify()
{
    return qstdweb::haveAsyncify();
}

// clang-format off
EM_ASYNC_JS(void, qt_asyncify_suspend_js, (), {
    if (Module.qtSuspendId === undefined)
        Module.qtSuspendId = 0;
    ++Module.qtSuspendId;
    await new Promise(resolve => {
        Module.qtAsyncifyWakeUp = resolve;
    });
});

EM_JS(void, qt_asyncify_resume_js, (), {
    let wakeUp = Module.qtAsyncifyWakeUp;
    if (wakeUp == undefined)
        return;
    Module.qtAsyncifyWakeUp = undefined;
    const suspendId = Module.qtSuspendId;

    // Delayed wakeup with zero-timer. Workaround/fix for
    // https://github.com/emscripten-core/emscripten/issues/10515
    setTimeout(() => {
        // Another suspend occurred while the timeout was in queue.
        if (Module.qtSuspendId !== suspendId)
            return;
        wakeUp();
    });
});
// clang-format on

#else

// EM_JS is not supported for side modules; disable asyncify

static bool useAsyncify()
{
    return false;
}

void qt_asyncify_suspend_js()
{
    Q_UNREACHABLE();
}

void qt_asyncify_resume_js()
{
    Q_UNREACHABLE();
}

#endif // defined(QT_STATIC)

// Suspends the main thread until qt_asyncify_resume() is called. Returns
// false immediately if Qt has already suspended the main thread (recursive
// suspend is not supported by Emscripten). Returns true (after resuming),
// if the thread was suspended.
bool qt_asyncify_suspend()
{
    if (g_is_asyncify_suspended)
        return false;
    g_is_asyncify_suspended = true;
    qt_asyncify_suspend_js();
    return true;
}

// Wakes any currently suspended main thread. Returns true if the main
// thread was suspended, in which case it will now be asynchronously woken.
void qt_asyncify_resume()
{
    if (!g_is_asyncify_suspended)
        return;
    g_is_asyncify_suspended = false;
    qt_asyncify_resume_js();
}


Q_CONSTINIT QEventDispatcherWasm *QEventDispatcherWasm::g_mainThreadEventDispatcher = nullptr;
#if QT_CONFIG(thread)
Q_CONSTINIT QVector<QEventDispatcherWasm *> QEventDispatcherWasm::g_secondaryThreadEventDispatchers;
Q_CONSTINIT std::mutex QEventDispatcherWasm::g_staticDataMutex;
emscripten::ProxyingQueue QEventDispatcherWasm::g_proxyingQueue;
pthread_t QEventDispatcherWasm::g_mainThread;
#endif

QEventDispatcherWasm::QEventDispatcherWasm()
{
    // QEventDispatcherWasm operates in two main modes:
    // - On the main thread:
    //   The event dispatcher can process native events but can't
    //   block and wait for new events, unless asyncify is used.
    // - On a secondary thread:
    //   The event dispatcher can't process native events but can
    //   block and wait for new events.
    //
    // Which mode is determined by the calling thread: construct
    // the event dispatcher object on the thread where it will live.

    qCDebug(lcEventDispatcher) << "Creating QEventDispatcherWasm instance" << this
                               << "is main thread" << emscripten_is_main_runtime_thread();

    if (emscripten_is_main_runtime_thread()) {
        // There can be only one main thread event dispatcher at a time; in
        // addition the main instance is used by the secondary thread event
        // dispatchers so we set a global pointer to it.
        Q_ASSERT(g_mainThreadEventDispatcher == nullptr);
        g_mainThreadEventDispatcher = this;
#if QT_CONFIG(thread)
        g_mainThread = pthread_self();
#endif

        // Call the "onLoaded" JavaScript callback, unless startup tasks
        // have been registered which should complete first. Run async
        // to make sure event dispatcher construction (in particular any
        // subclass construction) has completed first.
        runAsync(callOnLoadedIfRequired);
    } else {
#if QT_CONFIG(thread)
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.append(this);
#endif
    }
}

QEventDispatcherWasm::~QEventDispatcherWasm()
{
    qCDebug(lcEventDispatcher) << "Destroying QEventDispatcherWasm instance" << this;

    delete m_timerInfo;

#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.remove(g_secondaryThreadEventDispatchers.indexOf(this));
    } else
#endif
    {
        if (m_timerId > 0)
            emscripten_clear_timeout(m_timerId);
        QWasmSocket::clearSocketNotifiers();
        g_mainThreadEventDispatcher = nullptr;
    }
}

bool QEventDispatcherWasm::isMainThreadEventDispatcher()
{
    return this == g_mainThreadEventDispatcher;
}

bool QEventDispatcherWasm::isSecondaryThreadEventDispatcher()
{
    return this != g_mainThreadEventDispatcher;
}

bool QEventDispatcherWasm::isValidEventDispatcherPointer(QEventDispatcherWasm *eventDispatcher)
{
    if (eventDispatcher == g_mainThreadEventDispatcher)
        return true;
#if QT_CONFIG(thread)
    if (g_secondaryThreadEventDispatchers.contains(eventDispatcher))
        return true;
#endif
    return false;
}

bool QEventDispatcherWasm::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    qCDebug(lcEventDispatcher) << "QEventDispatcherWasm::processEvents flags" << flags;

    emit awake();

    if (isMainThreadEventDispatcher()) {
        if (flags & QEventLoop::DialogExec)
            handleDialogExec();
        else if (flags & QEventLoop::ApplicationExec)
            handleApplicationExec();
    }

#if QT_CONFIG(thread)
    {
        // Reset wakeUp state: if wakeUp() was called at some point before
        // this then processPostedEvents() below will service that call.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wakeUpCalled = false;
    }
#endif

    processPostedEvents();

    // The processPostedEvents() call above may process an event which deletes the
    // application object and the event dispatcher; stop event processing in that case.
    if (!isValidEventDispatcherPointer(this))
        return false;

    if (m_interrupted) {
        m_interrupted = false;
        return false;
    }

    if (flags & QEventLoop::WaitForMoreEvents)
        wait();

    if (m_processTimers) {
        m_processTimers = false;
        processTimers();
    }

    return false;
}

void QEventDispatcherWasm::registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1 || interval < 0ns || !object) {
        qWarning("QEventDispatcherWasm::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::registerTimer: timers cannot be started from another "
                 "thread");
        return;
    }
#endif
    qCDebug(lcEventDispatcherTimers) << "registerTimer" << int(timerId) << interval << timerType << object;

    m_timerInfo->registerTimer(timerId, interval, timerType, object);
    updateNativeTimer();
}

bool QEventDispatcherWasm::unregisterTimer(Qt::TimerId timerId)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1) {
        qWarning("QEventDispatcherWasm::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::unregisterTimer: timers cannot be stopped from another "
                 "thread");
        return false;
    }
#endif

    qCDebug(lcEventDispatcherTimers) << "unregisterTimer" << int(timerId);

    bool ans = m_timerInfo->unregisterTimer(timerId);
    updateNativeTimer();
    return ans;
}

bool QEventDispatcherWasm::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWasm::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::unregisterTimers: timers cannot be stopped from another "
                 "thread");
        return false;
    }
#endif

    qCDebug(lcEventDispatcherTimers) << "registerTimer" << object;

    bool ans = m_timerInfo->unregisterTimers(object);
    updateNativeTimer();
    return ans;
}

QList<QAbstractEventDispatcher::TimerInfoV2>
QEventDispatcherWasm::timersForObject(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWasm:registeredTimers: invalid argument");
        return {};
    }
#endif

    return m_timerInfo->registeredTimers(object);
}

QEventDispatcherWasm::Duration QEventDispatcherWasm::remainingTime(Qt::TimerId timerId) const
{
    return m_timerInfo->remainingDuration(timerId);
}

void QEventDispatcherWasm::interrupt()
{
    m_interrupted = true;
    wakeUp();
}

void QEventDispatcherWasm::wakeUp()
{
    // The event dispatcher thread may be blocked or suspended by
    // wait(), or control may have been returned to the browser's
    // event loop. Make sure the thread is unblocked or make it
    // process events.
    bool wasBlocked = wakeEventDispatcherThread();

    if (!wasBlocked && isMainThreadEventDispatcher()) {
        {
            LOCK_GUARD(m_mutex);
            if (m_pendingProcessEvents)
                return;
            m_pendingProcessEvents = true;
        }
        runOnMainThreadAsync([this](){
            QEventDispatcherWasm::callProcessPostedEvents(this);
        });
    }
}

void QEventDispatcherWasm::handleApplicationExec()
{
    // Start the main loop, and then stop it on the first callback. This
    // is done for the "simulateInfiniteLoop" functionality where
    // emscripten_set_main_loop() throws a JS exception which returns
    // control to the browser while preserving the C++ stack.
    //
    // Note that we don't use asyncify here: Emscripten supports one level of
    // asyncify only and we want to reserve that for dialog exec() instead of
    // using it for the one qApp exec().
    // When JSPI is used, awaited async calls are allowed to be nested, so we
    // proceed normally.
    const bool simulateInfiniteLoop = true;
    emscripten_set_main_loop([](){
        emscripten_pause_main_loop();
    }, 0, simulateInfiniteLoop);
}

void QEventDispatcherWasm::registerSocketNotifier(QSocketNotifier *notifier)
{
    QWasmSocket::registerSocketNotifier(notifier);
}

void QEventDispatcherWasm::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    QWasmSocket::unregisterSocketNotifier(notifier);
}

void QEventDispatcherWasm::socketSelect(int timeout, int socket, bool waitForRead, bool waitForWrite,
                                        bool *selectForRead, bool *selectForWrite, bool *socketDisconnect)
{
    QEventDispatcherWasm *eventDispatcher = static_cast<QEventDispatcherWasm *>(
        QAbstractEventDispatcher::instance(QThread::currentThread()));

    if (!eventDispatcher) {
        qWarning("QEventDispatcherWasm::socketSelect called without eventdispatcher instance");
        return;
    }

    QWasmSocket::waitForSocketState(eventDispatcher, timeout, socket, waitForRead, waitForWrite,
                                    selectForRead, selectForWrite, socketDisconnect);
}


void QEventDispatcherWasm::handleDialogExec()
{
    if (!useAsyncify()) {
        qWarning() << "Warning: exec() is not supported on Qt for WebAssembly in this configuration. Please build"
                   << "with asyncify support, or use an asynchronous API like QDialog::open()";
        emscripten_sleep(1); // This call never returns
    }
    // For the asyncify case we do nothing here and wait for events in wait()
}

// Blocks/suspends the calling thread. This is possible in two cases:
// - Caller is a secondary thread: block on m_moreEvents
// - Caller is the main thread and asyncify is enabled: suspend using qt_asyncify_suspend()
// Returns false if the wait timed out.
bool QEventDispatcherWasm::wait(int timeout)
{
#if QT_CONFIG(thread)
    using namespace std::chrono_literals;
    Q_ASSERT(QThread::currentThread() == thread());

    if (isSecondaryThreadEventDispatcher()) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // If wakeUp() was called there might be pending events in the event
        // queue which should be processed. Don't block, instead return
        // so that the event loop can spin and call processEvents() again.
        if (m_wakeUpCalled)
            return true;

        auto wait_time = timeout > 0 ? timeout * 1ms : std::chrono::duration<int, std::micro>::max();
        bool wakeUpCalled = m_moreEvents.wait_for(lock, wait_time, [=] { return m_wakeUpCalled; });
        return wakeUpCalled;
    }
#endif
    Q_ASSERT(emscripten_is_main_runtime_thread());
    Q_ASSERT(isMainThreadEventDispatcher());
    if (useAsyncify()) {
        if (timeout > 0)
            qWarning() << "QEventDispatcherWasm asyncify wait with timeout is not supported; timeout will be ignored"; // FIXME

        bool didSuspend = qt_asyncify_suspend();
        if (!didSuspend) {
            qWarning("QEventDispatcherWasm: current thread is already suspended; could not asyncify wait for events");
            return false;
        }

        return true;
    } else {
        qWarning("QEventLoop::WaitForMoreEvents is not supported on the main thread without asyncify");
        Q_UNUSED(timeout);
    }
    return false;
}

// Wakes a blocked/suspended event dispatcher thread. Returns true if the
// thread is unblocked or was resumed, false if the thread state could not
// be determined.
bool QEventDispatcherWasm::wakeEventDispatcherThread()
{
#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wakeUpCalled = true;
        m_moreEvents.notify_one();
        return true;
    }
#endif
    Q_ASSERT(isMainThreadEventDispatcher());
    if (!g_is_asyncify_suspended)
        return false;
    runOnMainThread([]() { qt_asyncify_resume(); });
    return true;
}

// Process event activation callbacks for the main thread event dispatcher.
// Must be called on the main thread.
void QEventDispatcherWasm::callProcessPostedEvents(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // Bail out if Qt has been shut down.
    if (!g_mainThreadEventDispatcher)
        return;

    // In the unlikely event that we get a callProcessPostedEvents() call for
    // a previous main thread event dispatcher (i.e. the QApplication
    // object was deleted and created again): just ignore it and return.
    if (context != g_mainThreadEventDispatcher)
        return;

    {
        LOCK_GUARD(g_mainThreadEventDispatcher->m_mutex);
        g_mainThreadEventDispatcher->m_pendingProcessEvents = false;
    }

    g_mainThreadEventDispatcher->processPostedEvents();
}

bool QEventDispatcherWasm::processPostedEvents()
{
    QCoreApplication::sendPostedEvents();
    return false;
}

void QEventDispatcherWasm::processTimers()
{
    m_timerInfo->activateTimers();
    updateNativeTimer(); // schedule next native timer, if any
}

// Updates the native timer based on currently registered Qt timers.
// Must be called on the event dispatcher thread.
void QEventDispatcherWasm::updateNativeTimer()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread() == thread());
#endif

    // Multiplex Qt timers down to a single native timer, maintained
    // to have a timeout corresponding to the shortest Qt timer. This
    // is done in two steps: first determine the target wakeup time
    // on the event dispatcher thread (since this thread has exclusive
    // access to m_timerInfo), and then call native API to set the new
    // wakeup time on the main thread.

    const std::optional<std::chrono::nanoseconds> wait = m_timerInfo->timerWait();
    const auto toWaitDuration = duration_cast<milliseconds>(wait.value_or(0ms));
    const auto newTargetTimePoint = m_timerInfo->currentTime + toWaitDuration;
    auto epochNsecs = newTargetTimePoint.time_since_epoch();
    auto newTargetTime = std::chrono::duration_cast<std::chrono::milliseconds>(epochNsecs);
    auto maintainNativeTimer = [this, wait, toWaitDuration, newTargetTime]() {
        Q_ASSERT(emscripten_is_main_runtime_thread());

        if (!wait) {
            if (m_timerId > 0) {
                emscripten_clear_timeout(m_timerId);
                m_timerId = 0;
                m_timerTargetTime = 0ms;
            }
            return;
        }

        if (m_timerTargetTime != 0ms && newTargetTime >= m_timerTargetTime)
            return; // existing timer is good

        qCDebug(lcEventDispatcherTimers)
                << "Created new native timer with wait" << toWaitDuration.count() << "ms"
                << "timeout" << newTargetTime.count() << "ms";
        emscripten_clear_timeout(m_timerId);
        m_timerId = emscripten_set_timeout(&QEventDispatcherWasm::callProcessTimers,
                                           toWaitDuration.count(), this);
        m_timerTargetTime = newTargetTime;
    };

    // Update the native timer for this thread/dispatcher. This must be
    // done on the main thread where we have access to native API.
    runOnMainThread([this, maintainNativeTimer]() {
        Q_ASSERT(emscripten_is_main_runtime_thread());

        // "this" may have been deleted, or may be about to be deleted.
        // Check if the pointer we have is still a valid event dispatcher,
        // and keep the mutex locked while updating the native timer to
        // prevent it from being deleted.
        LOCK_GUARD(g_staticDataMutex);
            if (isValidEventDispatcherPointer(this))
                maintainNativeTimer();
    });
}

// Static timer activation callback. Must be called on the main thread
// and will then either process timers on the main thread or wake and
// process timers on a secondary thread.
void QEventDispatcherWasm::callProcessTimers(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // Note: "context" may be a stale pointer here,
    // take care before casting and dereferencing!

    // Process timers on this thread if this is the main event dispatcher
    if (reinterpret_cast<QEventDispatcherWasm *>(context) == g_mainThreadEventDispatcher) {
        g_mainThreadEventDispatcher->m_timerTargetTime = 0ms;
        g_mainThreadEventDispatcher->processTimers();
        return;
    }

    // Wake and process timers on the secondary thread if this a secondary thread dispatcher
#if QT_CONFIG(thread)
    std::lock_guard<std::mutex> lock(g_staticDataMutex);
    if (g_secondaryThreadEventDispatchers.contains(context)) {
        QEventDispatcherWasm *eventDispatcher = reinterpret_cast<QEventDispatcherWasm *>(context);
        eventDispatcher->m_timerTargetTime = 0ms;
        eventDispatcher->m_processTimers = true;
        eventDispatcher->wakeUp();
    }
#endif
}

namespace {
    int g_startupTasks = 0;
}

// The following functions manages sending the "qtLoaded" event/callback
// from qtloader.js on startup, once Qt initialization has been completed
// and the application is ready to display the first frame. This can be
// either as soon as the event loop is running, or later, if additional
// startup tasks (e.g. local font loading) have been registered.

void QEventDispatcherWasm::registerStartupTask()
{
    ++g_startupTasks;
}

void QEventDispatcherWasm::completeStarupTask()
{
    --g_startupTasks;
    callOnLoadedIfRequired();
}

void QEventDispatcherWasm::callOnLoadedIfRequired()
{
    if (g_startupTasks > 0)
        return;

    static bool qtLoadedCalled = false;
    if (qtLoadedCalled)
        return;
    qtLoadedCalled = true;
}

void QEventDispatcherWasm::onLoaded()
{
    // TODO: call qtloader.js onLoaded from here, in order to delay
    // hiding the "Loading..." message until the app is ready to paint
    // the first frame. Currently onLoaded must be called early before
    // main() in order to ensure that the screen/container elements
    // have valid geometry at startup.
}

namespace {
    void trampoline(void *context) {

        auto async_fn = [](void *context){
            std::function<void(void)> *fn = reinterpret_cast<std::function<void(void)> *>(context);
            (*fn)();
            delete fn;
        };

        emscripten_async_call(async_fn, context, 0);
    }
}

// Runs a function right away
void QEventDispatcherWasm::run(std::function<void(void)> fn)
{
    fn();
}

void QEventDispatcherWasm::runOnMainThread(std::function<void(void)> fn)
{
#if QT_CONFIG(thread)
    qstdweb::runTaskOnMainThread<void>(fn, &g_proxyingQueue);
#else
    qstdweb::runTaskOnMainThread<void>(fn);
#endif
}

// Runs a function asynchronously. Main thread only.
void QEventDispatcherWasm::runAsync(std::function<void(void)> fn)
{
    trampoline(new std::function<void(void)>(fn));
}

// Runs a function on the main thread. The function always runs asynchronously,
// also if the calling thread is the main thread.
void QEventDispatcherWasm::runOnMainThreadAsync(std::function<void(void)> fn)
{
    void *context = new std::function<void(void)>(fn);
#if QT_CONFIG(thread)
    if (!emscripten_is_main_runtime_thread()) {
        g_proxyingQueue.proxyAsync(g_mainThread, [context]{
            trampoline(context);
        });
        return;
    }
#endif
    trampoline(context);
}

QT_END_NAMESPACE

#include "moc_qeventdispatcher_wasm_p.cpp"
