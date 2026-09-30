# SIH Autonomous UGV

Autonomous Unmanned Ground Vehicle (UGV) software stack for off-road perception, traversability mapping, localization, planning, control, and runtime integration.

This repository contains the current software implementation developed for the SIH 2026 autonomous UGV project.

## Overview

The system is organized into modular Python and C++ components covering:

- Semantic perception
- Traversability estimation
- Metric local mapping
- Localization interfaces
- Path planning
- Motion control
- Safety command handling
- Shared-memory and synchronized data transport
- Runtime integration
- Dataset preparation, training, evaluation, and inference

Large datasets, model checkpoints, generated logs, build artifacts, and local virtual environments are intentionally excluded from Git.

## Repository Structure

    SIH-Autonomous-UGV/
    ├── config/
    ├── cpp_core/
    ├── localization/
    ├── mapping/
    ├── perception/
    ├── datasets/
    ├── logs/
    ├── build/
    ├── venv/
    ├── .gitignore
    └── README.md

## Perception

### Semantic Segmentation

The current semantic perception model is **DeepLabV3Plus**.

| Parameter | Configuration |
|---|---|
| Architecture | DeepLabV3Plus |
| Encoder | ResNet18 |
| Encoder weights | ImageNet |
| Classes | 3 |
| Activation | None |
| Inference resolution | 512 × 512 |

Model configuration:

    config/model.yaml

Training configuration:

    config/training.yaml

### Validated Model Checkpoint

The current validated checkpoint is:

    logs/checkpoints/experiment_02/best_model.pth

Checkpoint details:

- Epoch: **15**
- Best validation mIoU: **0.7411357760**
- Best validation mIoU: **74.11%**
- Architecture: **DeepLabV3Plus**
- Encoder: **ResNet18**
- Encoder weights: **ImageNet**
- Classes: **3**

The checkpoint is approximately 141 MB and is intentionally excluded from Git.

### Perception Inference Validation

The validated checkpoint was tested using the inference pipeline on Apple Silicon.

Test details:

- Device: **Apple MPS**
- Test images available: **1,672**
- Representative images processed: **10**
- Checkpoint: `experiment_02/best_model.pth`
- Result: all 10 predictions generated successfully

Generated predictions are stored locally under:

    logs/predictions/

The 74.11% mIoU value is the best validation mIoU recorded in the checkpoint. The representative inference run confirms successful model loading and prediction generation; it is not a new accuracy evaluation.

## Traversability and Mapping

The repository contains components for converting perception information into traversability and metric-map representations.

Current components include:

- Camera geometry
- Ground projection
- Traversability estimation
- Metric local mapping
- Cost-map generation
- Cost-map adapters
- Traversability configuration
- Metric-map runtime
- Metric-map publishing

Relevant source directories:

    mapping/
    cpp_core/traversability/

## Localization

The localization package provides interfaces and pipeline components for:

- Robot pose
- Stereo camera frames
- IMU data
- Localization input
- Localization backend abstraction
- Localization pipeline
- Planner pose adaptation

Relevant source:

    localization/

### ORB-SLAM3 Status

An ORB-SLAM3 backend interface is present at:

    localization/orbslam3_backend.py

The current `ORBSLAM3Backend` provides the backend interface and pose-handling structure.

**The actual ORB-SLAM3 runtime is not yet connected to the UGV runtime pipeline.**

A separate ORB-SLAM3 build has been successfully built on Apple Silicon. This standalone build should not be interpreted as completed runtime integration with this repository.

## Planning

The C++ planning subsystem contains grid-based planning and local planning components.

Current components include:

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

Relevant source:

    cpp_core/planning/

Planning-related tests are located under:

    cpp_core/tests/

## Control

The C++ control subsystem contains:

- Motion controller
- Safety command handling
- Robot geometry
- Robot footprint handling

Relevant source:

    cpp_core/control/
    cpp_core/common/

## Runtime and Data Transport

The C++ runtime and transport layers provide infrastructure for exchanging perception, pose, and metric-map information.

Current components include:

- Shared-memory transport
- Synchronized perception transport
- Synchronized pose transport
- Synchronized metric-map transport
- Pose frames
- Metric-map frames
- Python perception reader
- Runtime interfaces

Relevant source:

    cpp_core/runtime/
    cpp_core/transport/

## Dataset

The project uses a large local dataset for perception development and evaluation.

Current local dataset:

- Raw data: approximately **22 GB**
- Processed data: approximately **5 GB**
- Total: approximately **27 GB**
- Files: approximately **37,980**

Dataset directories include:

    datasets/raw/
    datasets/processed/
    datasets/video/
    datasets/my_images/
    datasets/test/
    datasets/train/
    datasets/validation/

The datasets are intentionally excluded from Git because of their size.

## Training

The perception training pipeline is implemented in:

    perception/train.py

Supporting components include:

    perception/dataset.py
    perception/losses.py
    perception/train_smoke_test.py

Training configuration:

    config/training.yaml

Current configured checkpoint:

    logs/checkpoints/experiment_02/best_model.pth

## Evaluation and Inference

### Evaluation

    perception/evaluate_test.py

The evaluation script uses:

    logs/checkpoints/experiment_02/best_model.pth

### Test Prediction

    perception/predict_test.py

The test prediction script uses the validated experiment-02 checkpoint and generates representative segmentation predictions.

### Full Prediction Pipeline

    perception/full_prediction.py

The full prediction pipeline uses the validated segmentation checkpoint and additional depth-processing components for the local perception workflow.

## Current Status

### Implemented / Validated

- DeepLabV3Plus semantic segmentation
- ResNet18 ImageNet-pretrained encoder
- Training and checkpoint handling
- Validation checkpoint with 74.11% best validation mIoU
- Representative inference using the validated checkpoint
- Apple MPS inference
- Traversability components
- Ground projection
- Metric local mapping
- C++ planning components
- C++ control components
- Safety command handling
- Data transport components
- Localization interfaces
- Localization pipeline structure
- Runtime interfaces
- C++ test targets

### Not Yet Fully Integrated

The following are not currently complete end-to-end integrations:

- Actual ORB-SLAM3 runtime connection to the UGV localization pipeline
- Physical UGV/chassis deployment
- Full field validation on the target UGV
- Hardware-specific integrations not represented by the current source tree

## Large Files and Git Policy

The following directories are excluded from Git:

    datasets/
    logs/
    build/
    venv/

The repository also excludes generated and compiled files such as:

    *.pth
    *.a
    *.so
    *.dylib
    __pycache__/
    *.pyc

Large datasets, model checkpoints, build artifacts, virtual environments, and generated logs should not be committed directly to Git history.

Source code, configuration, tests, and reproducibility-related scripts remain tracked.

## Development Environment

Development and validation has primarily been performed on:

- macOS
- Apple Silicon
- Python virtual environment
- Apple MPS acceleration where available
- C++ and Python components

## GitHub Repository

https://github.com/ravi15-12/SIH-Autonomous-UGV

## Development Roadmap

The current development direction is progressive integration of the individual subsystems into a complete autonomous UGV pipeline:

1. Perception
2. Traversability mapping
3. Localization
4. Planning
5. Control
6. Runtime and data transport
7. ORB-SLAM3 integration
8. Physical UGV deployment
9. Field validation

Implementation status will be updated as each subsystem is integrated and tested.
