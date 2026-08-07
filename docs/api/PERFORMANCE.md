# PERFORMANCE

The monotonic high-resolution clock, reached through [`HOST::Performance`](HOST.md#performance). It answers "how much time has elapsed?" - never "what time is it?". It is the SDK's analog of the browser `performance` object (`performance.now` / `performance.timeOrigin`).

Use `PERFORMANCE` to measure durations. Unlike [`CHRONO`](CHRONO.md), it never goes backward and is not affected by clock adjustments, so it is the right tool for timing work, frame pacing, or profiling. It is a zero-cost view over the fabric handle; the clock's origin is anchored per fabric when the fabric loads.

## Methods

### Now

```rust
pub fn Now (&self) -> i64
```

- **Parameters:** none.
- **Returns:** 100 ns units elapsed since this fabric's origin (its load time).
- **Description:** A monotonic counter. The absolute value is meaningless; only the difference between two reads is - subtract an earlier `Now` from a later one to get an elapsed duration in 100 ns units (divide by 10,000 for milliseconds). Mirrors `performance.now ()`, except the grain is 100 ns rather than fractional milliseconds.
- **Example:**

```rust
let nStart = pHost.Performance ().Now ();
// ... do work ...
let nElapsedMs = (pHost.Performance ().Now () - nStart) / 10000;
pHost.Console ().Log (&format! ("took {} ms", nElapsedMs));
```

- **See also:** [`Origin`](#origin), [`CHRONO::Time`](CHRONO.md#time).

### Origin

```rust
pub fn Origin (&self) -> MOMENT
```

- **Parameters:** none.
- **Returns:** a wall-clock [`MOMENT`](MOMENT.md) captured at the monotonic clock's zero point.
- **Description:** The wall-clock anchor for [`Now`](#now): the real time at which the monotonic counter was zero. Add a `Now` reading (converted to the appropriate grain) to this instant to place a monotonic sample on the wall-clock calendar. Mirrors `performance.timeOrigin`: the origin is captured per fabric, at the instant the fabric loads (like a browser document's `timeOrigin`), so each fabric's `Now` counts up from its own load. A child fabric attached later gets its own, later origin.
- **Example:**

```rust
let mOrigin = pHost.Performance ().Origin ();
pHost.Console ().Log (&format! ("clock started at {}", mOrigin.String_Iso ()));
```

- **See also:** [`Now`](#now), [`MOMENT`](MOMENT.md).

## See also

- [`CHRONO`](CHRONO.md) - the wall clock; use it to read the calendar, not to measure durations.
- [`MOMENT`](MOMENT.md) - the value `Origin` returns.
- [API overview](overview.md).
