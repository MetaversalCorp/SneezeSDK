# MOMENT

A wall-clock instant - the value [`CHRONO`](CHRONO.md) hands you for "now" and the thing you read a calendar out of. It is the SDK's analog of a JavaScript `Date` *instance* (the value), where [`CHRONO`](CHRONO.md) is the analog of the `Date` *namespace* (the clock). Keeping the two apart is deliberate: a `MOMENT` never means "the current time", it means "this particular time".

A `MOMENT` is a 44-byte value you hold by copy or on the stack - there is nothing to free. When the host fills one it writes **both** scalar forms and **both** calendar breakdowns at once, so every getter below reads a cached field with no round-trip to the engine. Only the setters and `Format` cross back, because the host owns all calendar normalization.

Sub-second precision is stored once, canonically, as 100 ns units; `Milli` (milliseconds) and `Tick` (1/64 s) are derived views of it. A freshly constructed `MOMENT` is zeroed, which is the **invalid sentinel** (`IsValid` is `false`) until a host fill stamps it.

## Two calendars: local and UTC

Every filled `MOMENT` carries two breakdowns of the same instant - one in the local zone, one in UTC - plus the local offset from UTC in minutes. The bare accessors (`Year`, `Hour`, ...) read the **local** calendar; the `_Utc` accessors read UTC. The underlying instant is identical; only the calendar view differs. This mirrors JavaScript `Date`'s `getFullYear` / `getUTCFullYear` split.

## Construction

```rust
pub fn Null      () -> MOMENT
pub fn From_Time (tm: i64) -> MOMENT
pub fn From_Date (dt: i64) -> MOMENT
pub fn From_Parts (nYear: i32, nMonth: i32, nDay: i32, nHour: i32, nMinute: i32, nSecond: i32, eZone: eSNEEZE_ABI_CHRONO_ZONE) -> MOMENT
pub fn Parse (sText: &str, eZone: eSNEEZE_ABI_CHRONO_ZONE) -> MOMENT
```

- **Description:**
  - `Null` - a zeroed, invalid `MOMENT`. Useful as a placeholder; `IsValid` returns `false` until it is filled.
  - `From_Time` - fill from a `tm` scalar (1/64 s since 1601-01-01 UTC, the engine-native grain).
  - `From_Date` - fill from a `dt` scalar (Unix milliseconds since 1970-01-01 UTC, the web-native grain).
  - `From_Parts` - build from civil components interpreted in `eZone`. Out-of-range fields normalize the way JavaScript `Date` does (month 13 rolls to next January, day 0 to the last day of the prior month). Sub-second is zero.
  - `Parse` - read an ISO-8601 string. A trailing `Z` forces UTC; an otherwise naive string is read in `eZone`. On a parse failure the result is the invalid sentinel.
- **Example:**

```rust
let m0 = pHost.Chrono ().Now ();                                    // right now
let m1 = MOMENT::From_Date (0);                                       // 1970-01-01T00:00:00Z
let m2 = MOMENT::From_Parts (2026, 7, 30, 14, 0, 0,
   eSNEEZE_ABI_CHRONO_ZONE::kSNEEZE_ABI_CHRONO_ZONE_UTC);
let m3 = MOMENT::Parse ("2026-07-30T14:00:00Z",
   eSNEEZE_ABI_CHRONO_ZONE::kSNEEZE_ABI_CHRONO_ZONE_UTC);
```

