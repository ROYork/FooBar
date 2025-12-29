# fb_signal Usage Examples

## Quick Start

```cpp
#include <fb/signal.hpp>
#include <iostream>

int main()
{
  // Create a signal with int argument
  fb::signal<int> on_value_changed;
  
  // Connect a lambda
  auto conn = on_value_changed.connect([](int value) {
    std::cout << "Value changed to: " << value << "\n";
  });
  
  // Emit the signal
  on_value_changed.emit(42);  // Prints: Value changed to: 42
  
  // Disconnect when done
  conn.disconnect();
}
```

## Basic Usage

### Multiple Arguments

```cpp
fb::signal<int, const std::string&, double> on_data;

on_data.connect([](int id, const std::string& name, double value) {
  std::cout << id << ": " << name << " = " << value << "\n";
});

on_data.emit(1, "temperature", 23.5);
```

### Void Signals (No Arguments)

```cpp
fb::signal<> on_shutdown;

on_shutdown.connect([]() {
  std::cout << "Shutting down...\n";
});

on_shutdown.emit();
```

### Using operator() Instead of emit()

```cpp
fb::signal<int> sig;
sig.connect([](int x) { std::cout << x << "\n"; });

sig(42);  // Same as sig.emit(42)
```

## Connection Types

### Lambda Expressions

```cpp
// Non-capturing
sig.connect([](int x) { std::cout << x << "\n"; });

// Capturing by reference
int sum = 0;
sig.connect([&sum](int x) { sum += x; });

// Capturing by value
int multiplier = 10;
sig.connect([multiplier](int x) { std::cout << x * multiplier << "\n"; });
```

### Free Functions

```cpp
void handle_value(int x) {
  std::cout << "Got: " << x << "\n";
}

sig.connect(&handle_value);
```

### Member Functions

```cpp
class Handler {
public:
  void on_value(int x) {
    std::cout << "Handler got: " << x << "\n";
  }
};

Handler handler;
sig.connect(&handler, &Handler::on_value);
```

### Functors

```cpp
struct Logger {
  void operator()(int x) {
    std::cout << "[LOG] " << x << "\n";
  }
};

Logger logger;
sig.connect(std::ref(logger));  // Use std::ref to avoid copy
```

### std::function

```cpp
std::function<void(int)> func = [](int x) { std::cout << x << "\n"; };
sig.connect(func);
```

## Connection Lifecycle

### Manual Disconnect

```cpp
auto conn = sig.connect([](int) {});

if (conn.connected()) {
  conn.disconnect();
}
```

### Scoped Connection (RAII)

```cpp
{
  fb::scoped_connection sc = sig.connect([](int) {});
  sig.emit(1);  // Slot called
}  // Automatically disconnected

sig.emit(2);  // Slot NOT called
```

### Connection Guard (Multiple Connections)

```cpp
fb::connection_guard guard;

guard += sig1.connect([](int) {});
guard += sig2.connect([](int) {});
guard += sig3.connect([](int) {});

// All connections managed together
guard.block_all();
guard.unblock_all();
guard.disconnect_all();
```

### Temporary Blocking

```cpp
auto conn = sig.connect([](int) { std::cout << "Called\n"; });

conn.block();
sig.emit(1);  // NOT printed

conn.unblock();
sig.emit(2);  // Prints: Called
```

### Connection Blocker (RAII Block)

```cpp
auto conn = sig.connect([](int) {});

{
  fb::connection_blocker blocker(conn);
  sig.emit(1);  // Slot NOT called
}  // Automatically unblocked

sig.emit(2);  // Slot called
```

## Priority-Based Ordering

```cpp
using fb::priority;

sig.connect([](int) { std::cout << "Low\n"; }, priority::low);
sig.connect([](int) { std::cout << "High\n"; }, priority::high);
sig.connect([](int) { std::cout << "Normal\n"; }, priority::normal);

sig.emit(0);
// Output:
// High
// Normal
// Low
```

## Filtered Connections

