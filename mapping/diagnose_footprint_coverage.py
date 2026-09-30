import numpy as np

from mapping.camera_geometry import load_camera_geometry
from mapping.ground_projection import GroundProjector
from mapping.local_map_config import load_local_map_config


ROBOT_LENGTH_M = 0.40
ROBOT_WIDTH_M = 0.30


def main():
    camera_geometry = load_camera_geometry("config/camera.yaml")
    local_map = load_local_map_config("config/camera.yaml")
    projector = GroundProjector(camera_geometry)

    projected = projector.project_image_for_shape(
        camera_geometry.height,
        camera_geometry.width,
    )

    forward = projected[:, :, 0]
    left = projected[:, :, 1]

    valid = np.isfinite(forward) & np.isfinite(left)

    valid &= forward >= local_map.forward_min_m
    valid &= forward < local_map.forward_max_m
    valid &= left >= local_map.left_min_m
    valid &= left < local_map.left_max_m

    resolution = local_map.resolution_m

    height = local_map.height
    width = local_map.width

    observed = np.zeros((height, width), dtype=bool)

    forward_valid = forward[valid]
    left_valid = left[valid]

    grid_x = np.floor(
        (forward_valid - local_map.forward_min_m)
        / resolution
    ).astype(np.int64)

    grid_y = np.floor(
        (left_valid - local_map.left_min_m)
        / resolution
    ).astype(np.int64)

    inside = (
        (grid_x >= 0)
        & (grid_x < height)
        & (grid_y >= 0)
        & (grid_y < width)
    )

    observed[grid_x[inside], grid_y[inside]] = True

    half_length = ROBOT_LENGTH_M / 2.0
    half_width = ROBOT_WIDTH_M / 2.0

    footprint_x_min = 0.0 - half_length
    footprint_x_max = 0.0 + half_length
    footprint_y_min = 0.0 - half_width
    footprint_y_max = 0.0 + half_width

    footprint_x0 = int(
        np.floor(
            (footprint_x_min - local_map.forward_min_m)
            / resolution
        )
    )
    footprint_x1 = int(
        np.ceil(
            (footprint_x_max - local_map.forward_min_m)
            / resolution
        )
    )

    footprint_y0 = int(
        np.floor(
            (footprint_y_min - local_map.left_min_m)
            / resolution
        )
    )
    footprint_y1 = int(
        np.ceil(
            (footprint_y_max - local_map.left_min_m)
            / resolution
        )
    )

    footprint_x0 = max(0, footprint_x0)
    footprint_y0 = max(0, footprint_y0)
    footprint_x1 = min(height, footprint_x1)
    footprint_y1 = min(width, footprint_y1)

    footprint = observed[
        footprint_x0:footprint_x1,
        footprint_y0:footprint_y1,
    ]

    total = footprint.size
    observed_count = int(np.count_nonzero(footprint))
    unobserved_count = total - observed_count

    observed_percent = (
        100.0 * observed_count / total
        if total > 0
        else 0.0
    )

    print("REAL MAP NEAR-FIELD ANALYSIS")
    print(f"MAP: ({height}, {width})")
    print(
        f"ROBOT FOOTPRINT: "
        f"{ROBOT_LENGTH_M:.2f} m x {ROBOT_WIDTH_M:.2f} m"
    )
    print(f"RESOLUTION: {resolution:.3f} m/cell")
    print(f"FOOTPRINT CELLS: {total}")
    print(f"OBSERVED FOOTPRINT CELLS: {observed_count}")
    print(f"UNOBSERVED FOOTPRINT CELLS: {unobserved_count}")
    print(f"OBSERVED FOOTPRINT %: {observed_percent:.2f}")


if __name__ == "__main__":
    main()