- **See also:** [`CHRONO::Now`](CHRONO.md#now), [`PERFORMANCE::Origin`](PERFORMANCE.md#origin).

## Validity and raw scalars

```rust
pub fn IsValid     (&self) -> bool
pub fn Time        (&self) -> i64
pub fn Date        (&self) -> i64
pub fn Zone_Offset (&self) -> i32
```

- **Description:**
  - `IsValid` - `true` once the instant has been filled (its month is non-zero). A `Null` or failed-`Parse` `MOMENT` is `false`.
  - `Time` - the `tm` scalar: 1/64 s since 1601-01-01 UTC.
  - `Date` - the `dt` scalar: Unix milliseconds since 1970-01-01 UTC (JavaScript `Date.getTime`).
  - `Zone_Offset` - the local zone's offset from UTC, in minutes, at this instant.
- **Example:**

```rust
let m = pHost.Chrono ().Now ();
if m.IsValid ()
{
   pHost.Console ().Log (&format! ("unix ms = {}", m.Date ()));
}
```

## Calendar accessors

The bare accessors read the **local** calendar; each has a `_Utc` twin that reads the UTC calendar of the same instant.

```rust
pub fn Year    (&self) -> i32      pub fn Year_Utc    (&self) -> i32
pub fn Month   (&self) -> i32      pub fn Month_Utc   (&self) -> i32
pub fn Day     (&self) -> i32      pub fn Day_Utc     (&self) -> i32
pub fn Weekday (&self) -> i32      pub fn Weekday_Utc (&self) -> i32
pub fn Hour    (&self) -> i32      pub fn Hour_Utc    (&self) -> i32
pub fn Minute  (&self) -> i32      pub fn Minute_Utc  (&self) -> i32
pub fn Second  (&self) -> i32      pub fn Second_Utc  (&self) -> i32
pub fn Milli   (&self) -> i32      pub fn Milli_Utc   (&self) -> i32
pub fn Tick    (&self) -> i32      pub fn Tick_Utc    (&self) -> i32
```

- **Description:** Read one field of the breakdown. `Month` is **1-based** (July = 7), following Windows `SYSTEMTIME` rather than JavaScript's 0-based month. `Weekday` is 0-based with Sunday = 0. `Milli` is the sub-second in milliseconds; `Tick` is the same sub-second in 1/64 s ticks - both derived from the one canonical 100 ns fraction, so `Milli` may round where `Tick` does not.
- **Example:**

```rust
let m = pHost.Chrono ().Now ();
pHost.Console ().Log (&format! ("{:04}-{:02}-{:02}", m.Year (), m.Month (), m.Day ()));
```

- **See also:** [`String_Iso`](#formatting), [`Format`](#formatting).

## Setters

Setters mutate the `MOMENT` in place (JavaScript `Date`'s `set*` semantics): the SDK substitutes your one component into the cached breakdown and re-sends the whole set, and the host renormalizes and rewrites both breakdowns and both scalars. The bare setters operate on the **local** calendar; the `_Utc` twins operate on UTC.

```rust
pub fn Year_Set   (&mut self, nYear: i32)     pub fn Year_Utc_Set   (&mut self, nYear: i32)
pub fn Month_Set  (&mut self, nMonth: i32)    pub fn Month_Utc_Set  (&mut self, nMonth: i32)
pub fn Day_Set    (&mut self, nDay: i32)      pub fn Day_Utc_Set    (&mut self, nDay: i32)
pub fn Hour_Set   (&mut self, nHour: i32)     pub fn Hour_Utc_Set   (&mut self, nHour: i32)
pub fn Minute_Set (&mut self, nMinute: i32)   pub fn Minute_Utc_Set (&mut self, nMinute: i32)
pub fn Second_Set (&mut self, nSecond: i32)   pub fn Second_Utc_Set (&mut self, nSecond: i32)
pub fn Milli_Set  (&mut self, nMilli: i32)    pub fn Milli_Utc_Set  (&mut self, nMilli: i32)
pub fn Tick_Set   (&mut self, nTick: i32)     pub fn Tick_Utc_Set   (&mut self, nTick: i32)
pub fn Date_Set   (&mut self, dt: i64)
```

- **Description:** Each component setter overwrites one field and renormalizes, so `Month_Set (13)` rolls the year forward. `Milli_Set` and `Tick_Set` both write the one canonical sub-second, so setting milliseconds does not lose precision that ticks would keep, and vice versa. `Date_Set` replaces the whole instant from a `dt` scalar (JavaScript `Date.setTime`).
- **Example:**

```rust
let mut m = pHost.Chrono ().Now ();
m.Hour_Set (0);
m.Minute_Set (0);
m.Second_Set (0);   // local midnight today
```

- **See also:** [`From_Parts`](#construction).

## Formatting

```rust
pub fn String_Iso (&self) -> String
pub fn Json       (&self) -> String
pub fn Format     (&self, eZone: eSNEEZE_ABI_CHRONO_ZONE, sSpec: &str) -> String
pub fn String     (&self) -> String
pub fn String_Utc (&self) -> String
```

- **Description:**
  - `String_Iso` - ISO-8601 UTC, `YYYY-MM-DDTHH:MM:SS.mmmZ`, built guest-side from the cached UTC breakdown with no host crossing (JavaScript `toISOString`).
  - `Json` - the same ISO-8601 UTC string (JavaScript `toJSON`).
  - `Format` - render through the host formatter. An empty `sSpec` yields the default ISO rendering for the zone; a non-empty `sSpec` is a `strftime`-style pattern applied to the selected calendar.
  - `String` - the default local rendering (JavaScript `toString`).
  - `String_Utc` - the default UTC rendering (JavaScript `toUTCString`).
- **Example:**

```rust
let m = pHost.Chrono ().Now ();
pHost.Console ().Log (&m.String_Iso ());
pHost.Console ().Log (&m.Format (eSNEEZE_ABI_CHRONO_ZONE::kSNEEZE_ABI_CHRONO_ZONE_LOCAL, "%A %B %d"));
```

- **See also:** [`CHRONO`](CHRONO.md), [API overview](overview.md).
