from pathlib import Path
import random

import cv2
import numpy as np
import torch
import yaml
import segmentation_models_pytorch as smp

from torch.optim import AdamW
from torch.optim.lr_scheduler import ReduceLROnPlateau
from torch.utils.data import DataLoader
from tqdm import tqdm

from dataset import RellisUGVDataset
from losses import WeightedCrossEntropyDiceLoss


PROJECT_ROOT = Path(__file__).resolve().parent.parent

MODEL_CONFIG = PROJECT_ROOT / "config/model.yaml"
TRAIN_CONFIG = PROJECT_ROOT / "config/training.yaml"

TRAIN_IMAGES = PROJECT_ROOT / "datasets/processed/rellis_ugv/train/images"
TRAIN_MASKS = PROJECT_ROOT / "datasets/processed/rellis_ugv/train/masks"

VAL_IMAGES = PROJECT_ROOT / "datasets/processed/rellis_ugv/validation/images"
VAL_MASKS = PROJECT_ROOT / "datasets/processed/rellis_ugv/validation/masks"


def set_seed(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)


def calculate_metrics(logits, target, num_classes=3):
    """
    Calculate confusion matrix and segmentation metrics.

    Dataset IDs:
        0 = ignored
        1 = traversable
        2 = non-traversable
        3 = obstacle

    Model IDs:
        0 = traversable
        1 = non-traversable
        2 = obstacle
    """

    prediction = torch.argmax(logits, dim=1)

    valid = target != 0

    # Convert dataset IDs 1,2,3 → model IDs 0,1,2.
    target_model = target - 1

    prediction = prediction[valid].flatten()
    target_model = target_model[valid].flatten()

    confusion = torch.zeros(
        (num_classes, num_classes),
        dtype=torch.int64,
    )

    for true_class in range(num_classes):
        for predicted_class in range(num_classes):
            confusion[true_class, predicted_class] = (
                (
                    (target_model == true_class)
                    & (prediction == predicted_class)
                )
                .sum()
                .cpu()
            )

    return confusion


def metrics_from_confusion(confusion):
    intersection = torch.diag(confusion).float()

    actual = confusion.sum(dim=1).float()
    predicted = confusion.sum(dim=0).float()

    union = actual + predicted - intersection

    iou = intersection / torch.clamp(union, min=1)

    mean_iou = iou.mean().item()

    total_correct = intersection.sum().item()
    total_pixels = confusion.sum().item()

    pixel_accuracy = (
        total_correct / total_pixels
        if total_pixels > 0
        else 0.0
    )

    # Hazard = non-traversable + obstacle.
    hazard_true = actual[1] + actual[2]

    hazard_missed = (
        confusion[1, 0]
        + confusion[2, 0]
    )

    hazard_recall = (
        (hazard_true - hazard_missed) / hazard_true
        if hazard_true > 0
        else 0.0
    )

    return {
        "pixel_accuracy": pixel_accuracy,
        "mean_iou": mean_iou,
        "traversable_iou": iou[0].item(),
        "non_traversable_iou": iou[1].item(),
        "obstacle_iou": iou[2].item(),
        "hazard_recall": hazard_recall,
    }


