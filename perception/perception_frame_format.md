# UGV Perception Frame Format

## Purpose

This document defines the binary contract between the Python perception pipeline and the C++ navigation stack.

The contract is intentionally explicit so both sides can exchange perception data without relying on JSON or implementation-specific serialization.

---

## Frame Dimensions

Current perception resolution:

- Width: 512 pixels
- Height: 512 pixels
- Pixels: 262144

These dimensions are currently determined by `IMAGE_SIZE` in `perception/full_prediction.py`.

They are provisional runtime parameters and may be changed later when the camera/inference pipeline is optimized for the target hardware.

---

## Data Fields

Each perception frame contains four numerical arrays.

### 1. Segmentation

Type:

`uint8`

Shape:

`512 × 512`

Values:

- `0` = traversable
- `1` = non-traversable
- `2` = obstacle

---

### 2. Confidence

Type:

`float32`

Shape:

`512 × 512`

Range:

`0.0` to `1.0`

Meaning:

Per-pixel confidence of the semantic segmentation prediction.

---

### 3. Depth

Type:

`float32`

Shape:

`512 × 512`

Range:

`0.0` to `1.0`

Important:

This is normalized relative depth produced by Depth Anything V2.

It is **not physical distance in meters**.

It must not be interpreted as obstacle distance without a validated camera/depth calibration model.

---

### 4. Roughness

Type:

`float32`

Shape:

`512 × 512`

Range:

`0.0` to `1.0`

Meaning:

Normalized local terrain/depth roughness estimate.

The current roughness calculation is provisional and has not been validated as a safety-critical terrain measurement.

---

## Memory Layout

Arrays are stored in row-major order.

For a pixel:

`index = y × width + x`

where:

- `0 ≤ x < 512`
- `0 ≤ y < 512`

The complete arrays are stored in this order:

1. Segmentation
2. Confidence
3. Depth
4. Roughness

---

## Byte Layout

There is currently no padding between arrays.

Each array contains:

`512 × 512 = 262144 elements`

Therefore:

### Segmentation

`262144 × 1 byte = 262144 bytes`

### Confidence

`262144 × 4 bytes = 1048576 bytes`

### Depth

`262144 × 4 bytes = 1048576 bytes`

### Roughness

`262144 × 4 bytes = 1048576 bytes`

### Total Payload

`3407872 bytes`

Approximately:

`3.25 MiB`

---

## Frame Metadata

The binary payload must eventually be accompanied by metadata:

- Width
- Height
- Timestamp
- Frame sequence number

The timestamp will be used by the C++ freshness policy.

The sequence number will allow the runtime to detect:

- dropped frames
- repeated frames
- stale frames
- out-of-order frames

The exact metadata header representation will be defined before implementing the transport layer.

---

## Safety Rules

The following assumptions are mandatory:

1. A missing or invalid frame must never be treated as safe.

2. Non-finite floating-point values are invalid.

3. Segmentation IDs outside `[0, 2]` are invalid.

4. Confidence, depth, and roughness outside `[0.0, 1.0]` are invalid.

5. A stale frame must be rejected by the C++ runtime.

6. Normalized Depth Anything V2 output must never be interpreted directly as metric obstacle distance.

7. The current segmentation model is an experimental perception component and its predictions are not by themselves a safety-certified obstacle detector.

---

## Current Status

The Python perception pipeline currently produces the required four arrays and validates their shape, dtype, finite values, semantic IDs, and normalized ranges.

The C++ `PerceptionFrame` structure already represents the same logical data.

The transport mechanism has not yet been implemented.