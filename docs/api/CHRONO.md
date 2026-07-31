# CHRONO

The wall clock, reached through [`FABRIC::Chrono`](FABRIC.md#chrono). It answers "what time is it now?" and hands you the calendar logic that sits behind a [`MOMENT`](MOMENT.md). It is the SDK's analog of the JavaScript `Date` *namespace* (`Date.now`), where a [`MOMENT`](MOMENT.md) is the analog of a `Date` *instance* - splitting the "two hats" JavaScript's `Date` wears into two names on purpose.

`CHRONO` is a zero-cost view: it wraps the fabric handle and allocates nothing. The clock itself is global (process-wide, from the host's real-time clock), so these calls report the same instant for every fabric.

All times are UTC at the scalar level; local-versus-UTC only matters once you read a calendar off a [`MOMENT`](MOMENT.md).

## Methods

### Time

```rust
pub fn Time (&self) -> i64
```

- **Parameters:** none.
- **Returns:** the current instant as `tm` - 1/64 s since 1601-01-01 UTC.
- **Description:** The engine-native time scalar. Use it when you want the current moment as a single integer in the engine's own grain (for example, to arm nothing but compare two instants cheaply). For a full calendar, use [`Now`](#now).
- **Example:**

```rust
let tmNow = pFabric.Chrono ().Time ();
```

- **See also:** [`Date`](#date), [`Now`](#now).

### Date

```rust
pub fn Date (&self) -> i64
```

- **Parameters:** none.
- **Returns:** the current instant as `dt` - Unix milliseconds since 1970-01-01 UTC.
- **Description:** The web-native time scalar, identical to JavaScript's `Date.now ()`. Use it when you want a millisecond timestamp compatible with other web tooling.
- **Example:**

```rust
let dtNow = pFabric.Chrono ().Date ();
```

- **See also:** [`Time`](#time), [`Now`](#now).

### Now

```rust
pub fn Now (&self) -> MOMENT
```

- **Parameters:** none.
- **Returns:** a [`MOMENT`](MOMENT.md) for the current instant, fully filled (both scalars, both calendars).
- **Description:** The way you get a workable time value. One host call fills the whole [`MOMENT`](MOMENT.md), so every getter on the result reads a cached field with no further crossing. This is the analog of `new Date ()`.
- **Example:**

```rust
let m = pFabric.Chrono ().Now ();
pFabric.Console ().Log (&m.String_Iso ());
```

- **See also:** [`MOMENT`](MOMENT.md), [`PERFORMANCE`](PERFORMANCE.md) (for monotonic elapsed timing rather than wall time).

## See also

- [`MOMENT`](MOMENT.md) - the value `Now` returns and everything you can read from it.
- [`PERFORMANCE`](PERFORMANCE.md) - the monotonic clock; use it to measure durations, not to read the calendar.
- [`TIMER`](TIMER.md) - schedule a callback rather than poll the clock.
- [API overview](overview.md).
