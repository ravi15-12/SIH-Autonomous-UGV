# Perception Shared Memory Layout

## 1. Purpose

This document defines the binary shared-memory contract between:

- Python perception runtime
- C++ navigation runtime

The interface is intended for continuous perception-frame transport.

Python is the producer.

C++ is the consumer.

The transport layer must preserve:

- frame integrity
- metadata consistency
- synchronization
- frame freshness
- memory boundaries

---

# 2. Frame Representation

Each perception frame contains:

1. A fixed-size binary header
2. Segmentation data
3. Confidence data
4. Relative depth data
5. Ground-roughness data

The complete frame is laid out sequentially in memory.

```text
+----------------------------+
| Perception Frame Header    |
| 36 bytes                   |
+----------------------------+
| Segmentation               |
| uint8                      |
+----------------------------+
| Confidence                 |
| float32                    |
+----------------------------+
| Depth                      |
| float32                    |
+----------------------------+
| Roughness                  |
| float32                    |
+----------------------------+
```

---

# 3. Perception Resolution

The current perception representation uses:

```text
Width:  512 pixels
Height: 512 pixels
```

Total number of pixels:

```text
512 × 512 = 262,144
```

All arrays use row-major ordering.

For pixel coordinates:

```text
index = y × width + x
```

where:

```text
0 <= x < 512
0 <= y < 512
```

---

# 4. Canonical Semantic Classes

Segmentation uses the following system-wide semantic IDs:

| ID | Semantic class |
|---:|---|
| 0 | Unknown |
| 1 | Traversable |
| 2 | Non-traversable |
| 3 | Obstacle |

These IDs are the canonical IDs used by the C++ navigation stack.

The Python model's internal class IDs must be converted to these canonical IDs before the frame enters the transport layer.

---

# 5. Segmentation

## Data type

```text
uint8
```

## Valid values

```text
0
1
2
3
```

## Number of elements

```text
512 × 512 = 262,144
```

## Size

```text
262,144 bytes
```

The segmentation array contains exactly one semantic class ID for every pixel.

---

# 6. Confidence

## Data type

```text
float32
```

## Valid range

```text
0.0 <= confidence <= 1.0
```

## Number of elements

```text
512 × 512 = 262,144
```

## Size

```text
262,144 × 4
= 1,048,576 bytes
```

Confidence values must be finite.

NaN and infinity are invalid.

---

# 7. Depth

## Data type

```text
float32
```

## Valid range

```text
0.0 <= depth <= 1.0
```

## Number of elements

```text
512 × 512 = 262,144
```

## Size

```text
262,144 × 4
= 1,048,576 bytes
```

## Important limitation

Depth currently represents normalized relative depth.

It is **not metric distance**.

Therefore:

```text
depth = 0.5
```

does not mean:

```text
0.5 metres
```

The C++ navigation system must not interpret this value directly as physical distance.

Metric depth may be introduced later if a suitable depth sensor or validated depth-estimation pipeline becomes available.

---

# 8. Roughness

## Data type

```text
float32
```

## Valid range

```text
0.0 <= roughness <= 1.0
```

## Number of elements

```text
512 × 512 = 262,144
```

## Size

```text
262,144 × 4
= 1,048,576 bytes
```

Roughness represents an estimated ground-surface irregularity measure.

It is not a direct physical measurement of terrain elevation, suspension movement, or wheel slip.

---

# 9. Binary Frame Header

The perception frame header is exactly:

```text
36 bytes
```

The C++ structure is explicitly packed to prevent compiler-inserted padding.

Header fields:

| Field | Type | Offset | Size |
|---|---|---:|---:|
| magic | uint32 | 0 | 4 |
| version | uint16 | 4 | 2 |
| header_size | uint16 | 6 | 2 |
| width | uint32 | 8 | 4 |
| height | uint32 | 12 | 4 |
| sequence | uint64 | 16 | 8 |
| timestamp_ns | uint64 | 24 | 8 |
| payload_size | uint32 | 32 | 4 |

Total:

```text
36 bytes
```

These offsets are part of the binary protocol.

They must not be changed without increasing the protocol version.

---

# 10. Header Constants

## Magic

```text
0x55475650
```

ASCII representation:

```text
UGVP
```

This identifies the memory region as a UGV perception frame.

---

## Protocol version

```text
1
```

---

## Header size

```text
36 bytes
```

---

## Width

```text
512
```

---

## Height

```text
512
```

---

## Payload size

```text
3,407,872 bytes
```

---

# 11. Header Field Definitions

## 11.1 Magic

Type:

```text
uint32
```

Expected value:

```text
0x55475650
```

Used to verify that the memory region contains a UGV perception frame.

---

## 11.2 Version

