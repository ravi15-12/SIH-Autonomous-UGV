from pathlib import Path
import random

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
        augment=False,
    ):
        self.image_dir = Path(image_dir)
        self.mask_dir = Path(mask_dir)

        self.width, self.height = image_size
        self.augment = augment

        self.image_files = sorted(self.image_dir.glob("*.jpg"))

        if not self.image_files:
            raise RuntimeError(
                f"No JPG images found in {self.image_dir}"
            )

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

    def apply_augmentation(self, image, mask):
        """
        Apply paired augmentations to image and mask.
        """

        # ----------------------------------------------------
        # Horizontal flip
        # ----------------------------------------------------
        if random.random() < 0.5:
            image = cv2.flip(image, 1)
            mask = cv2.flip(mask, 1)

        # ----------------------------------------------------
        # Small rotation
        # ----------------------------------------------------
        if random.random() < 0.30:
            angle = random.uniform(-7.0, 7.0)

            center = (
                image.shape[1] / 2,
                image.shape[0] / 2,
            )

            matrix = cv2.getRotationMatrix2D(
                center,
                angle,
                1.0,
            )

            image = cv2.warpAffine(
                image,
                matrix,
                (image.shape[1], image.shape[0]),
                flags=cv2.INTER_LINEAR,
                borderMode=cv2.BORDER_REFLECT_101,
            )

            mask = cv2.warpAffine(
                mask,
                matrix,
                (mask.shape[1], mask.shape[0]),
                flags=cv2.INTER_NEAREST,
                borderMode=cv2.BORDER_CONSTANT,
                borderValue=0,
            )

        # ----------------------------------------------------
        # Brightness / contrast
        # ----------------------------------------------------
        if random.random() < 0.40:
            alpha = random.uniform(0.85, 1.15)
            beta = random.uniform(-20, 20)

            image = cv2.convertScaleAbs(
                image,
                alpha=alpha,
                beta=beta,
            )

        # ----------------------------------------------------
        # Mild Gaussian blur
        # ----------------------------------------------------
        if random.random() < 0.15:
            image = cv2.GaussianBlur(
                image,
                (3, 3),
                0,
            )

        return image, mask

    def __getitem__(self, index):

        image_path, mask_path = self.samples[index]

        # ----------------------------------------------------
        # Load RGB image
        # ----------------------------------------------------
        image = cv2.imread(
            str(image_path),
            cv2.IMREAD_COLOR,
        )

        if image is None:
            raise RuntimeError(
                f"Could not read image: {image_path}"
            )

        image = cv2.cvtColor(
            image,
            cv2.COLOR_BGR2RGB,
        )

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

        mask = cv2.resize(
            mask,
            (self.width, self.height),
            interpolation=cv2.INTER_NEAREST,
        )

        # ----------------------------------------------------
        # Validate mask IDs
        # ----------------------------------------------------
        unique_ids = np.unique(mask)

        if not np.all(
            np.isin(unique_ids, [0, 1, 2, 3])
        ):
            raise RuntimeError(
                f"Invalid mask IDs {unique_ids} in {mask_path}"
            )

        # ----------------------------------------------------
        # Training augmentation
        # ----------------------------------------------------
        if self.augment:
            image, mask = self.apply_augmentation(
                image,
                mask,
            )

        # ----------------------------------------------------
        # Convert image to tensor
        # ----------------------------------------------------
        image = image.astype(np.float32) / 255.0

        mean = np.array(
            [0.485, 0.456, 0.406],
            dtype=np.float32,
        )

        std = np.array(
            [0.229, 0.224, 0.225],
            dtype=np.float32,
        )

        image = (image - mean) / std

        # HWC → CHW
        image = np.transpose(
            image,
            (2, 0, 1),
        )

        image = torch.from_numpy(image)

        # ----------------------------------------------------
        # Convert mask to tensor
        # ----------------------------------------------------
        mask = torch.from_numpy(
            mask.astype(np.int64)
        )

        return image, mask