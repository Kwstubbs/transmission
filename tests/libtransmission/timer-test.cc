// This file Copyright (C) 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include "transmission.h"

#include "timer-ev.h"
#include "utils-ev.h"

#include "test-fixtures.h"

using namespace std::literals;

namespace libtransmission::test
{

class TimerTest : public ::testing::Test
{
protected:
    // setup + teardown to manage an event_base

    void SetUp() override
    {
        ::testing::Test::SetUp();
        evbase_.reset(event_base_new());
    }

    void TearDown() override
    {
        evbase_.reset();
        ::testing::Test::TearDown();
    }

    static auto constexpr AsMSec = [](auto val)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(val);
    };

    void sleep_msec(std::chrono::milliseconds msec)
    {
        EXPECT_FALSE(waitFor(
            evbase_.get(),
            []() { return false; },
            msec));
    }

    static void EXPECT_TIME(
        std::chrono::milliseconds expected,
        std::chrono::milliseconds actual,
        std::chrono::milliseconds allowed_deviation)
    {
        auto const lower_bound = expected - allowed_deviation;
        EXPECT_LT(lower_bound, actual) << "lower_bound:" << lower_bound.count() << " actual:" << actual.count();

        auto const upper_bound = expected + allowed_deviation;
        EXPECT_LT(actual, upper_bound) << "actual:" << actual.count() << " upper_bound:" << upper_bound.count();
    }

    // This checks that `actual` is in the bounds of [expected/2 ... expected*1.5]
    // to confirm that the timer didn't kick too close to the previous or next interval.
    static void EXPECT_INTERVAL(std::chrono::milliseconds expected, std::chrono::milliseconds actual)
    {
        EXPECT_TIME(expected, actual, expected / 2);
    }

    [[nodiscard]] static auto current_time()
    {
        return std::chrono::steady_clock::now();
    }

    evhelpers::evbase_unique_ptr evbase_;
};

TEST_F(TimerTest, canInstantiate)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);
}

TEST_F(TimerTest, singleShotCallsCallback)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto called = false;
    auto callback = [&called]()
    {
        called = true;
    };
    timer->setCallback(callback);

    static auto constexpr Interval = 100ms;
    timer->startSingleShot(Interval);

    waitFor(evbase_.get(), [&called] { return called; });
    EXPECT_TRUE(called);
}

TEST_F(TimerTest, repeatingCallsCallback)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto called = false;
    auto callback = [&called]()
    {
        called = true;
    };
    timer->setCallback(callback);

    static auto constexpr Interval = 100ms;
    timer->startRepeating(Interval);

    waitFor(evbase_.get(), [&called] { return called; });
    EXPECT_TRUE(called);
}

TEST_F(TimerTest, singleShotHonorsInterval)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto called = false;
    auto callback = [&called]()
    {
        called = true;
    };
    timer->setCallback(callback);

    // run a single-shot timer
    auto const begin_time = current_time();
    static auto constexpr Interval = 100ms;
    timer->startSingleShot(Interval);
    EXPECT_FALSE(timer->isRepeating());
    EXPECT_EQ(Interval, timer->interval());
    waitFor(evbase_.get(), [&called] { return called; });
    auto const end_time = current_time();

    // confirm that it kicked at the right interval
    EXPECT_TRUE(called);
    EXPECT_INTERVAL(Interval, AsMSec(end_time - begin_time));
}

TEST_F(TimerTest, repeatingHonorsInterval)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    // start a repeating timer
    auto const begin_time = current_time();
    static auto constexpr Interval = 100ms;
    static auto constexpr DesiredLoops = 3;
    timer->startRepeating(Interval);
    EXPECT_TRUE(timer->isRepeating());
    EXPECT_EQ(Interval, timer->interval());
    waitFor(evbase_.get(), [&n_calls] { return n_calls >= DesiredLoops; });
    auto const end_time = current_time();

    // confirm that it kicked the right number of times
    EXPECT_INTERVAL(Interval * DesiredLoops, AsMSec(end_time - begin_time));
    EXPECT_EQ(DesiredLoops, n_calls);
}

TEST_F(TimerTest, restartWithDifferentInterval)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    auto const test = [this, &n_calls, &timer](auto interval)
    {
        auto const next = n_calls + 1;
        auto const begin_time = current_time();
        timer->startSingleShot(interval);
        waitFor(evbase_.get(), [&n_calls, next]() { return n_calls >= next; });
        auto const end_time = current_time();

        EXPECT_INTERVAL(interval, AsMSec(end_time - begin_time));
    };

    test(100ms);
    test(200ms);
    test(100ms);
}

