# GBFR SDK

C++20 SDK that mirrors the structure of `granblue_fantasy_relink.exe` as
inferred from the headless Ghidra analysis (see
[../../reverse/20260510/docs/scene-and-save-structure.md](../../reverse/20260510/docs/scene-and-save-structure.md)).

## Design

- Every game-side class is wrapped by a `GameObject` that stores the
  in-process address (`m_address`).
- Field reads go through a typed `read<T>(offset)` helper backed by a
  pluggable `IMemory` (default: in-process direct pointers).
- The top-level [`gbfr::Game`](include/gbfr/game.hpp) exposes the singletons /
  managers. Singleton instance addresses must be configured at runtime (the
  game uses `cyan::Singleton<T>` registered by `string_hash32`; resolution of
  that registry is a runtime concern left to the user — once they have it,
  pass the address to `Game`).
- Save-data containers (`sys::data::*List`) are typed by `SaveListBase<T,Key,N>`
  / `SaveTempListBase<T,Key,N,C>` mirroring the C++ templates.
- vftable RVAs and `MaxCount`s recovered from RTTI are encoded as
  `static constexpr` members so each wrapper can self-verify the underlying
  object.

## Status of offsets

Field byte offsets (e.g. the offset of `level` inside `CharaData`, of the
list-array inside `SaveListBase`, of any field inside a manager) were NOT
recovered from RTTI and are marked **`GBFR_TODO_OFFSET`** throughout. They
need to be confirmed by examining decompilation or runtime memory. The SDK
will compile and load, but `read<...>()` on a TODO offset will be wrong until
patched.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```
