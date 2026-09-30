import torch

from losses import WeightedCrossEntropyDiceLoss


def main():
    device = torch.device(
        "mps" if torch.backends.mps.is_available() else "cpu"
    )

    # Small synthetic batch for testing only.
    logits = torch.randn(
        2, 3, 32, 32,
        device=device,
        requires_grad=True,
    )

    # Valid target IDs:
    # 0 = unknown
    # 1 = traversable
    # 2 = non-traversable
    # 3 = obstacle
    target = torch.randint(
        0,
        4,
        (2, 32, 32),
        device=device,
        dtype=torch.long,
    )

    loss_fn = WeightedCrossEntropyDiceLoss(
        class_weights=[
            0.9104,
            8.3293,
            1.0000,
        ],
        ignore_index=0,
        dice_weight=0.5,
    ).to(device)

    loss = loss_fn(logits, target)

    loss.backward()

    print(f"Device: {device}")
    print(f"Loss: {loss.item():.6f}")
    print(f"Gradient exists: {logits.grad is not None}")
    print(
        f"Gradient finite: "
        f"{torch.isfinite(logits.grad).all().item()}"
    )

    print("\nLoss test successful.")


if __name__ == "__main__":
    main()