TEST_F(TimerTest, restartWithSameInterval)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    auto const test = [this, &n_calls, &timer](auto interval)
    {
        auto const next = n_calls + 1;
        auto const begin_time = current_time();
        timer->startSingleShot(interval);
        waitFor(evbase_.get(), [&n_calls, next]() { return n_calls >= next; });
        auto const end_time = current_time();

        EXPECT_INTERVAL(interval, AsMSec(end_time - begin_time));
    };

    test(timer->interval());
    test(timer->interval());
    test(timer->interval());
}

TEST_F(TimerTest, repeatingThenSingleShot)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    // start a repeating timer and confirm that it's running
    auto begin_time = current_time();
    static auto constexpr RepeatingInterval = 100ms;
    static auto constexpr DesiredLoops = 2;
    timer->startRepeating(RepeatingInterval);
    EXPECT_EQ(RepeatingInterval, timer->interval());
    EXPECT_TRUE(timer->isRepeating());
    waitFor(evbase_.get(), [&n_calls]() { return n_calls >= DesiredLoops; });
    auto end_time = current_time();
    EXPECT_TIME(RepeatingInterval * DesiredLoops, AsMSec(end_time - begin_time), RepeatingInterval / 2);

    // now restart it as a single shot
    auto const baseline = n_calls;
    begin_time = current_time();
    static auto constexpr SingleShotInterval = 25ms;
    timer->startSingleShot(SingleShotInterval);
    EXPECT_EQ(SingleShotInterval, timer->interval());
    EXPECT_FALSE(timer->isRepeating());
    waitFor(evbase_.get(), [&n_calls]() { return n_calls >= DesiredLoops + 1; });
    end_time = current_time();

    // confirm that the single shot interval was honored
    EXPECT_INTERVAL(SingleShotInterval, AsMSec(end_time - begin_time));

    // confirm that the timer only kicks once, since it was converted into single-shot
    sleep_msec(SingleShotInterval * 3);
    EXPECT_EQ(baseline + 1, n_calls);
}

TEST_F(TimerTest, singleShotStop)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    // start a single-shot timer
    static auto constexpr Interval = 200ms;
    timer->startSingleShot(Interval);
    EXPECT_EQ(Interval, timer->interval());
    EXPECT_FALSE(timer->isRepeating());

    // wait half the interval, then stop the timer
    sleep_msec(Interval / 2);
    EXPECT_EQ(0U, n_calls);
    timer->stop();

    // wait until the timer has gone past.
    // since we stopped it, callback should not have been called.
    sleep_msec(Interval);
    EXPECT_EQ(0U, n_calls);
}

TEST_F(TimerTest, repeatingStop)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    // start a repeating timer
    static auto constexpr Interval = 200ms;
    timer->startRepeating(Interval);
    EXPECT_EQ(Interval, timer->interval());
    EXPECT_TRUE(timer->isRepeating());

    // wait half the interval, then stop the timer
    sleep_msec(Interval / 2);
    EXPECT_EQ(0U, n_calls);
    timer->stop();

    // wait until the timer has gone past.
    // since we stopped it, callback should not have been called.
    sleep_msec(Interval);
    EXPECT_EQ(0U, n_calls);
}

TEST_F(TimerTest, destroyedTimersStop)
{
    auto timer_maker = EvTimerMaker{ evbase_.get() };
    auto timer = timer_maker.create();
    EXPECT_TRUE(timer);

    auto n_calls = size_t{ 0U };
    auto callback = [&n_calls]()
    {
        ++n_calls;
    };
    timer->setCallback(callback);

    // start a repeating timer
    static auto constexpr Interval = 200ms;
    timer->startRepeating(Interval);
    EXPECT_EQ(Interval, timer->interval());
    EXPECT_TRUE(timer->isRepeating());

    // wait half the interval, then destroy the timer
    sleep_msec(Interval / 2);
    EXPECT_EQ(0U, n_calls);
    timer.reset();

    // wait until the timer has gone past.
    // since we destroyed it, callback should not have been called.
    sleep_msec(Interval);
    EXPECT_EQ(0U, n_calls);
}

} // namespace libtransmission::test
