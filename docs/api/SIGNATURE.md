# SIGNATURE

The MSF signature-verification result for the fabric. Reached through [`HOST::Signature`](HOST.md#snapshot-views), a read-only view over the [Open snapshot](SNAPSHOT.md).

Where [`CONTAINER`](CONTAINER.md)`::Trust ()` gives the single overall trust level, `SIGNATURE` gives the underlying detail that produced it.

## Methods

| Method | Returns | Meaning |
|--------|---------|---------|
| `Algorithm ()` | `&str` | The signing algorithm (e.g. `RS256`). |
| `IsValid ()` | `bool` | The signature verified against the payload. |
| `IsChainTrusted ()` | `bool` | The certificate chain is trusted. |
| `IsChainExpired ()` | `bool` | The certificate chain has expired. |

## Usage

```rust
fn Open (pHost: HOST)
{
   let pSig = pHost.Signature ();

   if pSig.IsValid ()  &&  pSig.IsChainTrusted ()  &&  !pSig.IsChainExpired ()
   {
      pHost.Console ().Info ("fully verified");
   }
   else
   {
      pHost.Console ().Warn (pSig.Algorithm ());
   }
}
```

## See also

- [HOST](HOST.md#snapshot-views) - how to obtain the view.
- [CONTAINER](CONTAINER.md) - the summarized trust level.