Type:

```text
uint16
```

Expected value:

```text
1
```

Protocol changes that alter the binary layout require a new protocol version.

---

## 11.3 Header Size

Type:

```text
uint16
```

Expected value:

```text
36
```

This allows the consumer to verify the expected beginning of the payload.

---

## 11.4 Width

Type:

```text
uint32
```

Expected value:

```text
512
```

---

## 11.5 Height

Type:

```text
uint32
```

Expected value:

```text
512
```

---

## 11.6 Sequence

Type:

```text
uint64
```

The sequence number identifies frame order.

Example:

```text
1
2
3
4
5
...
```

The producer must monotonically increase the sequence number.

A repeated or decreasing sequence number indicates a producer or synchronization problem.

---

## 11.7 Timestamp

Type:

```text
uint64
```

Unit:

```text
nanoseconds
```

The timestamp must be non-zero.

The timestamp is used by the C++ navigation runtime to determine frame freshness.

---

## 11.8 Payload Size

Type:

```text
uint32
```

Expected value:

```text
3,407,872
```

This is the size of the four payload arrays.

It does not include the 36-byte header.

---

# 12. Payload Layout

The payload begins immediately after the 36-byte header.

The payload is ordered as:

```text
Segmentation
Confidence
Depth
Roughness
```

---

# 13. Segmentation Offset

Header:

```text
36 bytes
```

Therefore segmentation begins at:

```text
Offset = 36
```

Size:

```text
262,144 bytes
```

Byte range:

```text
36
through
262,179
```

The next section begins at:

```text
262,180
```

---

# 14. Confidence Offset

Confidence begins at:

```text
Offset = 262,180
```

Size:

```text
1,048,576 bytes
```

Byte range:

```text
262,180
through
1,310,755
```

The next section begins at:

```text
1,310,756
```

---

# 15. Depth Offset

Depth begins at:

```text
Offset = 1,310,756
```

Size:

```text
1,048,576 bytes
```

Byte range:

```text
1,310,756
through
2,359,331
```

The next section begins at:

```text
2,359,332
```

---

# 16. Roughness Offset

Roughness begins at:

```text
Offset = 2,359,332
```

Size:

```text
1,048,576 bytes
```

Byte range:

```text
2,359,332
through
3,407,907
```

---

# 17. Complete Payload Size

The four payload components are:

```text
Segmentation:
262,144 bytes

Confidence:
1,048,576 bytes

Depth:
1,048,576 bytes

Roughness:
1,048,576 bytes
```

Total:

```text
262,144
+ 1,048,576
+ 1,048,576
+ 1,048,576
= 3,407,872 bytes
```

Therefore:

```text
payload_size = 3,407,872
```

---

# 18. Complete Frame Size

Header:

```text
36 bytes
```

Payload:

```text
3,407,872 bytes
```

Complete frame:

```text
36 + 3,407,872
= 3,407,908 bytes
```

Approximately:

```text
3.25 MiB
```

Therefore a single complete perception frame requires:

```text
3,407,908 bytes
```

The actual shared-memory allocation may be larger because synchronization metadata and buffering may be added.

---

# 19. Byte Order

All multi-byte integer fields use:

```text
little-endian
```

Floating-point values use:

```text
IEEE-754 float32
```

The producer and consumer must use compatible binary representations.

---

# 20. Array Ordering

All arrays use row-major ordering.

For:

```text
width = 512
height = 512
```

the element at:

```text
(x, y)
```

is stored at:

```text
index = y × 512 + x
```

Examples:

```text
pixel (0, 0) → index 0

pixel (1, 0) → index 1

pixel (2, 0) → index 2

pixel (0, 1) → index 512

pixel (1, 1) → index 513
```

No additional row padding is allowed inside the payload arrays.

---

# 21. Producer

The Python perception process is the producer.

Its responsibilities are:

1. Capture a frame.
2. Run semantic segmentation.
3. Run depth estimation.
4. Calculate ground roughness.
5. Validate the resulting arrays.
6. Generate frame metadata.
7. Serialize the complete frame.
8. Publish the frame only after the complete frame has been written.

Python must never publish a partially written frame.

---

# 22. Consumer

The C++ navigation process is the consumer.

Its responsibilities are:

1. Detect that a new frame is available.
2. Read the frame header.
3. Validate the header.
4. Validate payload boundaries.
5. Validate payload values.
6. Validate frame freshness.
7. Accept the frame only if all required checks pass.
8. Convert the validated data into the C++ perception representation.
9. Pass valid perception data to traversability and navigation modules.

Invalid frames must never reach the planner or controller.

---

# 23. Synchronization Requirement

The consumer must never read a frame while the producer is modifying that frame.

