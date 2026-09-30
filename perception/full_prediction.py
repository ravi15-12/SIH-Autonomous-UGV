import os
import cv2
import numpy as np
import torch

from PIL import Image
from transformers import pipeline
import segmentation_models_pytorch as smp
from mapping.traversability import calculate_ground_roughness


# ============================================================
# PATHS
# ============================================================

PROJECT_ROOT = os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))
)

INPUT_DIR = os.path.join(
    PROJECT_ROOT,
    "datasets",
    "my_images"
)

OUTPUT_DIR = os.path.join(
    PROJECT_ROOT,
    "logs",
    "full_predictions"
)

CHECKPOINT_PATH = os.path.join(
    PROJECT_ROOT,
    "logs",
    "checkpoints",
    "best_model.pth"
)


# ============================================================
# SETTINGS
# ============================================================

IMAGE_SIZE = 512

# Existing model:
# 0 = traversable
# 1 = non-traversable
# 2 = obstacle

SEGMENTATION_CLASSES = 3

DEPTH_MODEL = (
    "depth-anything/"
    "Depth-Anything-V2-Small-hf"
)


# ============================================================
# DEVICE
# ============================================================

if torch.backends.mps.is_available():
    DEVICE = torch.device("mps")
elif torch.cuda.is_available():
    DEVICE = torch.device("cuda")
else:
    DEVICE = torch.device("cpu")


print()
print("=" * 70)
print("UGV PERCEPTION + DEPTH + TRAVERSABILITY PIPELINE")
print("=" * 70)
print(f"Device: {DEVICE}")
print(f"Input : {INPUT_DIR}")
print(f"Output: {OUTPUT_DIR}")
print()


# ============================================================
# OUTPUT DIRECTORY
# ============================================================

os.makedirs(
    OUTPUT_DIR,
    exist_ok=True
)


# ============================================================
# LOAD SEGMENTATION MODEL
# ============================================================

print("Loading segmentation model...")

segmentation_model = smp.DeepLabV3Plus(
    encoder_name="resnet18",
    encoder_weights=None,
    in_channels=3,
    classes=SEGMENTATION_CLASSES,
    activation=None
)

checkpoint = torch.load(
    CHECKPOINT_PATH,
    map_location="cpu",
    weights_only=True
)

segmentation_model.load_state_dict(
    checkpoint["model_state_dict"]
)

segmentation_model = segmentation_model.to(
    DEVICE
)

segmentation_model.eval()

print("Segmentation model loaded.")


# ============================================================
# LOAD DEPTH MODEL
# ============================================================

print()
print("Loading pretrained depth model...")
print(f"Model: {DEPTH_MODEL}")

depth_pipeline = pipeline(
    task="depth-estimation",
    model=DEPTH_MODEL,
    device=DEVICE
)

print("Depth model loaded.")


# ============================================================
# SEGMENTATION PREPROCESSING
# ============================================================

def prepare_segmentation_image(image_rgb):

    resized = cv2.resize(
        image_rgb,
        (IMAGE_SIZE, IMAGE_SIZE),
        interpolation=cv2.INTER_LINEAR
    )

    image = (
        resized.astype(np.float32)
        / 255.0
    )

    mean = np.array(
        [0.485, 0.456, 0.406],
        dtype=np.float32
    )

    std = np.array(
        [0.229, 0.224, 0.225],
        dtype=np.float32
    )

    image = (
        image - mean
    ) / std

    image = np.transpose(
        image,
        (2, 0, 1)
    )

    tensor = torch.from_numpy(
        image.copy()
    ).float()

    tensor = tensor.unsqueeze(0)

    return tensor.to(DEVICE)


# ============================================================
# SEMANTIC SEGMENTATION
# ============================================================

def predict_segmentation(image_rgb):

    tensor = prepare_segmentation_image(
        image_rgb
    )

    with torch.no_grad():

        logits = segmentation_model(
            tensor
        )

        probabilities = torch.softmax(
            logits,
            dim=1
        )

        prediction = torch.argmax(
            probabilities,
            dim=1
        )

    prediction = (
        prediction[0]
        .detach()
        .cpu()
        .numpy()
    )

    confidence = (
        probabilities.max(
            dim=1
        )[0][0]
        .detach()
        .cpu()
        .numpy()
    )

    return prediction, confidence


# ============================================================
# DEPTH ESTIMATION
# ============================================================

