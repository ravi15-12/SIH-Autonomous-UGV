from pathlib import Path
import yaml
import cv2
import numpy as np

PROJECT_ROOT = Path(__file__).resolve().parent.parent

RELLIS_MAP_FILE = PROJECT_ROOT / "config" / "rellis_label_map.yaml"
UGV_MAP_FILE = PROJECT_ROOT / "config" / "ugv_rellis_mapping.yaml"

INPUT_LABEL = (
    PROJECT_ROOT
    / "datasets/raw/rellis3d/Rellis-3D/00000/"
    / "pylon_camera_node_label_id/frame000308-1581624683_550.png"
)

OUTPUT_MASK = PROJECT_ROOT / "logs/sample_ugv_mask.png"


# UGV output IDs
UGV_IDS = {
    "unknown": 0,
    "traversable": 1,
    "non_traversable": 2,
    "obstacle": 3,
}


def load_yaml(path):
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def main():
    rellis_data = load_yaml(RELLIS_MAP_FILE)
    ugv_data = load_yaml(UGV_MAP_FILE)

    rellis_labels = rellis_data["labels"]
    mapping = ugv_data["mapping"]

    # Convert class name -> UGV class ID
    class_to_ugv_id = {}

    for category, class_names in mapping.items():
        for class_name in class_names:
            class_to_ugv_id[class_name] = UGV_IDS[category]

    label = cv2.imread(str(INPUT_LABEL), cv2.IMREAD_UNCHANGED)

    if label is None:
        raise RuntimeError(f"Could not read label: {INPUT_LABEL}")

    output = np.zeros(label.shape, dtype=np.uint8)

    for raw_id, class_name in rellis_labels.items():
        if class_name not in class_to_ugv_id:
            raise RuntimeError(
                f"Class '{class_name}' has no UGV mapping."
            )

        output[label == int(raw_id)] = class_to_ugv_id[class_name]

    OUTPUT_MASK.parent.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(OUTPUT_MASK), output)

    print("Conversion successful.")
    print(f"Input:  {INPUT_LABEL}")
    print(f"Output: {OUTPUT_MASK}")
    print("\nUGV mask IDs:")

    for ugv_id, count in zip(*np.unique(output, return_counts=True)):
        name = next(
            name for name, value in UGV_IDS.items()
            if value == int(ugv_id)
        )
        print(f"  {ugv_id} -> {name}: {count:,} pixels")


if __name__ == "__main__":
    main()