```cpp
sig.connect_filtered(
  [](int x) { std::cout << "Even: " << x << "\n"; },
  [](int x) { return x % 2 == 0; }  // Filter: only even numbers
);

sig.emit(1);  // Nothing printed
sig.emit(2);  // Prints: Even: 2
sig.emit(3);  // Nothing printed
sig.emit(4);  // Prints: Even: 4
```

## Class Member Integration

### Pattern: Store scoped_connection in Class

```cpp
class OrderHandler {
public:
  void subscribe(fb::signal<Order>& on_order) {
    m_order_conn = on_order.connect([this](const Order& order) {
      process_order(order);
    });
  }
  
private:
  void process_order(const Order& order) {
    // Handle the order
  }
  
  fb::scoped_connection m_order_conn;  // Auto-disconnect on destruction
};
```

### Pattern: Multiple Subscriptions

```cpp
class TradingEngine {
public:
  void connect_all(MarketData& md) {
    m_connections += md.on_quote.connect([this](const Quote& q) { handle_quote(q); });
    m_connections += md.on_trade.connect([this](const Trade& t) { handle_trade(t); });
    m_connections += md.on_status.connect([this](int s) { handle_status(s); });
  }
  
  void disconnect_all() {
    m_connections.disconnect_all();
  }
  
private:
  fb::connection_guard m_connections;
};
```

## Cross-Thread Event Queue

### Basic Usage

```cpp
#include <fbls/event_queue.hpp>

// Get thread-local queue
auto& queue = fb::thread_event_queue();

// Producer thread enqueues
std::thread producer([&queue]() {
  queue.enqueue([]() { std::cout << "Hello from producer\n"; });
});

// Consumer thread processes
queue.process_pending();  // Invoke all pending events

producer.join();
```

### With Signal Integration

```cpp
class WorkerThread {
public:
  void start() {
    m_running = true;
    m_thread = std::thread([this]() { run(); });
  }
  
  void stop() {
    m_running = false;
    m_thread.join();
  }
  
  // Called from any thread - queues for processing on worker thread
  void post(std::function<void()> func) {
    m_queue.enqueue(std::move(func));
  }
  
private:
  void run() {
    while (m_running) {
      m_queue.process_pending();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  
  fb::event_queue m_queue;
  std::thread m_thread;
  std::atomic<bool> m_running{false};
};
```

## Signal in Class Interface

### Publisher Class

```cpp
class PriceSource {
public:
  // Public signal for subscribers
  fb::signal<double>& on_price() { return m_on_price; }
  
  void update_price(double price) {
    m_last_price = price;
    m_on_price.emit(price);
  }
  
private:
  fb::signal<double> m_on_price;
  double m_last_price = 0.0;
};
```

### Subscriber Class

```cpp
class PriceDisplay {
public:
  void subscribe(PriceSource& source) {
    m_conn = source.on_price().connect([this](double price) {
      display(price);
    });
  }
  
private:
  void display(double price) {
    std::cout << "Price: " << std::fixed << std::setprecision(2) << price << "\n";
  }
  
  fb::scoped_connection m_conn;
};
```

## Advanced Patterns

### Chain of Responsibility

```cpp
fb::signal<Request&> on_request;

// Each handler tries to process, marks as handled if successful
on_request.connect([](Request& req) {
  if (can_handle_type_a(req)) {
    handle_type_a(req);
    req.handled = true;
  }
}, fb::priority::high);

on_request.connect([](Request& req) {
  if (!req.handled && can_handle_type_b(req)) {
    handle_type_b(req);
    req.handled = true;
  }
}, fb::priority::normal);
```

### Observer with Cleanup

```cpp
class Observable {
public:
  fb::signal<int> on_change;
  
  ~Observable() {
    // All slots automatically become invalid
    // (they hold weak_ptr to signal's internal data)
  }
};

// Observers automatically know when Observable is destroyed
fb::connection conn = observable.on_change.connect([](int) {});
// After observable is destroyed: conn.connected() == false
```
