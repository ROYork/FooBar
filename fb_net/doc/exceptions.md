# fb_net Error Handling

## Overview

`fb_net` reports runtime failures using the C++ standard exception hierarchy.
The library no longer ships a custom `NetException` type; instead, it relies on
familiar standard exceptions so applications can integrate with existing error
handling code paths.  The guiding principles are:

- **System errors** are surfaced as `std::system_error` with portable
  `std::errc` values.
- **Programmer errors** (bad arguments, logic misuse) trigger the corresponding
  STL logic or argument exceptions (`std::invalid_argument`,
  `std::length_error`, `std::logic_error`).
- **Other failures** use `std::runtime_error` when no richer category applies.

Applications should catch the standard types that make sense for their error
handling strategy.  The sections below summarise the most common scenarios.

## std::system_error

Operations that interact with the operating system throw `std::system_error`
when they fail.  The exception embeds the native error code and a portable
`std::errc` mapping so callers can branch on well-known conditions.

| Scenario                                 | Typical `std::errc`              |
|----------------------------------------- |----------------------------------|
| Connection refused                       | `std::errc::connection_refused`  |
| Connect/accept timeout or would-block    | `std::errc::timed_out`           |
| Host unreachable / DNS errors            | `std::errc::host_unreachable`    |
| Socket already in progress               | `std::errc::operation_in_progress` |
| Permission or address-in-use errors      | `std::errc::address_in_use`, `std::errc::permission_denied` |

Always inspect `ex.code()` from the exception to obtain the rich error
information:

```cpp
try {
  client.connect(remote, std::chrono::seconds{5});
} catch (const std::system_error& ex) {
  if (ex.code() == std::errc::connection_refused) {
    retry_later();
  } else {
    log_failure(ex.code(), ex.what());
  }
}
```

## std::invalid_argument and friends

Invalid configuration parameters or malformed inputs raise
`std::invalid_argument`, `std::length_error`, or `std::out_of_range`.  Examples
include:

- Constructing `socket_address` with an unparsable host name.
- Passing a null buffer with a non-zero length.
- Requesting a multicast TTL outside the valid range.

These indicate bugs or misuse and should typically be fixed by the caller.

## std::logic_error

State-precondition failures use `std::logic_error`.  Examples:

- Attempting to use a `tcp_client` after it has been closed.
- Reinitialising a socket that is already open.
- Invoking `udp_socket::send()` before `connect()` has been called.

Catching `std::logic_error` is useful during testing to ensure your code does
not violate the API contracts.

## Best Practices

1. **Catch standard types** (`std::system_error`, `std::invalid_argument`,
   `std::logic_error`) rather than the removed `NetException`.
2. **Use error codes**: `std::system_error::code()` exposes both the native
   platform code and the portable `errc` value.
3. **Surface context**: most fb_net functions propagate descriptive messages,
   so include `ex.what()` when logging failures.
4. **Separate operational vs programmer errors**: treat `std::system_error`
   as a recoverable runtime condition, while `std::invalid_argument` /
   `std::logic_error` usually indicate bugs.

With these guidelines, legacy code that previously caught `NetException`
should be updated to handle the appropriate standard exception class, ensuring
a smoother integration with the broader C++ ecosystem.

