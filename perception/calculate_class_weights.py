from pathlib import Path
import cv2
import numpy as np

PROJECT_ROOT = Path(__file__).resolve().parent.parent
MASK_DIR = PROJECT_ROOT / "datasets/processed/rellis_ugv/train/masks"

counts = np.zeros(4, dtype=np.int64)

mask_files = sorted(MASK_DIR.glob("*.png"))

print(f"Found {len(mask_files)} training masks.")

for i, mask_path in enumerate(mask_files, 1):
    mask = cv2.imread(str(mask_path), cv2.IMREAD_GRAYSCALE)

    if mask is None:
        raise RuntimeError(f"Could not read: {mask_path}")

    values, pixel_counts = np.unique(mask, return_counts=True)

    for value, count in zip(values, pixel_counts):
        if value > 3:
            raise RuntimeError(
                f"Invalid class ID {value} in {mask_path}"
            )
        counts[value] += count

    if i % 500 == 0:
        print(f"Processed {i}/{len(mask_files)}")

print("\nTraining pixel distribution:")

names = [
    "unknown (ignored)",
    "traversable",
    "non_traversable",
    "obstacle",
]

for class_id, (name, count) in enumerate(zip(names, counts)):
    print(
        f"{class_id} - {name:20s}: "
        f"{count:15,} "
        f"({count / counts.sum() * 100:6.2f}%)"
    )

# Ignore class 0 when calculating weights.
learnable_counts = counts[1:]
total = learnable_counts.sum()
num_classes = len(learnable_counts)

# Median-frequency balancing.
frequencies = learnable_counts / total
median_frequency = np.median(frequencies)

weights = median_frequency / frequencies

print("\nRecommended training weights:")

for class_id, weight in zip(range(1, 4), weights):
    print(f"{class_id} - {names[class_id]:20s}: {weight:.4f}")

print("\nDone.")
