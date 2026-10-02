# 🚙 SIH Autonomous UGV

Autonomous Unmanned Ground Vehicle (UGV) software stack for **off-road perception, traversability mapping, localization, planning, control, safety, and runtime integration**.

This repository contains the current software implementation developed for the **SIH 2026 Autonomous UGV project**.

> **Current milestone:** ORB-SLAM3 stereo-inertial localization has been integrated with the UGV runtime and software-validated using the EuRoC dataset. Hardware sensor integration, calibration, dynamic collision avoidance, and physical UGV validation remain future development stages.

---

## 📌 Project Status

| Subsystem | Status |
|---|---|
| Semantic Segmentation | ✅ Implemented & Validated |
| Depth Estimation | ✅ Implemented |
| Traversability Mapping | ✅ Implemented |
| Metric Local Mapping | ✅ Implemented |
| ORB-SLAM3 | ✅ Software Integrated & EuRoC Validated |
| Live Pose IPC | ✅ Implemented |
| UgvRuntime Pose Integration | ✅ Implemented |
| Local Planner | 🟡 Foundation Implemented |
| Motion Controller | 🟡 Foundation Implemented |
| Dynamic Obstacle Tracking | 🔜 Planned |
| Dynamic Collision Avoidance | 🔜 Planned |
| Real Stereo Camera | 🔜 Hardware Stage |
| Real IMU | 🔜 Hardware Stage |
| Physical UGV Validation | 🔜 Future |

---

## 🧠 System Architecture

    Camera / Sensors
           │
           ├──────────────────────────────┐
           │                              │
           ▼                              ▼
    DeepLabV3Plus                    Stereo + IMU
        + Depth                           │
           │                              ▼
           ▼                         ORB-SLAM3
    Traversability                    IMU_STEREO
           │                              │
           ▼                              ▼
    Metric Local Map                 /ugv_pose
           │                              │
           └──────────────┬───────────────┘
                          ▼
                    UgvRuntime
                          │
                 ┌────────┴────────┐
                 ▼                 ▼
            Local Planner     Motion Controller

---

## 📁 Repository Structure

    SIH-Autonomous-UGV/
    │
    ├── config/
    │
    ├── cpp_core/
    │   ├── common/
    │   ├── control/
    │   ├── planning/
    │   ├── runtime/
    │   ├── transport/
    │   └── CMakeLists.txt
    │
    ├── localization/
    ├── mapping/
    ├── perception/
    │
    ├── datasets/       # excluded from Git
    ├── logs/           # excluded from Git
    ├── build/          # excluded from Git
    ├── venv/           # excluded from Git
    │
    ├── .gitignore
    └── README.md

---

## 👁️ Perception

### Semantic Segmentation

The current semantic perception model is **DeepLabV3Plus**.

| Parameter | Configuration |
|---|---|
| Architecture | DeepLabV3Plus |
| Encoder | ResNet18 |
| Encoder Weights | ImageNet |
| Classes | 3 |
| Inference Resolution | 512 × 512 |

Configuration files:

    config/model.yaml
    config/training.yaml

### Validated Model

    logs/checkpoints/experiment_02/best_model.pth

Checkpoint details:

- **Epoch:** 15
- **Best validation mIoU:** 74.11%
- **Architecture:** DeepLabV3Plus
- **Encoder:** ResNet18
- **Encoder weights:** ImageNet
- **Classes:** 3
- **Inference device:** Apple MPS

The checkpoint is intentionally excluded from Git because of its size.

The distributable model is provided through **GitHub Release `v0.1.0`**.

### Inference Validation

- Device: **Apple MPS**
- Test images available: **1,672**
- Representative images processed: **10**
- Predictions generated: **10/10**
- Result: **Successful**

The 74.11% mIoU is the best validation mIoU recorded in the checkpoint. The representative inference run confirms successful model loading and prediction generation; it is **not a new accuracy evaluation**.

---

## 🗺️ Traversability & Metric Mapping

The repository contains components for converting perception information into traversability and metric-map representations.

### Components

- Camera geometry
- Ground projection
- Traversability estimation
- Metric local mapping
- Cost-map generation
- Cost-map adapters
- Metric-map runtime
- Metric-map publishing

Relevant directories:

    mapping/
    cpp_core/traversability/

The metric-map pipeline has been exercised together with `UgvRuntime`.

> A valid metric map does not necessarily imply a valid path. Current test scenes can produce a valid metric map while reporting `FOOTPRINT UNKNOWN`, `PATH NO`, and `SAFETY STOP`.

---

## 📍 Localization

The localization package provides interfaces and pipeline components for:

- Robot pose
- Stereo camera frames
- IMU data
- Localization input
- Localization backend abstraction
- Localization pipeline
- Planner pose adaptation

Relevant directory:

    localization/