def predict_depth(image_rgb):

    pil_image = Image.fromarray(
        image_rgb
    )

    result = depth_pipeline(
        pil_image
    )

    depth = result["predicted_depth"]

    if torch.is_tensor(depth):

        depth = (
            depth
            .detach()
            .float()
            .cpu()
            .numpy()
        )

    else:

        depth = np.asarray(
            depth,
            dtype=np.float32
        )

    if depth.ndim == 3:
        depth = depth.squeeze()

    depth = cv2.resize(
        depth,
        (IMAGE_SIZE, IMAGE_SIZE),
        interpolation=cv2.INTER_LINEAR
    )

    depth = depth.astype(
        np.float32
    )

    depth_min = np.percentile(
        depth,
        2
    )

    depth_max = np.percentile(
        depth,
        98
    )

    depth = np.clip(
        depth,
        depth_min,
        depth_max
    )

    depth = (
        depth - depth_min
    ) / (
        depth_max - depth_min + 1e-8
    )

    return depth


# ============================================================
# DEPTH ROUGHNESS
# ============================================================

def calculate_roughness(depth):

    gx = cv2.Sobel(
        depth,
        cv2.CV_32F,
        1,
        0,
        ksize=3
    )

    gy = cv2.Sobel(
        depth,
        cv2.CV_32F,
        0,
        1,
        ksize=3
    )

    gradient = np.sqrt(
        gx * gx +
        gy * gy
    )

    local_mean = cv2.GaussianBlur(
        depth,
        (7, 7),
        0
    )

    local_sq_mean = cv2.GaussianBlur(
        depth * depth,
        (7, 7),
        0
    )

    variance = (
        local_sq_mean -
        local_mean * local_mean
    )

    variance = np.maximum(
        variance,
        0
    )

    roughness = (
        gradient +
        np.sqrt(variance)
    )

    p2 = np.percentile(
        roughness,
        2
    )

    p98 = np.percentile(
        roughness,
        98
    )

    roughness = np.clip(
        roughness,
        p2,
        p98
    )

    roughness = (
        roughness - p2
    ) / (
        p98 - p2 + 1e-8
    )

    return roughness

# ============================================================
# SINGLE-FRAME PERCEPTION API
# ============================================================

# ============================================================
# SINGLE-FRAME PERCEPTION API
# ============================================================

def predict_frame(image_rgb):
    """
    Run the complete perception pipeline on one RGB image.

    Returns the validated numerical outputs required by
    the downstream navigation stack.
    """

    if not isinstance(image_rgb, np.ndarray):
        raise TypeError(
            "image_rgb must be a NumPy array"
        )

    if image_rgb.ndim != 3:
        raise ValueError(
            "image_rgb must have shape (H, W, 3)"
        )

    if image_rgb.shape[2] != 3:
        raise ValueError(
            "image_rgb must have exactly 3 channels"
        )

    if image_rgb.dtype != np.uint8:
        raise ValueError(
            "image_rgb must have dtype uint8"
        )

    # --------------------------------------------------------
    # Semantic segmentation
    # --------------------------------------------------------

    prediction, confidence = (
        predict_segmentation(
            image_rgb
        )
    )
    model_prediction = prediction
    # --------------------------------------------------------
    # Depth estimation
    # --------------------------------------------------------

    depth = predict_depth(
        image_rgb
    )

    # --------------------------------------------------------
    # Ground roughness
    # --------------------------------------------------------

    roughness = calculate_ground_roughness(
        depth,
        prediction,
        confidence
    )
    prediction = (model_prediction + 1).astype(np.uint8)

    # --------------------------------------------------------
    # Convert to runtime contract
    # --------------------------------------------------------

    prediction = np.asarray(
        prediction,
        dtype=np.uint8
    )

    confidence = np.asarray(
        confidence,
        dtype=np.float32
    )

    depth = np.asarray(
        depth,
        dtype=np.float32
    )

    roughness = np.asarray(
        roughness,
        dtype=np.float32
    )

    # --------------------------------------------------------
    # Validate dimensions
    # --------------------------------------------------------

    expected_shape = (
        IMAGE_SIZE,
        IMAGE_SIZE
    )

    outputs = {
        "segmentation": prediction,
        "confidence": confidence,
        "depth": depth,
        "roughness": roughness,
    }

    for name, output in outputs.items():

        if output.shape != expected_shape:
            raise RuntimeError(
                f"{name} has shape "
                f"{output.shape}; expected "
                f"{expected_shape}"
            )

        if not np.isfinite(output).all():
            raise RuntimeError(
                f"{name} contains non-finite values"
            )

    # --------------------------------------------------------
    # Validate semantic class IDs
    # --------------------------------------------------------

    valid_classes = np.isin(
        prediction,
        [0, 1, 2,3]
    )

    if not valid_classes.all():
        raise RuntimeError(
            "segmentation contains invalid class IDs"
        )

    # --------------------------------------------------------
    # Validate normalized continuous outputs
    # --------------------------------------------------------

    for name in (
        "confidence",
        "depth",
        "roughness",
    ):

        output = outputs[name]

        if np.any(output < 0.0):
            raise RuntimeError(
                f"{name} contains values below 0"
            )

        if np.any(output > 1.0):
            raise RuntimeError(
                f"{name} contains values above 1"
            )

    return outputs