def run_epoch(
    model,
    loader,
    loss_fn,
    device,
    optimizer=None,
):
    training = optimizer is not None

    if training:
        model.train()
    else:
        model.eval()

    total_loss = 0.0
    num_batches = 0

    confusion = torch.zeros(
        (3, 3),
        dtype=torch.int64,
    )

    for images, masks in tqdm(loader, desc="Training" if training else "Validation", leave=False):

        images = images.to(device)
        masks = masks.to(device)

        if training:
            optimizer.zero_grad(set_to_none=True)

        with torch.set_grad_enabled(training):
            logits = model(images)

            loss = loss_fn(
                logits,
                masks,
            )

            if training:
                loss.backward()
                optimizer.step()

        total_loss += loss.item()
        num_batches += 1

        confusion += calculate_metrics(
            logits.detach(),
            masks,
        )

    average_loss = total_loss / max(num_batches, 1)

    metrics = metrics_from_confusion(confusion)

    metrics["loss"] = average_loss

    return metrics


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

    set_seed(training_cfg["seed"])

    # --------------------------------------------------------
    # Device
    # --------------------------------------------------------
    device = torch.device(
        "mps"
        if torch.backends.mps.is_available()
        else "cpu"
    )

    # --------------------------------------------------------
    # Datasets
    # --------------------------------------------------------
    train_dataset = RellisUGVDataset(
        image_dir=TRAIN_IMAGES,
        mask_dir=TRAIN_MASKS,
        image_size=(640, 384),
        augment=training_cfg["augmentation"]["enabled"],
    )

    val_dataset = RellisUGVDataset(
        image_dir=VAL_IMAGES,
        mask_dir=VAL_MASKS,
        image_size=(640, 384),
    )

    train_loader = DataLoader(
        train_dataset,
        batch_size=training_cfg["batch_size"],
        shuffle=True,
        num_workers=training_cfg["num_workers"],
    )

    val_loader = DataLoader(
        val_dataset,
        batch_size=training_cfg["batch_size"],
        shuffle=False,
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
    # Scheduler
    # --------------------------------------------------------
    scheduler_cfg = training_cfg["scheduler"]

    scheduler = ReduceLROnPlateau(
        optimizer,
        mode="min",
        factor=scheduler_cfg["factor"],
        patience=scheduler_cfg["patience"],
        min_lr=scheduler_cfg["min_lr"],
    )

    # --------------------------------------------------------
    # Checkpoint
    # --------------------------------------------------------
    checkpoint_cfg = training_cfg["checkpoint"]

    checkpoint_dir = (
        PROJECT_ROOT
        / checkpoint_cfg["directory"]
    )

    checkpoint_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    checkpoint_path = (
        checkpoint_dir
        / checkpoint_cfg["filename"]
    )

    # --------------------------------------------------------
    # Training
    # --------------------------------------------------------
    print("========================================")
    print("UGV PERCEPTION TRAINING")
    print("========================================")
    print(f"Device          : {device}")
    print(f"Training images : {len(train_dataset)}")
    print(f"Validation      : {len(val_dataset)}")
    print(f"Batch size      : {training_cfg['batch_size']}")
    print(f"Epochs          : {training_cfg['epochs']}")
    print(f"Learning rate   : {training_cfg['learning_rate']}")
    print(f"Model           : {model_cfg['architecture']}")
    print(f"Encoder         : {model_cfg['encoder']}")
    print("")

    best_val_loss = float("inf")
    epochs_without_improvement = 0

    for epoch in range(
        1,
        training_cfg["epochs"] + 1,
    ):

        train_metrics = run_epoch(
            model=model,
            loader=train_loader,
            loss_fn=loss_fn,
            device=device,
            optimizer=optimizer,
        )

        val_metrics = run_epoch(
            model=model,
            loader=val_loader,
            loss_fn=loss_fn,
            device=device,
        )

        scheduler.step(val_metrics["loss"])

        current_lr = optimizer.param_groups[0]["lr"]

        print(
            f"Epoch {epoch:02d}/{training_cfg['epochs']} | "
            f"LR {current_lr:.2e}"
        )

        print(
            f"  Train Loss: {train_metrics['loss']:.4f} | "
            f"Train mIoU: {train_metrics['mean_iou']:.4f}"
        )

        print(
            f"  Val Loss:   {val_metrics['loss']:.4f} | "
            f"Val mIoU:   {val_metrics['mean_iou']:.4f}"
        )

        print(
            f"  Val IoU → "
            f"Trav: {val_metrics['traversable_iou']:.4f} | "
            f"NonTrav: {val_metrics['non_traversable_iou']:.4f} | "
            f"Obstacle: {val_metrics['obstacle_iou']:.4f}"
        )

        print(
            f"  Val Pixel Accuracy: "
            f"{val_metrics['pixel_accuracy']:.4f} | "
            f"Hazard Recall: "
            f"{val_metrics['hazard_recall']:.4f}"
        )

        # ----------------------------------------------------
        # Save best checkpoint
        # ----------------------------------------------------
        if val_metrics["loss"] < best_val_loss:

            best_val_loss = val_metrics["loss"]
            epochs_without_improvement = 0

            torch.save(
                {
                    "epoch": epoch,
                    "model_state_dict": model.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                    "best_val_loss": best_val_loss,
                    "model_config": model_cfg,
                },
                checkpoint_path,
            )

            print(
                f"  ✓ Best model saved → "
                f"{checkpoint_path}"
            )

        else:
            epochs_without_improvement += 1

        # ----------------------------------------------------
        # Early stopping
        # ----------------------------------------------------
        early_cfg = training_cfg["early_stopping"]

        if (
            early_cfg["enabled"]
            and epochs_without_improvement
            >= early_cfg["patience"]
        ):
            print("")
            print("Early stopping triggered.")
            break

        print("")

    print("========================================")
    print("TRAINING COMPLETE")
    print("========================================")
    print(f"Best validation loss: {best_val_loss:.4f}")
    print(f"Best checkpoint: {checkpoint_path}")


if __name__ == "__main__":
    main()