---

## 🛰️ ORB-SLAM3 Stereo-Inertial Integration

### Status: ✅ Software Integrated & Validated

**ORB-SLAM3 is now integrated with the UGV runtime.**

Validated software flow:

    EuRoC Stereo Images + IMU
              │
              ▼
         ORB-SLAM3
         IMU_STEREO
              │
              ▼
          PoseFrame
         (x, y, yaw)
              │
              ▼
          /ugv_pose
              │
              ▼
    SynchronizedPoseTransport
              │
              ▼
          UgvRuntime
              │
              ├── Footprint Validation
              ├── Local Planner
              └── Motion Controller

### ORB-SLAM3 Bridge

The UGV-specific bridge is:

    ORB_SLAM3/Examples/UGV/orbslam3_ugv_euroc.cc

The bridge:

1. Loads the ORB-SLAM3 vocabulary and settings.
2. Reads stereo images from the EuRoC dataset.
3. Reads IMU measurements.
4. Runs ORB-SLAM3 using `IMU_STEREO`.
5. Extracts the estimated pose.
6. Converts the pose to `PoseFrame`.
7. Publishes the pose through `/ugv_pose`.

---

## 🔄 Live Pose Transport

Implemented under:

    cpp_core/transport/
    cpp_core/runtime/ugv_runtime.cpp

Each pose frame contains:

    sequence
    timestamp_ns
    x_m
    y_m
    yaw_rad

The runtime uses the latest available live pose for:

- Robot footprint validation
- Local planning
- Motion controller input

The pose transport supports non-blocking consumption so the runtime can incorporate the latest available SLAM pose without blocking metric-map processing.

---

## 🧪 SLAM Validation

ORB-SLAM3 was software-validated using:

- **macOS**
- **Apple Silicon / arm64**
- **EuRoC stereo dataset**
- **EuRoC IMU data**

### Validated

- ✅ ORB-SLAM3 compiled successfully for Apple Silicon
- ✅ Stereo + IMU software pipeline configured
- ✅ `IMU_STEREO` mode configured
- ✅ Continuous pose generation observed
- ✅ Pose publication through `/ugv_pose`
- ✅ UgvRuntime consumed changing live poses
- ✅ Live pose connected to footprint/planner/controller interfaces
- ✅ Continuous runtime test completed successfully

Example observed pose stream:

    LIVE_POSE seq=1 x=-0.000198 y=0.041863 yaw=-0.004775
    LIVE_POSE seq=2 x=-0.001099 y=0.065102 yaw=-0.004814
    LIVE_POSE seq=3 x=-0.002099 y=0.090617 yaw=-0.000671
    LIVE_POSE seq=4 x=-0.001692 y=0.116756 yaw=-0.000009
    LIVE_POSE seq=5 x=-0.002472 y=0.141815 yaw=0.005009

These values demonstrate changing live pose data being generated and consumed by the UGV runtime during the software test.

> The current SLAM validation uses the EuRoC dataset and development configuration. It is **not** a substitute for final UGV sensor calibration or physical field validation.

---

## ⚠️ Hardware Limitations

The current SLAM validation is **software/dataset based**.

The following remain hardware-stage tasks:

- [ ] Real stereo-camera integration
- [ ] Real IMU integration
- [ ] Stereo camera calibration
- [ ] IMU calibration
- [ ] Camera-IMU extrinsic calibration
- [ ] Calibrated ORB-SLAM-to-UGV coordinate transform
- [ ] Motor/controller interface
- [ ] Physical UGV validation

The EuRoC calibration/configuration must **not** be interpreted as production UGV calibration.

---

## 🧭 Planning

The C++ planning subsystem contains:

- Grid coordinates
- Grid clearance
- Distance transforms
- Inflation
- Heuristics
- Neighbor generation
- Search nodes
- Search queues
- Local planner
- Planner cost handling

Relevant directory:

    cpp_core/planning/

### Remaining Planning Validation

- [ ] Robust path-generation scenarios
- [ ] Goal-reaching validation
- [ ] Narrow-passage validation
- [ ] Blocked-path recovery
- [ ] Dynamic-obstacle-aware planning

---

## 🎮 Control

The C++ control subsystem contains:

- Motion controller
- Safety command handling
- Robot geometry
- Robot footprint handling

Relevant directories:

    cpp_core/control/
    cpp_core/common/

The controller is currently validated at the software/runtime level.

Physical motor and drive integration remains hardware-dependent.

---

## 🔗 Runtime & Data Transport

The C++ runtime and transport layers provide:

- Shared-memory transport
- Synchronized perception transport
- Synchronized pose transport
- Synchronized metric-map transport
- Pose frames
- Metric-map frames
- Python perception reader
- Runtime interfaces

Relevant directories:

    cpp_core/runtime/
    cpp_core/transport/

