from pathlib import Path

import torch
import yaml
import segmentation_models_pytorch as smp
from torch.optim import AdamW
from torch.utils.data import DataLoader

from dataset import RellisUGVDataset
from losses import WeightedCrossEntropyDiceLoss


PROJECT_ROOT = Path(__file__).resolve().parent.parent

MODEL_CONFIG = PROJECT_ROOT / "config/model.yaml"
TRAIN_CONFIG = PROJECT_ROOT / "config/training.yaml"

IMAGE_DIR = (
    PROJECT_ROOT
    / "datasets/processed/rellis_ugv/train/images"
)

MASK_DIR = (
    PROJECT_ROOT
    / "datasets/processed/rellis_ugv/train/masks"
)


def main():

    # --------------------------------------------------------
    # Load configuration
    # --------------------------------------------------------
    with MODEL_CONFIG.open("r", encoding="utf-8") as f:
        model_config = yaml.safe_load(f)

    with TRAIN_CONFIG.open("r", encoding="utf-8") as f:
        train_config = yaml.safe_load(f)

    model_cfg = model_config["model"]
    training_cfg = train_config["training"]

    # --------------------------------------------------------
    # Reproducibility
    # --------------------------------------------------------
    torch.manual_seed(training_cfg["seed"])

    # --------------------------------------------------------
    # Device
    # --------------------------------------------------------
    device = torch.device(
        "mps"
        if torch.backends.mps.is_available()
        else "cpu"
    )

    # --------------------------------------------------------
    # Dataset
    # --------------------------------------------------------
    dataset = RellisUGVDataset(
        image_dir=IMAGE_DIR,
        mask_dir=MASK_DIR,
        image_size=(640, 384),
    )

    loader = DataLoader(
        dataset,
        batch_size=training_cfg["batch_size"],
        shuffle=True,
        num_workers=training_cfg["num_workers"],
    )

    # --------------------------------------------------------
    # Model
    # --------------------------------------------------------
    model = smp.DeepLabV3Plus(
        encoder_name=model_cfg["encoder"],
        encoder_weights=model_cfg["encoder_weights"],
        in_channels=3,
        classes=model_cfg["classes"],
        activation=model_cfg["activation"],
    )

    model = model.to(device)

    # IMPORTANT:
    # Smoke test uses training mode because we are testing
    # forward + backward + optimizer behavior.
    model.train()

    # --------------------------------------------------------
    # Loss
    # --------------------------------------------------------
    class_weights = [
        model_config["training"]["class_weights"]["traversable"],
        model_config["training"]["class_weights"]["non_traversable"],
        model_config["training"]["class_weights"]["obstacle"],
    ]

    loss_fn = WeightedCrossEntropyDiceLoss(
        class_weights=class_weights,
        ignore_index=model_config["training"]["ignore_index"],
        dice_weight=0.5,
    ).to(device)

    # --------------------------------------------------------
    # Optimizer
    # --------------------------------------------------------
    optimizer = AdamW(
        model.parameters(),
        lr=training_cfg["learning_rate"],
        weight_decay=training_cfg["weight_decay"],
    )

    # --------------------------------------------------------
    # Training smoke test
    # --------------------------------------------------------
    print("========================================")
    print("UGV TRAINING SMOKE TEST")
    print("========================================")
    print(f"Device      : {device}")
    print(f"Dataset     : {len(dataset)} images")
    print(f"Batch size  : {training_cfg['batch_size']}")
    print(f"Model       : {model_cfg['architecture']}")
    print(f"Encoder     : {model_cfg['encoder']}")
    print("")

    for batch_idx, (images, masks) in enumerate(loader):

        if batch_idx >= 2:
            break

        images = images.to(device)
        masks = masks.to(device)

        optimizer.zero_grad(set_to_none=True)

        # Forward pass
        logits = model(images)

        # Loss
        loss = loss_fn(logits, masks)

        # Backward pass
        loss.backward()

        # Optimizer step
        optimizer.step()

        print(
            f"Batch {batch_idx + 1}/2 | "
            f"Loss: {loss.item():.6f} | "
            f"Input: {tuple(images.shape)} | "
            f"Output: {tuple(logits.shape)}"
        )

    print("")
    print("Training smoke test successful.")


if __name__ == "__main__":
    main()
