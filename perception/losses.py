import torch
import torch.nn as nn
import torch.nn.functional as F


class WeightedCrossEntropyDiceLoss(nn.Module):
    """
    Combined loss for UGV semantic segmentation.

    Classes:
        0 = unknown / ignored
        1 = traversable
        2 = non-traversable
        3 = obstacle

    The model predicts only classes 1-3.
    Mask value 0 is ignored.
    """

    def __init__(
        self,
        class_weights,
        ignore_index=0,
        dice_weight=0.5,
    ):
        super().__init__()

        self.ignore_index = ignore_index
        self.dice_weight = dice_weight

        self.register_buffer(
            "class_weights",
            torch.tensor(
                class_weights,
                dtype=torch.float32,
            ),
        )

    def forward(self, logits, target):
        # ----------------------------------------------------
        # Convert target IDs:
        #
        # 0 = ignored
        # 1 → model class 0
        # 2 → model class 1
        # 3 → model class 2
        # ----------------------------------------------------
        ce_target = target - 1

        valid = target != self.ignore_index

        # Weighted Cross Entropy.
        ce_loss = F.cross_entropy(
            logits,
            ce_target,
            weight=self.class_weights,
            ignore_index=-1,
        )

        # ----------------------------------------------------
        # Dice Loss
        # ----------------------------------------------------
        probabilities = torch.softmax(logits, dim=1)

        dice_losses = []

        for class_idx in range(3):
            target_class = (
                (target == (class_idx + 1))
                & valid
            ).float()

            prediction_class = probabilities[:, class_idx]

            prediction_class = prediction_class * valid.float()
            target_class = target_class * valid.float()

            intersection = (
                prediction_class * target_class
            ).sum()

            denominator = (
                prediction_class.sum()
                + target_class.sum()
            )

            dice = (
                (2.0 * intersection + 1e-6)
                / (denominator + 1e-6)
            )

            dice_losses.append(1.0 - dice)

        dice_loss = torch.stack(dice_losses).mean()

        return ce_loss + self.dice_weight * dice_loss
