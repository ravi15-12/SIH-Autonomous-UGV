import os
import yaml
import torch
import numpy as np
import cv2
import segmentation_models_pytorch as smp
from PIL import Image
from torchvision import transforms


# =========================
# Configuration
# =========================

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MODEL_CONFIG = os.path.join(PROJECT_ROOT, "config", "model.yaml")
CHECKPOINT = os.path.join(
    PROJECT_ROOT, "logs", "checkpoints", "best_model.pth"
)

TEST_IMAGES = os.path.join(
    PROJECT_ROOT, "datasets", "processed", "rellis_ugv", "test", "images"
)

OUTPUT_DIR = os.path.join(
    PROJECT_ROOT, "logs", "predictions"
)

os.makedirs(OUTPUT_DIR, exist_ok=True)


# =========================
# Device
# =========================

device = torch.device(
    "mps" if torch.backends.mps.is_available() else "cpu"
)

print(f"Device     : {device}")
print(f"Checkpoint : {CHECKPOINT}")


# =========================
# Load model configuration
# =========================

with open(MODEL_CONFIG, "r") as f:
    config = yaml.safe_load(f)

model_config = config["model"]

model = smp.DeepLabV3Plus(
    encoder_name=model_config["encoder"],
    encoder_weights=None,
    in_channels=3,
    classes=model_config["classes"],
    activation=None
)


# =========================
# Load trained weights
# =========================

checkpoint = torch.load(
    CHECKPOINT,
    map_location="cpu",
    weights_only=True
)

model.load_state_dict(checkpoint["model_state_dict"])

model.to(device)
model.eval()


# =========================
# Image preprocessing
# =========================

transform = transforms.Compose([
    transforms.Resize((512, 512)),
    transforms.ToTensor(),
    transforms.Normalize(
        mean=[0.485, 0.456, 0.406],
        std=[0.229, 0.224, 0.225]
    )
])


# =========================
# Prediction
# =========================

image_files = sorted([
    f for f in os.listdir(TEST_IMAGES)
    if f.lower().endswith((".jpg", ".jpeg", ".png"))
])

# Select a few images spread through the test set
num_samples = min(10, len(image_files))

indices = np.linspace(
    0,
    len(image_files) - 1,
    num_samples,
    dtype=int
)

selected_images = [image_files[i] for i in indices]


print(f"Test images available: {len(image_files)}")
print(f"Generating predictions: {len(selected_images)}")
print()


with torch.no_grad():

    for count, filename in enumerate(selected_images, 1):

        image_path = os.path.join(TEST_IMAGES, filename)

        image = Image.open(image_path).convert("RGB")

        original = np.array(image)

        input_tensor = transform(image).unsqueeze(0).to(device)

        output = model(input_tensor)

        prediction = torch.argmax(
            output,
            dim=1
        )[0].cpu().numpy()

        # Resize prediction back to original image size
        prediction = cv2.resize(
            prediction.astype(np.uint8),
            (original.shape[1], original.shape[0]),
            interpolation=cv2.INTER_NEAREST
        )

        # =====================================
        # Create visualization
        #
        # 0 = traversable
        # 1 = non-traversable
        # 2 = obstacle
        # =====================================

        visualization = np.zeros_like(original)
        
        visualization[prediction == 0] = [0, 255, 0]
        # Traversable → green
        visualization[prediction == 1] = [255, 255, 0]
        # Non-traversable → yellow
        visualization[prediction == 2] = [255, 0, 0]
        # Obstacle → red

        

        # Blend prediction with original image
        overlay = cv2.addWeighted(
            original,
            0.55,
            visualization,
            0.45,
            0
        )

        # Save
        output_path = os.path.join(
            OUTPUT_DIR,
            f"prediction_{count:02d}_{filename}"
        )

        Image.fromarray(overlay).save(output_path)

        print(
            f"[{count}/{num_samples}] "
            f"{filename} → {output_path}"
        )


print()
print("========================================")
print("PREDICTION GENERATION COMPLETE")
print("========================================")
print(f"Output directory: {OUTPUT_DIR}")
