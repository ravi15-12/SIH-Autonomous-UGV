from pathlib import Path

import cv2
import numpy as np
import torch
from torch.utils.data import Dataset


class RellisUGVDataset(Dataset):
    """
    RELLIS-3D dataset for UGV perception.

    Mask IDs:
        0 = unknown / ignored
        1 = traversable
        2 = non-traversable
        3 = obstacle
    """

    def __init__(
        self,
        image_dir,
        mask_dir,
        image_size=(640, 384),
    ):
        self.image_dir = Path(image_dir)
        self.mask_dir = Path(mask_dir)

        self.width, self.height = image_size

        self.image_files = sorted(self.image_dir.glob("*.jpg"))

        if not self.image_files:
            raise RuntimeError(
                f"No JPG images found in {self.image_dir}"
            )

        # Verify every image has a corresponding mask.
        self.samples = []

        for image_path in self.image_files:
            mask_path = self.mask_dir / f"{image_path.stem}.png"

            if not mask_path.exists():
                raise RuntimeError(
                    f"Missing mask for image: {image_path.name}"
                )

            self.samples.append((image_path, mask_path))

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, index):
        image_path, mask_path = self.samples[index]

        # ----------------------------------------------------
        # Load RGB image
        # ----------------------------------------------------
        image = cv2.imread(str(image_path), cv2.IMREAD_COLOR)

        if image is None:
            raise RuntimeError(
                f"Could not read image: {image_path}"
            )

        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

        # ----------------------------------------------------
        # Load segmentation mask
        # ----------------------------------------------------
        mask = cv2.imread(
            str(mask_path),
            cv2.IMREAD_GRAYSCALE,
        )

        if mask is None:
            raise RuntimeError(
                f"Could not read mask: {mask_path}"
            )

        # ----------------------------------------------------
        # Resize
        # ----------------------------------------------------
        image = cv2.resize(
            image,
            (self.width, self.height),
            interpolation=cv2.INTER_LINEAR,
        )

        # IMPORTANT:
        # Segmentation masks MUST use nearest-neighbor.
        mask = cv2.resize(
            mask,
            (self.width, self.height),
            interpolation=cv2.INTER_NEAREST,
        )

        # ----------------------------------------------------
        # Validate mask IDs
        # ----------------------------------------------------
        unique_ids = np.unique(mask)

        if not np.all(np.isin(unique_ids, [0, 1, 2, 3])):
            raise RuntimeError(
                f"Invalid mask IDs {unique_ids} in {mask_path}"
            )

        # ----------------------------------------------------
        # Convert image to tensor
        # ----------------------------------------------------
        image = image.astype(np.float32) / 255.0
        # ImageNet normalization for pretrained ResNet encoder.
        mean=np.array(
            [0.485,0.456,0.406],
            dtype=np.float32,
        )
        std=np.array(
            [0.229,0.224,0.225],
            dtype=np.float32,
        )
        image=(image-mean)/std
        # HWC → CHW
        image = np.transpose(image, (2, 0, 1))

        image = torch.from_numpy(image)

        # ----------------------------------------------------
        # Convert mask to tensor
        # ----------------------------------------------------
        mask = torch.from_numpy(
            mask.astype(np.int64)
        )

        return image, mask
