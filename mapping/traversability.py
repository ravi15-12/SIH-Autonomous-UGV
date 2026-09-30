import cv2
import numpy as np


# ============================================================
# UGV TRAVERSABILITY COST MAP
# ============================================================
#
# Input:
#   segmentation : 0 traversable
#                  1 non-traversable
#                  2 obstacle
#
#   confidence   : segmentation confidence [0, 1]
#   roughness    : relative terrain roughness [0, 1]
#   depth        : normalized relative depth [0, 1]
#
# Output:
#   cost         : 0   = safest
#                  100 = most dangerous
#
# IMPORTANT:
# This is a relative perception cost map.
# It is NOT a metric elevation map and does not claim
# to measure real obstacle height or ditch depth.
# ============================================================


# ------------------------------------------------------------
# COST WEIGHTS
# ------------------------------------------------------------

TRAVERSABLE_COST = 5.0
NON_TRAVERSABLE_COST = 65.0
OBSTACLE_COST = 100.0

CONFIDENCE_WEIGHT = 20.0
ROUGHNESS_WEIGHT = 25.0
DEPTH_EDGE_WEIGHT = 20.0

# Extra penalty toward the bottom of the image.
# This is gradual, not a hard ROI.
PROXIMITY_WEIGHT = 15.0


# ============================================================
# NORMALIZATION
# ============================================================

def normalize_map(values):

    values = values.astype(
        np.float32
    )

    low = np.percentile(
        values,
        2
    )

    high = np.percentile(
        values,
        98
    )

    if high - low < 1e-8:
        return np.zeros_like(
            values,
            dtype=np.float32
        )

    values = (
        values - low
    ) / (
        high - low
    )

    return np.clip(
        values,
        0.0,
        1.0
    )


# ============================================================
# DEPTH EDGE
# ============================================================

def calculate_depth_edge(depth):

    depth = depth.astype(
        np.float32
    )

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

    return normalize_map(
        gradient
    )
def calculate_ground_roughness(
    depth,
    segmentation,
    confidence
):
    """
    Estimate terrain roughness primarily on
    regions that could actually be traversable.

    This prevents sky/tree/object boundaries from
    dominating the terrain roughness estimate.
    """

    depth = depth.astype(np.float32)

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

    # --------------------------------------------------------
    # Only use likely ground regions.
    #
    # Current segmentation:
    # 0 = traversable
    # 1 = non-traversable
    # 2 = obstacle
    #
    # We deliberately exclude obstacle pixels.
    # --------------------------------------------------------

    ground_mask = (
        segmentation != 2
    )

    # Remove very uncertain pixels.
    ground_mask &= (
        confidence >= 0.40
    )

    # Smooth the mask slightly so isolated pixels
    # don't create noisy roughness.
    ground_mask = cv2.morphologyEx(
        ground_mask.astype(np.uint8),
        cv2.MORPH_OPEN,
        np.ones((5, 5), np.uint8)
    )

    ground_mask = (
        ground_mask.astype(bool)
    )

    # --------------------------------------------------------
    # Normalize using valid ground pixels.
    # --------------------------------------------------------

    valid_values = roughness[
        ground_mask
    ]

    if valid_values.size > 100:

        p2 = np.percentile(
            valid_values,
            2
        )

        p98 = np.percentile(
            valid_values,
            98
        )

    else:

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

    # Non-ground areas should not be treated as
    # terrain roughness.
    roughness[
        ~ground_mask
    ] = 0.0

    return np.clip(
        roughness,
        0.0,
        1.0
    )

# ============================================================
# PROXIMITY MAP
# ============================================================

def calculate_proximity(
    height,
    width
):

    y = np.linspace(
        0.0,
        1.0,
        height,
        dtype=np.float32
    )

    # Gradual increase toward camera.
    proximity = y ** 1.7

    return np.repeat(
        proximity[:, None],
        width,
        axis=1
    )


# ============================================================
# BUILD COST MAP
# ============================================================

