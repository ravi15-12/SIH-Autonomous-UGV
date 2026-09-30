from pathlib import Path
import yaml
import torch
import segmentation_models_pytorch as smp


PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONFIG_FILE = PROJECT_ROOT / "config/model.yaml"


def main():
    with CONFIG_FILE.open("r", encoding="utf-8") as f:
        config = yaml.safe_load(f)

    model_cfg = config["model"]

    model = smp.DeepLabV3Plus(
        encoder_name=model_cfg["encoder"],
        encoder_weights=model_cfg["encoder_weights"],
        in_channels=3,
        classes=model_cfg["classes"],
        activation=model_cfg["activation"],
    )

    device = torch.device(
        "mps" if torch.backends.mps.is_available() else "cpu"
    )

    model = model.to(device)
    model.eval()

    print("Model initialized successfully.")
    print(f"Architecture : {model_cfg['architecture']}")
    print(f"Encoder      : {model_cfg['encoder']}")
    print(f"Classes      : {model_cfg['classes']}")
    print(f"Device       : {device}")

    # Test a single forward pass.
    test_input = torch.randn(1, 3, 384, 640, device=device)

    with torch.no_grad():
        output = model(test_input)

    print(f"Input shape  : {tuple(test_input.shape)}")
    print(f"Output shape : {tuple(output.shape)}")


if __name__ == "__main__":
    main()