The transport layer must therefore provide synchronization.

The required conceptual sequence is:

```text
Python
   |
   | Capture + inference
   |
   | Build complete frame
   v
Shared memory
   |
   | Frame becomes available
   v
C++
   |
   | Validate frame
   v
Navigation processing
```

The exact synchronization mechanism will be implemented separately.

Possible mechanisms include:

- atomic sequence numbers
- double buffering
- ring buffering
- process-shared synchronization primitives

The first implementation should prioritize correctness and deterministic behavior over maximum throughput.

---

# 24. Frame Publication Rule

A frame must not be considered available to the consumer until:

1. Header fields have been written.
2. Segmentation has been written.
3. Confidence has been written.
4. Depth has been written.
5. Roughness has been written.
6. Producer-side validation has completed.
7. The frame publication/synchronization mechanism has been updated.

The consumer must only process frames that have been fully published.

---

# 25. Frame Freshness

A valid frame must also be sufficiently recent.

The C++ runtime already contains a frame-age policy.

The consumer must reject a frame when:

```text
timestamp_ns == 0
```

or:

```text
now_timestamp_ns < frame_timestamp_ns
```

or:

```text
now_timestamp_ns - frame_timestamp_ns
```

exceeds the configured maximum frame age.

A stale frame must not be treated as current perception.

---

# 26. Sequence Validation

The consumer should track the last successfully processed sequence number.

For normal operation:

```text
1
2
3
4
5
...
```

is valid.

A repeated sequence number:

```text
10
10
```

must not be processed as a new frame.

A decreasing sequence:

```text
10
9
```

indicates an invalid producer or synchronization state.

A skipped sequence:

```text
10
12
```

may indicate a dropped frame.

A skipped frame is not necessarily a fatal error, but it must be observable through runtime diagnostics.

---

# 27. Header Validation

The following conditions are required for a valid frame:

```text
magic == 0x55475650
```

```text
version == 1
```

```text
header_size == 36
```

```text
width == 512
```

```text
height == 512
```

```text
timestamp_ns != 0
```

```text
payload_size == 3,407,872
```

Failure of any required condition means the frame must be rejected.

---

# 28. Payload Validation

The C++ consumer must validate the payload before navigation processing.

## Segmentation

Every value must satisfy:

```text
0 <= segmentation <= 3
```

---

## Confidence

Every value must be:

```text
finite
```

and:

```text
0.0 <= confidence <= 1.0
```

---

## Depth

Every value must be:

```text
finite
```

and:

```text
0.0 <= depth <= 1.0
```

Depth remains relative and must not be converted directly into meters.

---

## Roughness

Every value must be:

```text
finite
```

and:

```text
0.0 <= roughness <= 1.0
```

---

# 29. Memory Boundary Validation

Before accessing the payload, the consumer must verify that:

```text
header_size + payload_size
```

does not exceed the allocated shared-memory region.

For the current protocol:

```text
36 + 3,407,872
= 3,407,908 bytes
```

Therefore the minimum frame storage size is:

```text
3,407,908 bytes
```

The consumer must never perform an out-of-bounds read even if the header claims an unexpected payload size.

---

# 30. Invalid Frame Behavior

If a frame fails validation:

```text
Reject frame
     |
     v
Do not update navigation perception state
     |
     v
Do not generate a motion command from that frame
```

The navigation runtime must continue using its configured safe-state behavior.

A corrupted or stale perception frame must never be treated as valid sensor information.

---

# 31. Transport Failure Behavior

If the Python producer stops publishing frames:

```text
No new frame
       |
       v
Last frame becomes stale
       |
       v
C++ freshness policy rejects it
       |
       v
Navigation enters configured safe behavior
```

The system must not continue indefinitely using stale perception.

---

# 32. Producer Failure

If the Python perception process terminates unexpectedly:

```text
Python perception stops
        |
        v
No new frames
        |
        v
Existing frame becomes stale
        |
        v
C++ rejects stale frame
        |
        v
Safe navigation behavior
```

The C++ process must not assume that Python is always running.

---

# 33. Consumer Failure

If the C++ navigation process terminates or stops consuming frames:

The Python producer must not indefinitely block the perception pipeline.

The exact back-pressure behavior will be defined during transport implementation.

The first transport implementation should prioritize bounded memory usage and predictable failure behavior.

---

# 34. Shared-Memory Ownership

The producer owns frame generation.

The consumer owns frame consumption.

Neither process may modify data that is currently being consumed by the other process.

The synchronization mechanism must establish clear ownership of a frame buffer before modification or reading.

---

# 35. Buffering Strategy

The shared-memory implementation must support continuous operation.

A single-buffer design is possible but introduces a risk of producer/consumer contention.

