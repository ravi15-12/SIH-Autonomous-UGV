from pathlib import Path
import shutil
import yaml
import cv2
import numpy as np

PROJECT_ROOT = Path(__file__).resolve().parent.parent

RELLIS_ROOT = PROJECT_ROOT / "datasets/raw/rellis3d/Rellis-3D"
SPLIT_ROOT = PROJECT_ROOT / "datasets/raw/rellis3d"
OUTPUT_ROOT = PROJECT_ROOT / "datasets/processed/rellis_ugv"

RELLIS_MAP_FILE = PROJECT_ROOT / "config/rellis_label_map.yaml"
UGV_MAP_FILE = PROJECT_ROOT / "config/ugv_rellis_mapping.yaml"

UGV_IDS = {
    "unknown": 0,
    "traversable": 1,
    "non_traversable": 2,
    "obstacle": 3,
}


def load_yaml(path):
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def build_class_mapping():
    rellis_data = load_yaml(RELLIS_MAP_FILE)
    ugv_data = load_yaml(UGV_MAP_FILE)

    rellis_labels = rellis_data["labels"]
    ugv_mapping = ugv_data["mapping"]

    class_to_ugv = {}

    for category, class_names in ugv_mapping.items():
        for class_name in class_names:
            class_to_ugv[class_name] = UGV_IDS[category]

    missing = [
        name for name in rellis_labels.values()
        if name not in class_to_ugv
    ]

    if missing:
        raise RuntimeError(
            f"Missing UGV mappings for: {sorted(set(missing))}"
        )

    return rellis_labels, class_to_ugv


def convert_mask(input_path, output_path, rellis_labels, class_to_ugv):
    label = cv2.imread(str(input_path), cv2.IMREAD_UNCHANGED)

    if label is None:
        raise RuntimeError(f"Could not read label: {input_path}")

    output = np.zeros(label.shape, dtype=np.uint8)

    for raw_id, class_name in rellis_labels.items():
        output[label == int(raw_id)] = class_to_ugv[class_name]

    # Verify that the generated mask contains only valid UGV IDs.
    unique_ids = set(np.unique(output).tolist())

    if not unique_ids.issubset(set(UGV_IDS.values())):
        raise RuntimeError(
            f"Invalid UGV mask IDs found: {unique_ids}"
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)

    if not cv2.imwrite(str(output_path), output):
        raise RuntimeError(f"Could not write mask: {output_path}")


def process_split(split_name, rellis_labels, class_to_ugv):
    split_file = SPLIT_ROOT / f"{split_name}.lst"

    if not split_file.exists():
        raise FileNotFoundError(f"Split file not found: {split_file}")

    output_split = "validation" if split_name == "val" else split_name

    image_output = OUTPUT_ROOT / output_split / "images"
    mask_output = OUTPUT_ROOT / output_split / "masks"

    # Safety: never overwrite an existing processed split.
    if image_output.exists() and any(image_output.iterdir()):
        raise RuntimeError(
            f"Output already contains images: {image_output}\n"
            "Delete the processed split manually only if you are sure "
            "it is safe to rebuild."
        )

    if mask_output.exists() and any(mask_output.iterdir()):
        raise RuntimeError(
            f"Output already contains masks: {mask_output}\n"
            "Delete the processed split manually only if you are sure "
            "it is safe to rebuild."
        )

    entries = []

    with split_file.open("r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split()

            if len(parts) == 2:
                entries.append(parts)

    print(f"\n===== {split_name.upper()} =====")
    print(f"Samples: {len(entries)}")

    processed = 0
    failures = []

    for i, (image_rel, label_rel) in enumerate(entries, 1):

        image_path = RELLIS_ROOT / image_rel
        label_path = RELLIS_ROOT / label_rel

        image_dest = image_output / Path(image_rel).name
        mask_dest = mask_output / Path(label_rel).name

        try:
            if not image_path.exists():
                raise FileNotFoundError(
                    f"Missing image: {image_path}"
                )

            if not label_path.exists():
                raise FileNotFoundError(
                    f"Missing label: {label_path}"
                )

            image = cv2.imread(
                str(image_path),
                cv2.IMREAD_COLOR
            )

            if image is None:
                raise RuntimeError(
                    f"Could not decode image: {image_path}"
                )

            label = cv2.imread(
                str(label_path),
                cv2.IMREAD_UNCHANGED
            )

            if label is None:
                raise RuntimeError(
                    f"Could not decode label: {label_path}"
                )

            if image.shape[:2] != label.shape[:2]:
                raise RuntimeError(
                    f"Dimension mismatch: "
                    f"{image.shape[:2]} vs {label.shape[:2]}"
                )

            image_output.mkdir(parents=True, exist_ok=True)

            shutil.copy2(
                image_path,
                image_dest
            )

            convert_mask(
                label_path,
                mask_dest,
                rellis_labels,
                class_to_ugv
            )

            processed += 1

        except Exception as exc:
            failures.append(
                f"{image_rel} -> {exc}"
            )

        if i % 250 == 0 or i == len(entries):
            print(
                f"Processed {i}/{len(entries)} "
                f"| successful={processed} "
                f"| failures={len(failures)}"
            )

    print(f"Successful: {processed}")
    print(f"Failures:   {len(failures)}")

    if failures:
        print("\nFirst failures:")

        for failure in failures[:10]:
            print(f"  - {failure}")

    return processed, failures


def main():

    print("Building RELLIS → UGV dataset")

    rellis_labels, class_to_ugv = build_class_mapping()

    print(
        f"RELLIS classes: {len(rellis_labels)}"
    )

    print(
        f"UGV mapped classes: "
        f"{len(class_to_ugv)}"
    )

    total_processed = 0
    total_failures = 0

    # TRAIN and VALIDATION are already complete.
    # Process TEST only in this step.
    processed, failures = process_split(
        "test",
        rellis_labels,
        class_to_ugv
    )

    total_processed += processed
    total_failures += len(failures)


    print("\n========================================")
    print("TEST DATASET BUILD COMPLETE")
    print("========================================")
    print(f"Successful: {total_processed}")
    print(f"Failures:   {total_failures}")
    print(f"Output: {OUTPUT_ROOT / 'test'}")

    if total_failures:
        raise RuntimeError(
            "Training dataset contains failures. "
            "Do not continue until they are investigated."
        )


if __name__ == "__main__":
    main()
