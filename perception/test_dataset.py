from pathlib import Path

from dataset import RellisUGVDataset


PROJECT_ROOT = Path(__file__).resolve().parent.parent

IMAGE_DIR = (
    PROJECT_ROOT
    / "datasets/processed/rellis_ugv/train/images"
)

MASK_DIR = (
    PROJECT_ROOT
    / "datasets/processed/rellis_ugv/train/masks"
)


def main():
    dataset = RellisUGVDataset(
        image_dir=IMAGE_DIR,
        mask_dir=MASK_DIR,
        image_size=(640, 384),
    )

    print(f"Dataset size: {len(dataset)}")

    image, mask = dataset[0]

    print(f"Image shape: {tuple(image.shape)}")
    print(f"Image dtype: {image.dtype}")
    print(
        f"Image range: "
        f"{image.min().item():.4f} → {image.max().item():.4f}"
    )

    print(f"Mask shape: {tuple(mask.shape)}")
    print(f"Mask dtype: {mask.dtype}")
    print(f"Mask IDs: {sorted(mask.unique().tolist())}")


if __name__ == "__main__":
    main()
