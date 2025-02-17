// This file Copyright 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <chrono>
#include <memory>
#include <utility>

#include <event2/event.h>

#include "timer-ev.h"
#include "tr-assert.h"
#include "utils-ev.h"

using namespace std::literals;

namespace libtransmission
{

class EvTimer final : public Timer
{
public:
    explicit EvTimer(struct event_base* base)
        : base_{ base }
    {
    }

    EvTimer(EvTimer&&) = delete;
    EvTimer(EvTimer const&) = delete;
    EvTimer& operator=(EvTimer&&) = delete;
    EvTimer& operator=(EvTimer const&) = delete;

    ~EvTimer() override = default;

    void stop() override
    {
        if (!is_running_)
        {
            return;
        }

        event_del(evtimer_.get());

        is_running_ = false;
    }

    void start() override
    {
        if (is_running_)
        {
            return;
        }

        using namespace std::chrono;
        auto const secs = duration_cast<seconds>(interval_);
        auto tv = timeval{};
        tv.tv_sec = secs.count();
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>(duration_cast<microseconds>(interval_ - secs).count());
        evtimer_add(evtimer_.get(), &tv);

        is_running_ = true;
    }

    void setCallback(std::function<void()> callback) override
    {
        callback_ = std::move(callback);
    }

    [[nodiscard]] std::chrono::milliseconds interval() const noexcept override
    {
        return interval_;
    }

    void setInterval(std::chrono::milliseconds interval) override
    {
        TR_ASSERT_MSG(interval.count() > 0 || !isRepeating(), "repeating timers must have a positive interval");

        if (interval_ == interval)
        {
            return;
        }

        interval_ = interval;
        applyChanges();
    }

    [[nodiscard]] bool isRepeating() const noexcept override
    {
        return is_repeating_;
    }

    void setRepeating(bool repeating) override
    {
        if (is_repeating_ == repeating)
        {
            return;
        }

        is_repeating_ = repeating;
        applyChanges();
    }

private:
    [[nodiscard]] constexpr short events(bool is_repeating) noexcept
    {
        return static_cast<short>(EV_TIMEOUT | (is_repeating ? EV_PERSIST : 0));
    }

    void applyChanges()
    {
        auto const old_events = event_get_events(evtimer_.get());
        auto const new_events = events(isRepeating());
        auto const was_running = isRunning();

        if (was_running)
        {
            stop();
        }

        if (new_events != old_events)
        {
            [[maybe_unused]] auto const val = event_assign(evtimer_.get(), base_, -1, new_events, &EvTimer::onTimer, this);
            TR_ASSERT(val == 0);
        }

        if (was_running)
        {
            start();
        }
    }

    static void onTimer(evutil_socket_t /*unused*/, short /*unused*/, void* vself)
    {
        static_cast<EvTimer*>(vself)->handleTimer();
    }

    void handleTimer()
    {
        is_running_ = is_repeating_;

        TR_ASSERT(callback_);
        callback_();
    }

    [[nodiscard]] constexpr bool isRunning() const noexcept
    {
        return is_running_;
    }

    std::chrono::milliseconds interval_ = 100ms;
    bool is_repeating_ = false;
    bool is_running_ = false;
    std::function<void()> callback_;

    struct event_base* const base_;
    evhelpers::event_unique_ptr const evtimer_{ event_new(base_, -1, events(is_repeating_), &EvTimer::onTimer, this) };
};

std::unique_ptr<Timer> EvTimerMaker::create()
{
    return std::make_unique<EvTimer>(event_base_);
}

} // namespace libtransmission
