from pathlib import Path
import cv2
import numpy as np

ROOT = Path("datasets/processed/rellis_ugv")

SPLITS = {
    "train": 3302,
    "validation": 983,
    "test": 1672,
}

CLASS_NAMES = {
    0: "unknown",
    1: "traversable",
    2: "non_traversable",
    3: "obstacle",
}

total_counts = np.zeros(4, dtype=np.int64)

print("========================================")
print("UGV 4-CLASS DISTRIBUTION")
print("========================================")

for split, expected in SPLITS.items():

    mask_dir = ROOT / split / "masks"
    masks = sorted(mask_dir.glob("*.png"))

    if len(masks) != expected:
        raise RuntimeError(
            f"{split}: expected {expected} masks, found {len(masks)}"
        )

    counts = np.zeros(4, dtype=np.int64)

    for i, mask_path in enumerate(masks, 1):

        mask = cv2.imread(
            str(mask_path),
            cv2.IMREAD_UNCHANGED
        )

        if mask is None:
            raise RuntimeError(
                f"Could not read: {mask_path}"
            )

        unique, pixels = np.unique(
            mask,
            return_counts=True
        )

        for class_id, count in zip(unique, pixels):

            class_id = int(class_id)

            if class_id not in CLASS_NAMES:
                raise RuntimeError(
                    f"Invalid class ID {class_id} in {mask_path}"
                )

            counts[class_id] += int(count)

        if i % 500 == 0 or i == len(masks):
            print(f"{split}: {i}/{len(masks)}")

    total_counts += counts

    total_pixels = counts.sum()

    print(f"\n===== {split.upper()} =====")
    print(f"Total pixels: {total_pixels:,}")

    for class_id in range(4):

        percentage = counts[class_id] / total_pixels * 100

        print(
            f"{class_id} - "
            f"{CLASS_NAMES[class_id]:16s}: "
            f"{counts[class_id]:15,} "
            f"({percentage:6.2f}%)"
        )

total_pixels = total_counts.sum()

print("\n========================================")
print("COMBINED DATASET")
print("========================================")
print(f"Total pixels: {total_pixels:,}")

for class_id in range(4):

    percentage = total_counts[class_id] / total_pixels * 100

    print(
        f"{class_id} - "
        f"{CLASS_NAMES[class_id]:16s}: "
        f"{total_counts[class_id]:15,} "
        f"({percentage:6.2f}%)"
    )

print("\nDistribution calculation successful.")