---

## 🚧 Dynamic Obstacle Tracking

### Status: 🔜 Planned

Dynamic obstacle tracking is **not yet implemented**.

Planned pipeline:

    Perception + Depth
            │
            ▼
    Obstacle Extraction
            │
            ▼
    Object / Obstacle Tracking
            │
            ▼
    Position History
            │
            ▼
    Velocity Estimation
            │
            ▼
    Motion Prediction

### Planned Capabilities

- [ ] Dynamic obstacle extraction
- [ ] Persistent obstacle tracking
- [ ] Track IDs
- [ ] Position history
- [ ] Velocity estimation
- [ ] Motion prediction
- [ ] Dynamic risk classification

---

## 🛡️ Dynamic Collision Avoidance

### Status: 🔜 Planned

Dynamic collision avoidance will be implemented after obstacle tracking and velocity estimation.

Planned pipeline:

    Tracked Obstacle
           │
           ▼
    Current Position + Velocity
           │
           ▼
    Predicted Trajectory
           │
           ▼
    UGV Predicted Trajectory
           │
           ▼
    Collision Prediction
           │
           ▼
    Time-to-Collision / Risk
           │
           ├───────────────┐
           ▼               ▼
        SAFE           COLLISION RISK
           │               │
           ▼               ▼
     Normal Planning   Avoid / Stop
           │               │
           └───────┬───────┘
                   ▼
           Motion Controller

### Planned Capabilities

- [ ] Collision prediction
- [ ] Time-to-collision estimation
- [ ] Emergency-stop logic
- [ ] Local avoidance
- [ ] Safe trajectory generation
- [ ] Dynamic-obstacle-aware planner integration
- [ ] Multiple moving-obstacle handling
- [ ] Sudden-obstacle handling

---

## 📊 Current Project Status

### ✅ Implemented & Validated

- DeepLabV3Plus semantic segmentation
- ResNet18 ImageNet-pretrained encoder
- Training and checkpoint handling
- 74.11% best validation mIoU checkpoint
- Representative inference
- Apple MPS inference
- Depth-processing pipeline
- Traversability estimation
- Ground projection
- Metric local mapping
- Metric-map IPC
- C++ planning components
- C++ control components
- Safety command handling
- Shared-memory data transport
- Localization interfaces
- Localization pipeline structure
- **ORB-SLAM3 stereo-inertial software integration**
- **EuRoC-based SLAM validation**
- **Continuous live pose generation**
- **`/ugv_pose` pose transport**
- **UgvRuntime live-pose consumption**
- **Live pose connected to footprint/planner/controller interfaces**
- C++ runtime integration test

### 🔜 Next Software Development

1. Dynamic obstacle extraction
2. Persistent obstacle tracking
3. Velocity estimation
4. Motion prediction
5. Collision prediction
6. Dynamic collision avoidance
7. Dynamic-obstacle-aware planning
8. Navigation scenario validation
9. Runtime safety/fault handling
10. Telemetry and diagnostics

---

## 🔧 Hardware & Field Development

When the physical UGV platform is available:

1. Stereo camera integration
2. Stereo synchronization
3. Stereo calibration
4. IMU driver integration
5. IMU calibration
6. Camera-IMU calibration
7. ORB-SLAM-to-UGV extrinsic calibration
8. Motor/drive interface
9. Hardware emergency stop
10. Real-time command interface
11. Physical navigation testing
12. Field validation

---

## 🧪 Software Validation Roadmap

Planned repeatable scenarios:

- [ ] Static obstacle scenario
- [ ] Moving obstacle scenario
- [ ] Crossing obstacle
- [ ] Approaching obstacle
- [ ] Sudden obstacle
- [ ] Multiple moving obstacles
- [ ] Narrow passage
- [ ] Blocked goal
- [ ] Planner recovery
- [ ] SLAM tracking loss
- [ ] Perception failure
- [ ] Stale pose
- [ ] Stale map
- [ ] Long-duration runtime test
- [ ] Automated regression tests

---

## 📦 Dataset

Large datasets are intentionally excluded from Git.

The project uses local datasets for:

- Semantic segmentation
- Traversability development
- Model training
- Evaluation
- Inference

The EuRoC dataset used for ORB-SLAM3 validation is also kept outside the repository.

---

## 🏋️ Training

The perception training pipeline is implemented in:

    perception/train.py

Supporting components:

    perception/dataset.py
    perception/losses.py
    perception/train_smoke_test.py

Training configuration:

    config/training.yaml

---

## 🔬 Evaluation & Inference

### Evaluation

    perception/evaluate_test.py

### Test Prediction

    perception/predict_test.py

### Full Prediction Pipeline

    perception/full_prediction.py

---

## 📦 Large Files & Git Policy

