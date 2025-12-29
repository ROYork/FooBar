#include <gtest/gtest.h>

#include <fb/stop_watch.h>

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace
{

constexpr auto SLEEP_PADDING = 2ms;

} // namespace

TEST(StopWatchTest, StartsImmediatelyByDefault)
{
  fb::stop_watch watch;

  std::this_thread::sleep_for(SLEEP_PADDING);

  EXPECT_TRUE(watch.is_running());
  EXPECT_GE(
      watch.elapsed_time(),
      std::chrono::duration_cast<fb::stop_watch::duration>(SLEEP_PADDING));
}

TEST(StopWatchTest, CanConstructWithoutStarting)
{
  fb::stop_watch watch{false};

  EXPECT_FALSE(watch.is_running());
  EXPECT_EQ(watch.elapsed_time(), fb::stop_watch::duration::zero());
}

TEST(StopWatchTest, StartAndStopAccumulatesTime)
{
  fb::stop_watch watch{false};

  watch.start();
  std::this_thread::sleep_for(SLEEP_PADDING);
  watch.stop();

  const auto elapsed_after_first_stop = watch.elapsed_time();
  EXPECT_FALSE(watch.is_running());
  EXPECT_GE(
      elapsed_after_first_stop,
      std::chrono::duration_cast<fb::stop_watch::duration>(SLEEP_PADDING));

  std::this_thread::sleep_for(SLEEP_PADDING);
  watch.start();
  std::this_thread::sleep_for(SLEEP_PADDING);
  watch.stop();

  const auto elapsed_after_second_stop = watch.elapsed_time();
  EXPECT_GT(elapsed_after_second_stop, elapsed_after_first_stop);
}

TEST(StopWatchTest, ElapsedTimeUpdatesWhileRunning)
{
  fb::stop_watch watch;

  const auto first_measurement = watch.elapsed_time();
  std::this_thread::sleep_for(SLEEP_PADDING);
  const auto second_measurement = watch.elapsed_time();

  EXPECT_LT(first_measurement, second_measurement);
}

TEST(StopWatchTest, ResetCanOptionallyRestart)
{
  fb::stop_watch watch;

  std::this_thread::sleep_for(SLEEP_PADDING);
  watch.stop();
  EXPECT_FALSE(watch.is_running());
  EXPECT_GT(
      watch.elapsed_time(),
      std::chrono::duration_cast<fb::stop_watch::duration>(SLEEP_PADDING));

  watch.reset(false);
  EXPECT_FALSE(watch.is_running());
  EXPECT_EQ(watch.elapsed_time(), fb::stop_watch::duration::zero());

  watch.reset();
  EXPECT_TRUE(watch.is_running());
  EXPECT_LT(
      watch.elapsed_time(),
      std::chrono::duration_cast<fb::stop_watch::duration>(SLEEP_PADDING));
}
