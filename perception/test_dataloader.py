from pathlib import Path

import torch
from torch.utils.data import DataLoader

from dataset import RellisUGVDataset


PROJECT_ROOT = Path(__file__).resolve().parent.parent

IMAGE_DIR = PROJECT_ROOT / "datasets/processed/rellis_ugv/train/images"
MASK_DIR = PROJECT_ROOT / "datasets/processed/rellis_ugv/train/masks"


def main():
    dataset = RellisUGVDataset(
        image_dir=IMAGE_DIR,
        mask_dir=MASK_DIR,
        image_size=(640, 384),
    )

    loader = DataLoader(
        dataset,
        batch_size=4,
        shuffle=True,
        num_workers=0,
    )

    images, masks = next(iter(loader))

    print(f"Dataset size : {len(dataset)}")
    print(f"Image batch  : {tuple(images.shape)}")
    print(f"Mask batch   : {tuple(masks.shape)}")
    print(f"Image dtype  : {images.dtype}")
    print(f"Mask dtype   : {masks.dtype}")
    print(f"Mask IDs     : {sorted(masks.unique().tolist())}")

    device = torch.device(
        "mps" if torch.backends.mps.is_available() else "cpu"
    )

    images = images.to(device)

    print(f"Device       : {device}")
    print(f"Batch device : {images.device}")

    print("\nDataLoader test successful.")


if __name__ == "__main__":
    main()
