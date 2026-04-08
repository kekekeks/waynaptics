#include "synshared.h"
#include "glib.h"
#include <memory>


class Timer
{
public:
    OsTimerCallback callback = nullptr;
    void *arg = nullptr;
    guint _timerId = 0;
private:
    static gboolean g_timer_func(void* userData)
    {
        auto timer = static_cast<Timer *>(userData);
        timer->_timerId = 0;
        timer->callback(timer, GetTimeInMillis(), timer->arg);
        return G_SOURCE_REMOVE;
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
        _timerId = g_timeout_add(ms, g_timer_func, this);
    }

    ~Timer()
    {
        cancel();
    }

};

extern "C" CARD32 GetTimeInMillis(void) {
    return g_get_monotonic_time() / 1000;
}

extern "C" CARD64 GetTimeInMicros(void) {
    return g_get_monotonic_time();
}

extern "C" OsTimerPtr TimerSet(OsTimerPtr timerPtr,
                    int flags,
                    CARD32 millis,
                    OsTimerCallback func,
                    void *arg) {
    Timer *timer;
    if (timerPtr) {
        timer = static_cast<Timer *>(timerPtr);
    } else {
        timer = new Timer();
    }
    timer->callback = func;
    timer->arg = arg;
    if (func && millis > 0)
        timer->setTimeoutMs(millis);
    return timer;
}

extern "C" void TimerCancel(OsTimerPtr ptr) {
    static_cast<Timer *>(ptr)->cancel();
}

extern "C" void TimerFree(OsTimerPtr timer) {
    delete static_cast<Timer *>(timer);
}