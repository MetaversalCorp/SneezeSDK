# TIMER

Scheduled callbacks, reached through [`HOST::Timer`](HOST.md#timer). Arm a one-shot or a repeating timer and the engine calls your module back when it fires - the analog of the browser's `setTimeout` / `setInterval` / `clearTimeout`. It is a zero-cost view over the fabric handle.

A fire is **not** a return value; it arrives later as an [`INSTANCE::Timer`](INSTANCE.md#timer) callback. Every arm returns a `twTimerIx` (the timer id, `0` on failure) and carries a `qwParam` cookie that the engine echoes back on the callback, so one handler can tell its timers apart without a lookup table.

## The unit

A timer's period is expressed with an [`eSNEEZE_ABI_TIMER_UNIT`](../incorporating-the-abi.md):

| Unit | Meaning |
|------|---------|
| `kSNEEZE_ABI_TIMER_UNIT_TICK` | `nValue` ticks of 1/64 s each |
| `kSNEEZE_ABI_TIMER_UNIT_MS`   | `nValue` milliseconds |
| `kSNEEZE_ABI_TIMER_UNIT_HZ`   | fire at `nValue` hertz (period = 1/`nValue` s) |

`HZ` is the natural way to ask for a steady rate ("60 times a second") without computing a period yourself. A non-positive `nValue` is rejected (the arm returns `0`).

## Methods

### Set

```rust
pub fn Set (&self, nValue: i32, eUnit: eSNEEZE_ABI_TIMER_UNIT, qwParam: u64) -> u64
```

- **Parameters:**
  - `nValue` - the period, interpreted per `eUnit`.
  - `eUnit` - the unit of `nValue` (tick, millisecond, or hertz).
  - `qwParam` - an opaque cookie echoed back to [`INSTANCE::Timer`](INSTANCE.md#timer) when this timer fires.
- **Returns:** the new timer's `twTimerIx`, or `0` if the unit/value was invalid.
- **Description:** Arms a **one-shot** timer that fires once, one period from now, then disarms itself. Mirrors `setTimeout`.
- **Example:**

```rust
fn Open (pHost: &HOST)
{
   pHost.Timer ().Set (500, eSNEEZE_ABI_TIMER_UNIT::kSNEEZE_ABI_TIMER_UNIT_MS, 1);
}
```

- **See also:** [`Interval`](#interval), [`Clear`](#clear), [`INSTANCE::Timer`](INSTANCE.md#timer).

### Interval

```rust
pub fn Interval (&self, nValue: i32, eUnit: eSNEEZE_ABI_TIMER_UNIT, qwParam: u64) -> u64
```

- **Parameters:** identical to [`Set`](#set).
- **Returns:** the new timer's `twTimerIx`, or `0` if the unit/value was invalid.
- **Description:** Arms a **repeating** timer that re-fires every period until you [`Clear`](#clear) it (or the fabric closes). Mirrors `setInterval`.
- **Example:**

```rust
fn Open (pHost: &HOST)
{
   // tick 60 times a second, tagged 2
   pHost.Timer ().Interval (60, eSNEEZE_ABI_TIMER_UNIT::kSNEEZE_ABI_TIMER_UNIT_HZ, 2);
}
```

- **See also:** [`Set`](#set), [`Clear`](#clear).

### Clear

```rust
pub fn Clear (&self, twTimerIx: u64) -> bool
```

- **Parameters:**
  - `twTimerIx` - the id returned by [`Set`](#set) or [`Interval`](#interval).
- **Returns:** `true` if a matching timer was found and disarmed; `false` otherwise.
- **Description:** Disarms a timer. Mirrors `clearTimeout` / `clearInterval`. A one-shot that has already fired, or an unknown id, returns `false`. Timers are also disarmed automatically when their fabric closes, so you do not have to clear them in `Close`.
- **Example:**

```rust
let twId = pHost.Timer ().Interval (1, eSNEEZE_ABI_TIMER_UNIT::kSNEEZE_ABI_TIMER_UNIT_MS, 0);
// ... later ...
pHost.Timer ().Clear (twId);
```

- **See also:** [`Set`](#set), [`Interval`](#interval).

## Receiving fires

Implement [`INSTANCE::Timer`](INSTANCE.md#timer) to receive fires. The engine passes the fabric, the timer's `twTimerIx`, and the `qwParam` you armed it with:

```rust
impl INSTANCE for MY_MODULE
{
   fn Open (pHost: &HOST)
   {
      pHost.Timer ().Interval (1, eSNEEZE_ABI_TIMER_UNIT::kSNEEZE_ABI_TIMER_UNIT_HZ, 7);
   }

   fn Timer (pHost: &HOST, twTimerIx: u64, qwParam: u64)
   {
      pHost.Console ().Log (&format! ("timer {} fired (param {})", twTimerIx, qwParam));
   }
}
```

## See also

- [`INSTANCE::Timer`](INSTANCE.md#timer) - the callback a fire is delivered to.
- [`CHRONO`](CHRONO.md) / [`PERFORMANCE`](PERFORMANCE.md) - reading the clock rather than scheduling against it.
- [API overview](overview.md).
