from pathlib import Path
import sys

import torch
import yaml
import numpy as np
from torch.utils.data import DataLoader
import segmentation_models_pytorch as smp

PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))

from perception.dataset import RellisUGVDataset


def calculate_metrics(pred, target):
    """
    Dataset labels:
        0 = unknown
        1 = traversable
        2 = non-traversable
        3 = obstacle

    Model predictions:
        0 = traversable
        1 = non-traversable
        2 = obstacle
    """

    valid = target != 0

    target_model = target[valid] - 1
    pred_valid = pred[valid]

    num_classes = 3

    ious = []
    confusion = np.zeros((num_classes, num_classes), dtype=np.int64)

    for true_class in range(num_classes):
        for pred_class in range(num_classes):
            confusion[true_class, pred_class] = int(
                ((target_model == true_class) & (pred_valid == pred_class)).sum()
            )

    for c in range(num_classes):
        tp = confusion[c, c]
        fp = confusion[:, c].sum() - tp
        fn = confusion[c, :].sum() - tp

        denominator = tp + fp + fn

        if denominator == 0:
            iou = float("nan")
        else:
            iou = tp / denominator

        ious.append(iou)

    pixel_accuracy = (
        (pred_valid == target_model).sum().item() / len(target_model)
        if len(target_model) > 0
        else 0.0
    )

    # Hazard = non-traversable + obstacle
    hazard_true = (target_model == 1) | (target_model == 2)
    hazard_pred = (pred_valid == 1) | (pred_valid == 2)

    hazard_recall_denominator = hazard_true.sum().item()

    if hazard_recall_denominator > 0:
        hazard_recall = (
            (hazard_true & hazard_pred).sum().item()
            / hazard_recall_denominator
        )
    else:
        hazard_recall = 0.0

    return ious, pixel_accuracy, hazard_recall, confusion


def main():
    model_config_path = PROJECT_ROOT / "config" / "model.yaml"
    training_config_path = PROJECT_ROOT / "config" / "training.yaml"
    checkpoint_path = (
        PROJECT_ROOT / "logs" / "checkpoints" / "experiment_02" / "best_model.pth"
    )

    with open(model_config_path, "r") as f:
        model_config = yaml.safe_load(f)

    with open(training_config_path, "r") as f:
        training_config = yaml.safe_load(f)

    device = torch.device(
        "mps" if torch.backends.mps.is_available() else "cpu"
    )

    dataset = RellisUGVDataset(
        PROJECT_ROOT / "datasets" / "processed" / "rellis_ugv" / "test" / "images",
        PROJECT_ROOT / "datasets" / "processed" / "rellis_ugv" / "test" / "masks",
    )

    loader = DataLoader(
        dataset,
        batch_size=4,
        shuffle=False,
        num_workers=0,
    )

    model = smp.DeepLabV3Plus(
        encoder_name=model_config["model"]["encoder"],
        encoder_weights=None,
        in_channels=3,
        classes=model_config["model"]["classes"],
        activation=model_config["model"]["activation"],
    )

    checkpoint = torch.load(
        checkpoint_path,
        map_location="cpu",
        weights_only=True,
    )

    model.load_state_dict(checkpoint["model_state_dict"])
    model.to(device)
    model.eval()

    print("========================================")
    print("RELLIS-3D TEST SET EVALUATION")
    print("========================================")
    print(f"Device     : {device}")
    print(f"Test images: {len(dataset)}")
    print(f"Checkpoint : {checkpoint_path}")
    print()

    total_confusion = np.zeros((3, 3), dtype=np.int64)
    total_correct = 0
    total_valid = 0
    total_hazard_true = 0
    total_hazard_correct = 0

    with torch.no_grad():
        for batch_idx, (images, masks) in enumerate(loader, start=1):
            images = images.to(device)
            masks = masks.to(device)

            outputs = model(images)
            predictions = torch.argmax(outputs, dim=1)

            valid = masks != 0

            target_model = masks[valid] - 1
            pred_valid = predictions[valid]

            total_correct += (pred_valid == target_model).sum().item()
            total_valid += target_model.numel()

            hazard_true = (target_model == 1) | (target_model == 2)
            hazard_pred = (pred_valid == 1) | (pred_valid == 2)

            total_hazard_true += hazard_true.sum().item()
            total_hazard_correct += (
                (hazard_true & hazard_pred).sum().item()
            )

            for true_class in range(3):
                for pred_class in range(3):
                    total_confusion[true_class, pred_class] += int(
                        (
                            (target_model == true_class)
                            & (pred_valid == pred_class)
                        ).sum().item()
                    )

            if batch_idx % 50 == 0 or batch_idx == len(loader):
                print(
                    f"Processed {batch_idx}/{len(loader)} batches",
                    flush=True,
                )

    ious = []

    for c in range(3):
        tp = total_confusion[c, c]
        fp = total_confusion[:, c].sum() - tp
        fn = total_confusion[c, :].sum() - tp

        denominator = tp + fp + fn

        if denominator == 0:
            iou = float("nan")
        else:
            iou = tp / denominator

        ious.append(iou)

    miou = float(np.nanmean(ious))
    pixel_accuracy = total_correct / total_valid
    hazard_recall = (
        total_hazard_correct / total_hazard_true
        if total_hazard_true > 0
        else 0.0
    )

    print()
    print("========================================")
    print("FINAL TEST RESULTS")
    print("========================================")
    print(f"Test mIoU          : {miou:.4f} ({miou * 100:.2f}%)")
    print(f"Traversable IoU    : {ious[0]:.4f} ({ious[0] * 100:.2f}%)")
    print(
        f"Non-traversable IoU: {ious[1]:.4f} ({ious[1] * 100:.2f}%)"
    )
    print(f"Obstacle IoU       : {ious[2]:.4f} ({ious[2] * 100:.2f}%)")
    print(
        f"Pixel Accuracy     : {pixel_accuracy:.4f} "
        f"({pixel_accuracy * 100:.2f}%)"
    )
    print(
        f"Hazard Recall      : {hazard_recall:.4f} "
        f"({hazard_recall * 100:.2f}%)"
    )

    print()
    print("Confusion Matrix")
    print("(rows = true, columns = predicted)")
    print("              Trav   NonTrav   Obstacle")
    print(
        f"Trav         {total_confusion[0,0]:7d}"
        f"{total_confusion[0,1]:10d}"
        f"{total_confusion[0,2]:10d}"
    )
    print(
        f"NonTrav      {total_confusion[1,0]:7d}"
        f"{total_confusion[1,1]:10d}"
        f"{total_confusion[1,2]:10d}"
    )
    print(
        f"Obstacle     {total_confusion[2,0]:7d}"
        f"{total_confusion[2,1]:10d}"
        f"{total_confusion[2,2]:10d}"
    )

    print("========================================")


if __name__ == "__main__":
    main()
