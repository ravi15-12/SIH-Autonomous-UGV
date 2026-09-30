# UGV Perception Frame Header

## Purpose

This header defines the metadata that accompanies each binary perception frame.

The header allows the C++ runtime to validate the payload, detect stale or repeated frames, and verify that the received frame matches the expected dimensions.

---

## Header Fields

Each frame contains the following metadata:

| Field | Type | Meaning |
|---|---|---|
| magic | uint32 | Identifies a valid UGV perception frame |
| version | uint16 | Binary format version |
| header_size | uint16 | Size of the header in bytes |
| width | uint32 | Frame width in pixels |
| height | uint32 | Frame height in pixels |
| sequence | uint64 | Monotonically increasing frame number |
| timestamp_ns | uint64 | Frame timestamp in nanoseconds |
| payload_size | uint32 | Size of the binary payload in bytes |

---

## Field Requirements

### magic

A fixed constant used to identify the beginning of a valid perception frame.

Value:

`0x55475650`

This corresponds to the ASCII characters:

`UGVP`

---

### version

Current format version:

`1`

Any incompatible future format must use a new version number.

---

### header_size

Size of the complete header structure in bytes.

The receiver must verify this value before reading the payload.

---

### width

Current expected value:

`512`

---

### height

Current expected value:

`512`

The receiver must reject frames whose dimensions do not match the expected perception configuration.

---

### sequence

A monotonically increasing frame number.

The receiver can use this field to detect:

- dropped frames
- repeated frames
- out-of-order frames

The sequence number starts at:

`0`

---

### timestamp_ns

Timestamp associated with the perception frame.

Unit:

nanoseconds

The timestamp must be non-zero.

The C++ runtime uses this value for frame freshness validation.

---

### payload_size

Expected payload size:

`3407872 bytes`

The payload consists of:

1. segmentation
2. confidence
3. depth
4. roughness

No padding exists between these arrays.

---

## Complete Frame Layout

```text
┌──────────────────────────────┐
│ Header                       │
│                              │
│ magic                        │
│ version                      │
│ header_size                  │
│ width                        │
│ height                       │
│ sequence                     │
│ timestamp_ns                 │
│ payload_size                 │
├──────────────────────────────┤
│ Segmentation                 │
│ 512 × 512 uint8              │
├──────────────────────────────┤
│ Confidence                   │
│ 512 × 512 float32            │
├──────────────────────────────┤
│ Depth                        │
│ 512 × 512 float32            │
├──────────────────────────────┤
│ Roughness                    │
│ 512 × 512 float32            │
└──────────────────────────────┘
---

## Validation Rules

The receiver must reject a frame if:

1. `magic` is incorrect.
2. `version` is unsupported.
3. `header_size` is incorrect.
4. `width` is invalid.
5. `height` is invalid.
6. `sequence` is older than the previously accepted frame.
7. `timestamp_ns` is zero.
8. `payload_size` does not match the expected payload size.
9. The complete payload is not available.
10. Any payload value fails the perception-frame validation rules.

---

## Endianness

All integer fields use:

`little-endian`

Floating-point values use IEEE-754:

`float32`

---

## Current Status

The binary frame header is now formally defined.

Transport implementation has not yet been implemented.