# ============================================================
# COLOR VISUALIZATION
# ============================================================

def make_segmentation_visual(
    prediction,
    confidence
):

    visualization = np.zeros(
        (
            IMAGE_SIZE,
            IMAGE_SIZE,
            3
        ),
        dtype=np.uint8
    )

    # Traversable
    visualization[
        prediction == 0
    ] = [0, 255, 0]

    # Non-traversable
    visualization[
        prediction == 1
    ] = [0, 255, 255]

    # Obstacle
    visualization[
        prediction == 2
    ] = [0, 0, 255]

    # Low-confidence pixels become neutral
    low_confidence = (
        confidence < 0.50
    )

    visualization[
        low_confidence
    ] = [128, 128, 128]

    return visualization


# ============================================================
# DEPTH VISUALIZATION
# ============================================================

def make_depth_visual(depth):

    depth_uint8 = (
        depth * 255
    ).astype(np.uint8)

    return cv2.applyColorMap(
        depth_uint8,
        cv2.COLORMAP_TURBO
    )


# ============================================================
# ROUGHNESS VISUALIZATION
# ============================================================

def make_roughness_visual(
    roughness
):

    roughness_uint8 = (
        roughness * 255
    ).astype(np.uint8)

    return cv2.applyColorMap(
        roughness_uint8,
        cv2.COLORMAP_HOT
    )


# ============================================================
# TRAVERSABILITY SCORE
# ============================================================

def calculate_traversability(
    prediction,
    confidence,
    roughness,
    depth
):

    score = np.full(
        prediction.shape,
        100.0,
        dtype=np.float32
    )

    # --------------------------------------------------------
    # Semantic penalties
    # --------------------------------------------------------

    # Non-traversable
    score[
        prediction == 1
    ] -= 55.0

    # Obstacles
    score[
        prediction == 2
    ] -= 90.0

    # --------------------------------------------------------
    # Low confidence penalty
    # --------------------------------------------------------

    confidence_penalty = (
        1.0 - confidence
    ) * 30.0

    score -= confidence_penalty

    # --------------------------------------------------------
    # Roughness penalty
    # --------------------------------------------------------

    score -= (
        roughness * 30.0
    )

    # --------------------------------------------------------
    # Gradual proximity weighting
    #
    # Lower part of image is treated as more important,
    # but there is NO hard horizontal ROI cutoff.
    # --------------------------------------------------------

    h, w = score.shape

    y = np.linspace(
        0,
        1,
        h,
        dtype=np.float32
    )

    proximity = (
        y ** 1.7
    )

    proximity = (
        proximity[:, None]
    )

    hazard_mask = (
        (prediction == 1) |
        (prediction == 2)
    )

    proximity_penalty = (
        hazard_mask.astype(
            np.float32
        )
        * proximity
        * 20.0
    )

    score -= proximity_penalty

    # --------------------------------------------------------
    # Depth discontinuity penalty
    # --------------------------------------------------------

    depth_gx = cv2.Sobel(
        depth,
        cv2.CV_32F,
        1,
        0,
        ksize=3
    )

    depth_gy = cv2.Sobel(
        depth,
        cv2.CV_32F,
        0,
        1,
        ksize=3
    )

    depth_gradient = np.sqrt(
        depth_gx * depth_gx +
        depth_gy * depth_gy
    )

    dg_p98 = np.percentile(
        depth_gradient,
        98
    )

    depth_gradient = np.clip(
        depth_gradient /
        (dg_p98 + 1e-8),
        0,
        1
    )

    score -= (
        depth_gradient * 20.0
    )

    # --------------------------------------------------------
    # Final range
    # --------------------------------------------------------

    score = np.clip(
        score,
        0,
        100
    )

    return score


# ============================================================
# SAFETY VISUALIZATION
# ============================================================

def make_safety_visual(score):

    # Higher score = safer.
    safety = score.astype(
        np.uint8
    )

    return cv2.applyColorMap(
        safety,
        cv2.COLORMAP_TURBO
    )


# ============================================================
# DASHBOARD
# ============================================================

