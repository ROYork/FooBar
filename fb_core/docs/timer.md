# Timer Class Documentation

## Overview

The `Timer` class provides event timer functionality with integration of 
the `fb_signal` signal/slot mechanism. 
It offers high-resolution timing, thread-safe operation, and flexible single-shot or repeating modes.

## Table of Contents

1. [Features](#features)
2. [Quick Start](#quick-start)
3. [API Reference](#api-reference)
4. [Usage Patterns](#usage-patterns)
5. [Thread Safety](#thread-safety)
6. [Timing Guarantees](#timing-guarantees)
7. [Best Practices](#best-practices)
8. [Examples](#examples)

---

## Features

- **Single-Shot and Repeating Modes**: Configure timer to fire once or repeatedly
- **Millisecond Precision**: High-resolution timing using `std::chrono::steady_clock`
- **Signal/Slot Integration**: Full fb_signal integration for event-driven programming
- **Thread-Safe**: All operations synchronized with mutex protection
- **Automatic Drift Correction**: Repeating timers automatically correct for drift
- **Query Remaining Time**: Check time remaining until next timeout
- **Move Semantics**: Efficient resource management with move support
- **Static Single-Shot**: Convenient static method for one-time delayed actions

---

## Quick Start

### Basic Repeating Timer

```cpp
#include <fb/timer.h>
#include <iostream>

using namespace fb;

int main()
{
  Timer timer;

  // Connect slot (lambda) to timeout signal
  timer.timeout.connect([]() {
      std::cout << "Timer fired!" << std::endl;
  });

  // Start timer with 1000ms interval
  timer.start(1000);

  // Timer fires every second...

  timer.stop();  // Stop when done
  return 0;
}
```

### Single-Shot Timer

```cpp
Timer timer;

timer.setSingleShot(true);
timer.timeout.connect([]() {
    std::cout << "One-time timeout!" << std::endl;
});

timer.start(5000);  // Fire once after 5 seconds
```

### Static Single-Shot Method

```cpp
auto timer = Timer::singleShot(3000, []() {
    std::cout << "3 seconds elapsed" << std::endl;
});
// Keep timer alive until it fires
```

---

## API Reference

### Construction and Destruction

#### Timer()
```cpp
Timer();
```
Constructs a timer in the stopped state with:
- Interval: 0ms
- Single-shot mode: false (repeating)
- Active: false

#### ~Timer()
```cpp
~Timer();
```
Destructor stops the timer if active and joins the timer thread.

**Thread Safety**: Safe to destroy from any thread.

---

### Timer Control

#### start()
```cpp
void start();
void start(int intervalMs);
```

Starts the timer.

**Parameters**:
- `intervalMs` (optional): Timeout interval in milliseconds (must be > 0)

**Behavior**:
- If timer is already active, restarts it
- With no parameters, uses current interval (must be set first)
- With parameter, sets interval and starts atomically

**Throws**:
- `std::runtime_error`: If interval is 0 (no-parameter version)
- `std::invalid_argument`: If intervalMs <= 0 (parameter version)

**Example**:
```cpp
timer.start(1000);  // Start with 1000ms interval

// Or:
timer.setInterval(1000);
timer.start();  // Start with previously set interval
```

---

#### stop()
```cpp
void stop();
```

Stops the timer immediately.

**Behavior**:
- If timer is inactive, this is a no-op
- Blocks until timer thread has fully stopped
- Pending timeout signals will not be emitted

**Thread Safety**: Can be called from any thread, including from within a timeout slot.

**Example**:
```cpp
timer.stop();
```

---

#### restart()
```cpp
void restart();
```

Restarts the timer, equivalent to `stop()` followed by `start()`.

**Throws**:
- `std::runtime_error`: If interval is 0

**Example**:
```cpp
timer.restart();  // Reset to beginning of interval
```

---

### Configuration

#### setInterval()
```cpp
void setInterval(int intervalMs);
```

Sets the timer interval.

**Parameters**:
- `intervalMs`: Timeout interval in milliseconds (must be > 0)

**Behavior**:
- If timer is active, automatically restarts with new interval

**Throws**:
- `std::invalid_argument`: If intervalMs <= 0

**Example**:
```cpp
timer.setInterval(2000);  // 2 second interval
```

---

#### interval()
```cpp
int interval() const;
```

Gets the current interval.

**Returns**: Current timeout interval in milliseconds

**Example**:
```cpp
int currentInterval = timer.interval();
```

---

#### setSingleShot()
```cpp
void setSingleShot(bool singleShot);
```

Sets single-shot mode.

**Parameters**:
- `singleShot`: true for single-shot, false for repeating

**Behavior**:
- In single-shot mode, timer fires once then stops automatically
- In repeating mode (default), timer fires repeatedly

**Example**:
```cpp
timer.setSingleShot(true);  // Fire only once
```

---

#### isSingleShot()
```cpp
bool isSingleShot() const;
```

Checks if timer is in single-shot mode.

**Returns**: true if single-shot, false if repeating

**Example**:
```cpp
if (timer.isSingleShot()) {
  std::cout << "Timer will fire once" << std::endl;
}
```

---

### Status Query

#### isActive()
```cpp
bool isActive() const;
```

Checks if timer is currently running.

**Returns**: true if active, false otherwise

**Example**:
```cpp
if (timer.isActive()) {
  std::cout << "Timer is running" << std::endl;
}
```

---

#### remainingTime()
```cpp
int remainingTime() const;
```

Gets the time remaining until next timeout.

**Returns**: Remaining time in milliseconds, or 0 if timer is inactive

**Note**: Value is a snapshot and may be slightly inaccurate due to scheduling.

**Example**:
```cpp
int remaining = timer.remainingTime();
std::cout << "Fires in " << remaining << "ms" << std::endl;
```

---

### Accuracy Control

#### setAccuracy()
```cpp
void setAccuracy(TimerAccuracy accuracy);
```

Sets timer accuracy mode (reserved for future use).

**Parameters**:
- `accuracy`: Desired accuracy mode (COARSE, PRECISE, VERY_PRECISE)

**Note**: Currently a no-op. All timers use PRECISE mode.

---

#### accuracy()
```cpp
TimerAccuracy accuracy() const;
```

Gets current accuracy mode.

**Returns**: Current accuracy mode (always PRECISE in current implementation)

---

### Static Methods

#### singleShot()
```cpp
static std::shared_ptr<Timer> singleShot(int intervalMs, std::function<void()> slot);
```

Creates a single-shot timer that fires once after a delay.

**Parameters**:
- `intervalMs`: Delay before firing
- `slot`: Function to call when timer expires

**Returns**: Shared pointer to the timer (keep alive until timeout)

**Example**:
```cpp
auto timer = Timer::singleShot(5000, []() {
  std::cout << "5 seconds elapsed" << std::endl;
});
// Timer fires after 5 seconds
```

---

### Signals

#### timeout
```cpp
fb::signal<> timeout;
```

fb::signal emitted when timer expires.

**Usage**:
```cpp
timer.timeout.connect([]() {
  std::cout << "Timeout occurred" << std::endl;
});
```

**Connection Types**:
- **DIRECT** (default): Slot runs on timer's thread
- **QUEUED**: Slot runs on receiver's thread (use for cross-thread)
- **BLOCKING_QUEUED**: Slot runs on receiver's thread, timer blocks

---

## Usage Patterns

### Pattern 1: Periodic Task Execution

```cpp
class DataCollector
{
public:
  DataCollector(int intervalMs)
  {
    timer_.timeout.connect([this]() {
        collectData();
    });
    timer_.start(intervalMs);
  }

  ~DataCollector()
  {
    timer_.stop();
  }

private:
  void collectData()
  {
    // Collect data periodically
  }

  Timer timer_;
};
```

---

### Pattern 2: Countdown Timer

```cpp
void countdown(int seconds)
{
  Timer timer;

  timer.timeout.connect([&seconds, &timer]() {
      std::cout << seconds << "..." << std::endl;
      seconds--;

      if (seconds < 0) {
          timer.stop();
          std::cout << "Done!" << std::endl;
      }
  });

  timer.start(1000);
}
```

---

### Pattern 3: Timeout Detection

```cpp
void operationWithTimeout(int timeoutMs)
{
  Timer watchdog;
  std::atomic<bool> timedOut{false};

  watchdog.setSingleShot(true);
  watchdog.timeout.connect([&timedOut]() {
      timedOut = true;
      std::cout << "Operation timed out!" << std::endl;
  });

  watchdog.start(timeoutMs);

  // Perform operation...
  doOperation();

  if (!timedOut) {
      watchdog.stop();  // Cancel timeout
  }
}
```

---

### Pattern 4: Delayed Action

```cpp
// Execute action after delay
Timer::singleShot(2000, []() {
  std::cout << "Delayed action executed" << std::endl;
});
```

---

### Pattern 5: Heartbeat Monitor

```cpp
class HeartbeatMonitor
{
public:
  
HeartbeatMonitor(int intervalMs, int timeoutMs)
{
  heartbeatTimer_.timeout.connect([this]() {
    sendHeartbeat();
  });

  watchdogTimer_.setSingleShot(true);
  watchdogTimer_.timeout.connect([this]() {
    onTimeout();
  });

  heartbeatTimer_.start(intervalMs);
  watchdogTimer_.start(timeoutMs);
}

void receiveAck()
{
  watchdogTimer_.restart();  // Reset watchdog
}

private:
  void sendHeartbeat() { /* Send heartbeat */ }
  void onTimeout() { /* Connection lost */ }

  Timer heartbeatTimer_;
  Timer watchdogTimer_;
};
```

---

## Thread Safety

### Thread-Safe Operations

All Timer methods are thread-safe and can be called from any thread:

- `start()` / `stop()` / `restart()`
- `setInterval()` / `interval()`
- `setSingleShot()` / `isSingleShot()`
- `isActive()` / `remainingTime()`

### Signal Emission Thread

The `timeout` signal is emitted on the **timer's internal thread**, not the thread that created the timer.

**For cross-thread communication**, use QUEUED connections:

```cpp
class Receiver : public Trackable
{
public:
  void onTimeout() {
      // Runs on receiver's thread
  }
};

auto receiver = std::make_shared<Receiver>();
timer.timeout.connect(receiver, &Receiver::onTimeout, ConnectionType::QUEUED);

// In receiver's thread event loop:
EventDispatcher::current()->processEvents();
```

### Stopping from Timeout Slot

Safe to stop timer from within its own timeout slot:

```cpp
timer.timeout.connect([&timer, &count]() {
  count++;
  if (count >= 5) {
      timer.stop();  // Safe
  }
});
```

---

## Timing Guarantees

### Minimum Interval

- **Minimum**: 1 millisecond
- **Intervals < 1ms**: Will throw `std::invalid_argument`

### Timing Accuracy

| Load Condition | Accuracy |
|----------------|----------|
| Light load | ±1-2ms |
| Moderate load | ±2-5ms |
| Heavy load | ±5-20ms |

**Factors Affecting Accuracy**:
- System scheduler granularity
- CPU load
- Thread priority
- OS scheduler policy

### Drift Correction

Repeating timers automatically correct for drift:

```cpp
// Timer fires at: 100ms, 200ms, 300ms, 400ms...
// NOT:          100ms, 202ms, 305ms, 409ms... (drift)
```

The timer uses **absolute time calculations** to prevent cumulative drift.

### System Time Changes

Timer uses `std::chrono::steady_clock`, which is **not affected by**:
- System time changes (daylight saving, manual adjustment)
- NTP synchronization
- Clock skew

### Heavy Load Behavior

Under heavy system load:
- Timeout may be delayed
- Timeout events are **never lost**
- Multiple timeouts never coalesce into one

**Example**: If 3 timeouts are missed due to load, 3 timeout signals will be emitted when CPU becomes available.

---

## Best Practices

### DO:

 **Stop timers in destructors**:
```cpp
~MyClass()
{
    timer_.stop();
}
```

 **Use QUEUED for cross-thread**:
```cpp
timer.timeout.connect(receiver, &Receiver::slot, ConnectionType::QUEUED);
```

 **Check `isActive()` before operations**:
```cpp
if (timer.isActive()) {
    timer.stop();
}
```

 **Use `Timer::singleShot()` for one-time delays**:
```cpp
auto t = Timer::singleShot(1000, []() { /* action */ });
```

 **Keep singleShot timer alive**:
```cpp
auto timer = Timer::singleShot(5000, slot);
// Store timer or it will be destroyed immediately
```

### DON'T:

**Don't block in timeout slots**:
```cpp
// BAD
timer.timeout.connect([]() {
    std::this_thread::sleep_for(5s);  // Blocks timer thread!
});
```

**Don't use very short intervals unnecessarily**:
```cpp
// BAD - wastes CPU
timer.start(1);  // Fires 1000 times per second!

// GOOD
timer.start(100);  // 10 times per second
```

**Don't rely on exact timing**:
```cpp
// BAD - assumes exact timing
if (timer.remainingTime() == 100) { /* ... */ }

// GOOD - check ranges
if (timer.remainingTime() < 200 && timer.remainingTime() > 0) { /* ... */ }
```

**Don't forget to process events for QUEUED**:
```cpp
// BAD - events never processed
timer.timeout.connect(receiver, &Receiver::slot, ConnectionType::QUEUED);
// Missing: EventDispatcher::current()->processEvents();
```

---

## Examples

### Example 1: Simple Countdown

```cpp
void countdown(int seconds)
{
    Timer timer;

    timer.timeout.connect([&]() {
        std::cout << seconds-- << "..." << std::endl;
        if (seconds < 0) {
            timer.stop();
            std::cout << "Blast off!" << std::endl;
        }
    });

    timer.start(1000);
}
```

### Example 2: Stopwatch

```cpp
class Stopwatch
{
public:
    void start()
    {
        startTime_ = std::chrono::steady_clock::now();
        timer_.start(100);  // Update every 100ms
    }

    void stop()
    {
        timer_.stop();
    }

private:
    Stopwatch()
    {
        timer_.timeout.connect([this]() {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - startTime_
            );
            std::cout << "\rElapsed: " << elapsed.count() << "ms" << std::flush;
        });
    }

    Timer timer_;
    std::chrono::steady_clock::time_point startTime_;
};
```

### Example 3: Rate Limiter

```cpp
class RateLimiter
{
public:
    RateLimiter(int requestsPerSecond)
    {
        int intervalMs = 1000 / requestsPerSecond;

        timer_.timeout.connect([this]() {
            if (!queue_.empty()) {
                auto request = queue_.front();
                queue_.pop();
                processRequest(request);
            }
        });

        timer_.start(intervalMs);
    }

    void enqueue(Request request)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(request);
    }

private:
    Timer timer_;
    std::queue<Request> queue_;
    std::mutex mutex_;
};
```

### Example 4: Retry Mechanism

```cpp
class RetryOperation
{
public:

  RetryOperation(int maxRetries, int retryDelayMs): 
  maxRetries_(maxRetries),
  retryDelayMs_(retryDelayMs)
  {
    retryTimer_.setSingleShot(true);
    retryTimer_.timeout.connect([this](){ retry(); });
  }

  void execute()
  {
     if (tryOperation()) {
         std::cout << "Success!" << std::endl;
     } else {
          scheduleRetry();
    }
  }

private:
  
  void scheduleRetry()
  {
     if (retries_ < maxRetries_) 
     {
         retries_++;
         std::cout << "Retry " << retries_ << " in "
                   << retryDelayMs_ << "ms..." << std::endl;
         retryTimer_.start(retryDelayMs_);
     } 
     else
     {
         std::cout << "Failed after " << maxRetries_ << " retries" << std::endl;
     }
  }

  void retry()
  {
      execute();
  }

  bool tryOperation()
  {
      // Attempt operation
      return (rand() % 3) == 0;  // Simulate success/failure
  }

  Timer retryTimer_;
  int retries_ = 0;
  int maxRetries_;
  int retryDelayMs_;
};
```

---

## Comparison with Other Approaches

| Feature | fb::Timer | std::thread sleep | Boost.Asio timer |
|---------|-----------|-------------------|------------------|
| Signal/Slot integration | Yes | No | No |
| Thread-safe | Yes | N/A | Yes |
| Single-shot mode | Yes | Manual | Yes |
| Repeating mode | Yes | Manual | Manual |
| Drift correction | Yes | No | No |
| Remaining time query | Yes | No | Yes |

---

## What's NOT Implemented

- **Sub-millisecond precision**: Minimum interval is 1ms
- **Timer wheel/hierarchy**: Single timer per instance
- **Pause without reset**: Use stop()/start() to pause
- **Multiple timeout signals**: One signal per timer

---

## See Also


- [fb_signal Documentation](../../fb_signal/doc/index.md) - Signal/slot system
- [Examples](../examples/timer_examples.cpp) - Complete example applications
- [Unit Tests](../ut/test_timer.cpp) - Comprehensive test coverage

---

## Summary

The Timer class provides:
-  High-resolution timing (millisecond precision)
-  Single-shot and repeating modes
-  Thread-safe operation
-  Signal/slot integration with fb_signal
-  Automatic drift correction
-  Remaining time queries
-  Move semantics support

Perfect for:
- Periodic tasks
- Timeouts and watchdogs
- Delayed actions
- Countdowns and stopwatches
- Rate limiting
- Heartbeat monitoring
- Retry mechanisms
