#include "synshared.h"
#include "mainloop.h"
#include "glib.h"


void run_main_loop()
{
    g_main_context_acquire(g_main_context_default());
    auto loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
}


class Timer
{
public:
    OsTimerCallback callback = nullptr;
    void *arg = nullptr;
    guint _timerId = 0;
private:
    static void g_timer_func(void* userData)
    {
        auto timer = (Timer *) userData;
        timer->_timerId = 0;
        timer->callback(timer, GetTimeInMillis(), timer->arg);
    }
public:
    void cancel() {
        if(_timerId)
        {
            g_source_remove(_timerId);
            _timerId = 0;
        }
    }

    void setTimeoutMs(CARD32 ms) {
        cancel();
        _timerId = g_timeout_add_once(ms, g_timer_func, this);
    }

    ~Timer()
    {
        cancel();
    }

};

extern "C"
{
CARD32 GetTimeInMillis(void) {
    return g_get_monotonic_time() / 1000;
}
CARD64 GetTimeInMicros(void) {
    return g_get_monotonic_time();
}


extern _X_EXPORT OsTimerPtr TimerSet(OsTimerPtr timerPtr,
                                     int flags,
                                     CARD32 millis,
                                     OsTimerCallback func,
                                     void *arg) {
    auto timer = timerPtr ? (Timer *) timerPtr : new Timer();
    timer->callback = func;
    timer->arg = arg;
    if (func && millis > 0)
        timer->setTimeoutMs(millis);
    return timer;
}

extern _X_EXPORT void TimerCancel(OsTimerPtr ptr) {
    ((Timer *) ptr)->cancel();
}

extern _X_EXPORT void TimerFree(OsTimerPtr timer) {
    delete (Timer *) timer;
}

}