def build_cost_map(
    segmentation,
    confidence,
    roughness,
    depth
):

    if not (
        segmentation.shape ==
        confidence.shape ==
        roughness.shape ==
        depth.shape
    ):
        raise ValueError(
            "All input maps must have "
            "the same shape."
        )

    height, width = (
        segmentation.shape
    )

    # --------------------------------------------------------
    # Start with semantic cost
    # --------------------------------------------------------

    cost = np.full(
        (height, width),
        TRAVERSABLE_COST,
        dtype=np.float32
    )

    cost[
        segmentation == 1
    ] = NON_TRAVERSABLE_COST

    cost[
        segmentation == 2
    ] = OBSTACLE_COST

    # --------------------------------------------------------
    # Confidence penalty
    # --------------------------------------------------------

    confidence = np.clip(
        confidence,
        0.0,
        1.0
    )

    confidence_penalty = (
        1.0 - confidence
    ) * CONFIDENCE_WEIGHT

    cost += confidence_penalty

    # --------------------------------------------------------
    # Roughness penalty
    # --------------------------------------------------------

    roughness = np.clip(
        roughness,
        0.0,
        1.0
    )

    cost += (
        roughness *
        ROUGHNESS_WEIGHT
    )

    # --------------------------------------------------------
    # Depth discontinuity
    # --------------------------------------------------------

    depth_edge = (
        calculate_depth_edge(
            depth
        )
    )

    cost += (
        depth_edge *
        DEPTH_EDGE_WEIGHT
    )

    # --------------------------------------------------------
    # Proximity weighting
    # --------------------------------------------------------

    proximity = (
        calculate_proximity(
            height,
            width
        )
    )

    hazard_mask = (
        (segmentation == 1) |
        (segmentation == 2)
    )

    cost += (
        hazard_mask.astype(
            np.float32
        )
        * proximity
        * PROXIMITY_WEIGHT
    )

    # --------------------------------------------------------
    # Obstacles remain extremely expensive
    # --------------------------------------------------------

    cost[
        segmentation == 2
    ] = np.maximum(
        cost[
            segmentation == 2
        ],
        90.0
    )

    # --------------------------------------------------------
    # Final range
    # --------------------------------------------------------

    cost = np.clip(
        cost,
        0.0,
        100.0
    )

    return cost


# ============================================================
# SAFETY CLASSES
# ============================================================

def classify_safety(
    cost
):

    safety = np.zeros(
        cost.shape,
        dtype=np.uint8
    )

    # 0 = SAFE
    safety[
        cost < 30
    ] = 0

    # 1 = CAUTION
    safety[
        (cost >= 30) &
        (cost < 65)
    ] = 1

    # 2 = BLOCKED
    safety[
        cost >= 65
    ] = 2

    return safety


# ============================================================
# COST VISUALIZATION
# ============================================================

def visualize_cost(
    cost
):

    cost_uint8 = (
        cost
        .clip(0, 100)
        * 2.55
    ).astype(
        np.uint8
    )

    return cv2.applyColorMap(
        cost_uint8,
        cv2.COLORMAP_TURBO
    )


# ============================================================
# SAFETY VISUALIZATION
# ============================================================

def visualize_safety(
    safety
):

    visualization = np.zeros(
        (
            safety.shape[0],
            safety.shape[1],
            3
        ),
        dtype=np.uint8
    )

    # SAFE
    visualization[
        safety == 0
    ] = [0, 255, 0]

    # CAUTION
    visualization[
        safety == 1
    ] = [0, 255, 255]

    # BLOCKED
    visualization[
        safety == 2
    ] = [0, 0, 255]

    return visualization


# ============================================================
# SUMMARY
# ============================================================

def summarize_cost(
    cost,
    safety
):

    total = safety.size

    safe_pixels = np.count_nonzero(
        safety == 0
    )

    caution_pixels = np.count_nonzero(
        safety == 1
    )

    blocked_pixels = np.count_nonzero(
        safety == 2
    )

    return {
        "mean_cost": float(
            np.mean(cost)
        ),
        "safe_percent": (
            100.0 *
            safe_pixels /
            total
        ),
        "caution_percent": (
            100.0 *
            caution_pixels /
            total
        ),
        "blocked_percent": (
            100.0 *
            blocked_pixels /
            total
        )
    }