The following are intentionally excluded from Git:

    datasets/
    logs/
    build/
    venv/

Generated and compiled files are also excluded:

    *.pth
    *.a
    *.so
    *.dylib
    __pycache__/
    *.pyc
    .DS_Store

Large datasets, model checkpoints, build artifacts, virtual environments, and generated logs should not be committed directly to Git history.

---

## 🖥️ Development Environment

Development and validation has primarily been performed on:

- macOS
- Apple Silicon
- Python virtual environment
- Apple MPS acceleration where available
- C++
- Python

The current SLAM validation was performed using EuRoC data rather than physical UGV sensors.

---

## 🚀 Release

### v0.1.0 — Prototype Release

The current prototype release contains distributable runtime assets that are intentionally not stored in the source repository.

### Release Assets

- DeepLabV3Plus segmentation model
- ORB-SLAM3 vocabulary (`ORBvoc.txt`)

The ORB-SLAM3 vocabulary is approximately **139 MB** and is distributed through the release rather than committed to the source repository.

---

## 🗺️ Development Roadmap

### Phase 1 — Perception

- [x] Semantic segmentation
- [x] Depth processing
- [x] Traversability estimation
- [x] Metric local mapping

### Phase 2 — Localization

- [x] ORB-SLAM3 Apple Silicon build
- [x] Stereo-inertial EuRoC software validation
- [x] Continuous pose generation
- [x] `/ugv_pose` IPC
- [x] UgvRuntime live-pose integration
- [ ] Real stereo-camera integration
- [ ] Real IMU integration
- [ ] Sensor calibration
- [ ] ORB-to-UGV extrinsic calibration

### Phase 3 — Navigation Foundation

- [x] Planner architecture
- [x] Motion-controller architecture
- [x] Metric-map to runtime integration
- [ ] Robust path-generation scenarios
- [ ] Goal-reaching validation
- [ ] Blocked-path recovery
- [ ] Narrow-passage validation

### Phase 4 — Dynamic Obstacle Intelligence

- [ ] Dynamic obstacle extraction
- [ ] Persistent obstacle tracking
- [ ] Velocity estimation
- [ ] Motion prediction
- [ ] Time-to-collision estimation
- [ ] Dynamic risk classification

### Phase 5 — Dynamic Collision Avoidance

- [ ] Collision prediction
- [ ] Emergency-stop logic
- [ ] Local avoidance
- [ ] Safe trajectory generation
- [ ] Dynamic-obstacle-aware planner integration
- [ ] Multiple moving-obstacle handling
- [ ] Sudden-obstacle handling

### Phase 6 — Safety & Runtime

- [x] Safety-command architecture
- [x] Shared-memory synchronization
- [ ] SLAM-loss safety behavior
- [ ] Perception-loss safety behavior
- [ ] Stale-pose detection
- [ ] Stale-map detection
- [ ] Communication timeout handling
- [ ] Runtime telemetry

### Phase 7 — Simulation & Software Validation

- [ ] Static obstacle scenarios
- [ ] Moving obstacle scenarios
- [ ] Crossing obstacle scenarios
- [ ] Approaching obstacle scenarios
- [ ] Sudden obstacle scenarios
- [ ] Multiple-obstacle scenarios
- [ ] Goal-blocked/recovery scenarios
- [ ] Long-duration runtime testing
- [ ] Automated regression tests

### Phase 8 — Hardware Integration

- [ ] Stereo camera driver
- [ ] Stereo synchronization
- [ ] Stereo calibration
- [ ] IMU driver
- [ ] IMU calibration
- [ ] Camera-IMU calibration
- [ ] ORB-to-UGV extrinsic calibration
- [ ] Motor/drive interface
- [ ] Hardware emergency stop
- [ ] Real-time command interface

### Phase 9 — Physical UGV Validation

- [ ] Indoor validation
- [ ] Outdoor validation
- [ ] Uneven-terrain testing
- [ ] Static obstacle avoidance
- [ ] Dynamic obstacle avoidance
- [ ] Localization stress testing
- [ ] Sensor-loss testing
- [ ] Long-duration autonomous run
- [ ] Full-system safety validation

---

## 📄 Project Disclaimer

This repository represents an actively developed **software prototype** for the SIH 2026 Autonomous UGV project.

Software validation using datasets and development hardware does not constitute physical UGV deployment or field validation.

Hardware-specific calibration, real-time sensor integration, motor control, dynamic collision avoidance, and physical field validation remain future development stages.

---

## 🔗 Repository

**GitHub:** https://github.com/ravi15-12/SIH-Autonomous-UGV

**Current Release:** `v0.1.0`

---

## 👥 SIH 2026

**Project:** Autonomous UGV  
**Team:** RORTOS  
**Purpose:** Software platform for autonomous perception, localization, navigation, safety, and future physical UGV deployment.