The preferred implementation should therefore use a bounded buffering strategy such as:

```text
Double Buffer
```

or:

```text
Ring Buffer
```

The exact design will be selected during transport implementation after considering:

- perception FPS
- C++ processing latency
- memory usage
- frame freshness
- synchronization complexity

The buffer count must remain bounded.

---

# 36. Latency Requirement

The shared-memory layer must not intentionally introduce unbounded latency.

For navigation, a newer valid frame is generally more useful than an old queued frame.

The implementation should therefore be designed around frame freshness rather than accumulating an unlimited backlog.

The final policy will be established when the complete Python-to-C++ runtime is implemented and measured.

---

# 37. Current Perception Performance

The current continuous perception baseline has been tested using:

```text
datasets/video/ugv_test.mp4
```

The test video contains:

```text
826 frames
```

The current perception pipeline has successfully processed multiple consecutive frames.

The measured processing speed is currently a development baseline and is not yet a final real-time performance specification.

Performance will be re-evaluated when:

- the shared-memory transport is implemented
- the runtime is connected end-to-end
- the actual USB/CSI camera is available
- the final hardware platform is selected

---

# 38. Camera Independence

The shared-memory protocol is intentionally independent of the physical camera interface.

The producer may receive frames from:

```text
USB camera
CSI camera
video file
test source
```

The transport layer only receives the canonical perception representation.

Therefore:

```text
Camera source
      |
      v
CameraStream
      |
      v
PerceptionRuntime
      |
      v
Canonical perception frame
      |
      v
Shared memory
```

This allows the current video source to be used for development before the physical camera is available.

---

# 39. Relationship to Python API

The Python perception runtime currently produces:

```text
sequence
timestamp_ns
width
height
segmentation
confidence
depth
roughness
```

The shared-memory transport will serialize these values into the binary frame format defined by this document.

The transport layer must not change the semantic meaning of the perception data.

---

# 40. Relationship to C++ API

The C++ navigation system currently uses:

```text
PerceptionFrame
```

with:

```text
width
height
timestamp_ns
segmentation
confidence
depth
roughness
```

The shared-memory consumer will:

1. Deserialize the binary frame.
2. Validate it.
3. Convert the payload into the C++ representation.
4. Pass the validated frame into navigation processing.

---

# 41. Safety Notes

This shared-memory interface is a transport contract.

It does not make the perception system safety-certified.

The current semantic segmentation model is not safety-certified.

The current depth representation is relative depth and is not metric distance.

The current roughness representation is an estimated image-derived quantity.

Physical robot geometry remains provisional until the actual robot chassis, wheel dimensions, camera mounting position, camera height, and other hardware parameters are measured.

Safety-critical thresholds must therefore be validated on the real robot before autonomous operation.

---

# 42. Current Implementation Status

## Implemented

- Continuous Python camera/video interface
- Continuous perception runtime
- Semantic segmentation
- Confidence generation
- Relative depth generation
- Ground roughness calculation
- Canonical semantic IDs
- C++ perception frame representation
- Packed 36-byte perception frame header
- Header field offsets
- Header validation
- Frame freshness policy
- C++ perception-frame header unit test

## Not yet implemented

- Shared-memory transport
- Shared-memory allocation
- Producer/consumer synchronization
- Double buffering or ring buffering
- Runtime frame serialization
- Runtime frame deserialization
- C++ payload validation
- End-to-end Python-to-C++ transport test
- Transport performance benchmark

---

# 43. Protocol Change Policy

The binary layout defined in this document is versioned.

Current protocol:

```text
Version = 1
```

Changes that modify:

- header field order
- header field size
- payload ordering
- payload data type
- semantic class encoding
- required metadata

must result in a protocol version change.

Backward compatibility must not be assumed unless explicitly implemented and tested.

---

# 44. Current Binary Contract Summary

```text
MAGIC
    0x55475650

VERSION
    1

HEADER SIZE
    36 bytes

RESOLUTION
    512 × 512

SEGMENTATION
    uint8
    262,144 bytes

CONFIDENCE
    float32
    1,048,576 bytes

DEPTH
    float32
    1,048,576 bytes

ROUGHNESS
    float32
    1,048,576 bytes

PAYLOAD
    3,407,872 bytes

TOTAL FRAME
    3,407,908 bytes

BYTE ORDER
    little-endian

FLOAT FORMAT
    IEEE-754 float32

SEMANTIC IDS
    0 = Unknown
    1 = Traversable
    2 = Non-traversable
    3 = Obstacle
```

---

# 45. Status

This document defines the intended binary contract.

The transport implementation has **not yet been implemented**.

The next implementation stage is to build the shared-memory transport around this verified contract.