def make_dashboard(
    original,
    segmentation,
    depth_visual,
    roughness_visual,
    safety_visual
):

    original = cv2.resize(
        original,
        (IMAGE_SIZE, IMAGE_SIZE)
    )

    segmentation = cv2.resize(
        segmentation,
        (IMAGE_SIZE, IMAGE_SIZE)
    )

    depth_visual = cv2.resize(
        depth_visual,
        (IMAGE_SIZE, IMAGE_SIZE)
    )

    roughness_visual = cv2.resize(
        roughness_visual,
        (IMAGE_SIZE, IMAGE_SIZE)
    )

    safety_visual = cv2.resize(
        safety_visual,
        (IMAGE_SIZE, IMAGE_SIZE)
    )

    row1 = np.hstack(
        [
            original,
            segmentation
        ]
    )

    row2 = np.hstack(
        [
            depth_visual,
            roughness_visual
        ]
    )

    row3 = np.hstack(
        [
            safety_visual,
            original
        ]
    )

    dashboard = np.vstack(
        [
            row1,
            row2,
            row3
        ]
    )

    return dashboard


# ============================================================
# IMAGE DISCOVERY
# ============================================================

# ============================================================
# BATCH PROCESSING
# ============================================================

def process_image_directory():

    valid_extensions = (
        ".jpg",
        ".jpeg",
        ".png",
        ".bmp",
        ".webp"
    )

    image_files = sorted(
        [
            f
            for f in os.listdir(
                INPUT_DIR
            )
            if f.lower().endswith(
                valid_extensions
            )
        ]
    )

    print(
        f"Images found: {len(image_files)}"
    )
    print()

    # ========================================================
    # PROCESS IMAGES
    # ========================================================

    for index, filename in enumerate(
        image_files,
        start=1
    ):

        print(
            f"[{index}/{len(image_files)}] "
            f"{filename}"
        )

        input_path = os.path.join(
            INPUT_DIR,
            filename
        )

        image_bgr = cv2.imread(
            input_path
        )

        if image_bgr is None:

            print(
                "  Could not read image."
            )

            continue

        image_rgb = cv2.cvtColor(
            image_bgr,
            cv2.COLOR_BGR2RGB
        )

        # ----------------------------------------------------
        # Complete perception
        # ----------------------------------------------------

        result = predict_frame(
            image_rgb
        )

        prediction = result[
            "segmentation"
        ]

        confidence = result[
            "confidence"
        ]

        depth = result[
            "depth"
        ]

        roughness = result[
            "roughness"
        ]

        # ----------------------------------------------------
        # Traversability
        # ----------------------------------------------------

        score = calculate_traversability(
            prediction,
            confidence,
            roughness,
            depth
        )

        # ----------------------------------------------------
        # Visualizations
        # ----------------------------------------------------

        segmentation_visual = (
            make_segmentation_visual(
                prediction,
                confidence
            )
        )

        depth_visual = (
            make_depth_visual(
                depth
            )
        )

        roughness_visual = (
            make_roughness_visual(
                roughness
            )
        )

        safety_visual = (
            make_safety_visual(
                score
            )
        )

        # ----------------------------------------------------
        # Resize original
        # ----------------------------------------------------

        original_small = cv2.resize(
            image_bgr,
            (IMAGE_SIZE, IMAGE_SIZE),
            interpolation=cv2.INTER_AREA
        )

        # ----------------------------------------------------
        # Overlay
        # ----------------------------------------------------

        overlay = cv2.addWeighted(
            original_small,
            0.55,
            segmentation_visual,
            0.45,
            0
        )

        # ----------------------------------------------------
        # Dashboard
        # ----------------------------------------------------

        dashboard = make_dashboard(
            original_small,
            segmentation_visual,
            depth_visual,
            roughness_visual,
            safety_visual
        )

        base_name = os.path.splitext(
            filename
        )[0]

        # ----------------------------------------------------
        # Save visual outputs
        # ----------------------------------------------------

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_segmentation.jpg"
            ),
            segmentation_visual
        )

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_depth.jpg"
            ),
            depth_visual
        )

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_roughness.jpg"
            ),
            roughness_visual
        )

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_overlay.jpg"
            ),
            overlay
        )

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_safety.jpg"
            ),
            safety_visual
        )

        cv2.imwrite(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_analysis.jpg"
            ),
            dashboard
        )

        # ----------------------------------------------------
        # Save numerical outputs
        # ----------------------------------------------------

        np.save(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_depth.npy"
            ),
            depth
        )

        np.save(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_roughness.npy"
            ),
            roughness
        )

        np.save(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_segmentation.npy"
            ),
            prediction
        )

        np.save(
            os.path.join(
                OUTPUT_DIR,
                f"{base_name}_confidence.npy"
            ),
            confidence
        )

        print("  Saved.")

    # ========================================================
    # COMPLETE
    # ========================================================

    print()
    print("=" * 70)
    print("PREDICTION COMPLETE")
    print("=" * 70)
    print(
        f"Results saved to:\n{OUTPUT_DIR}"
    )
    print()


# ============================================================
# SCRIPT ENTRY POINT
# ============================================================

if __name__ == "__main__":
    process_image_directory()