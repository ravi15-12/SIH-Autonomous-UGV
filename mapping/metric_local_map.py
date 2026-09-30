import math

import numpy as np

from mapping.local_map_config import LocalMapConfig
from mapping.ground_projection import GroundProjector
from mapping.traversability import build_cost_map


class MetricLocalMapBuilder:
    """
    Converts camera-space perception into a metric local cost map.

    Coordinate convention:
        forward = UGV forward direction
        left    = UGV left direction

    Output:
        cost map: uint8 values in [0, 100]
        observed: bool mask indicating cells supported
                  by valid camera projection
    """

    def __init__(
        self,
        projector: GroundProjector,
        config: LocalMapConfig,
    ):
        config.validate()

        self.projector = projector
        self.config = config

        if (
            projector.geometry.width <= 0
            or projector.geometry.height <= 0
        ):
            raise ValueError(
                "Invalid projector image dimensions."
            )

    def build(
        self,
        segmentation,
        confidence,
        depth,
        roughness,
    ):
        segmentation = np.asarray(
            segmentation,
            dtype=np.uint8,
        )

        confidence = np.asarray(
            confidence,
            dtype=np.float32,
        )

        depth = np.asarray(
            depth,
            dtype=np.float32,
        )

        roughness = np.asarray(
            roughness,
            dtype=np.float32,
        )

        expected_shape = segmentation.shape

        if segmentation.ndim != 2:
            raise ValueError(
                "Segmentation must be a 2D array."
            )

        if confidence.shape != expected_shape:
            raise ValueError(
                f"Confidence shape must be {expected_shape}, "
                f"got {confidence.shape}."
            )

        if depth.shape != expected_shape:
            raise ValueError(
                f"Depth shape must be {expected_shape}, "
                f"got {depth.shape}."
            )

        if roughness.shape != expected_shape:
            raise ValueError(
                f"Roughness shape must be {expected_shape}, "
                f"got {roughness.shape}."
            )

        if not np.all(np.isfinite(confidence)):
            raise ValueError(
                "Confidence contains non-finite values."
            )

        if not np.all(np.isfinite(depth)):
            raise ValueError(
                "Depth contains non-finite values."
            )

        if not np.all(np.isfinite(roughness)):
            raise ValueError(
                "Roughness contains non-finite values."
            )

        if np.any(confidence < 0.0) or np.any(
            confidence > 1.0
        ):
            raise ValueError(
                "Confidence must be in [0, 1]."
            )

        if np.any(depth < 0.0) or np.any(
            depth > 1.0
        ):
            raise ValueError(
                "Depth must be in [0, 1]."
            )

        if np.any(roughness < 0.0) or np.any(
            roughness > 1.0
        ):
            raise ValueError(
                "Roughness must be in [0, 1]."
            )

        pixel_cost = build_cost_map(
            segmentation,
            confidence,
            roughness,
            depth,
        )

        projected = self.projector.project_image_for_shape(
            segmentation.shape[0],
            segmentation.shape[1],
        )

        forward = projected[:, :, 0]
        left = projected[:, :, 1]

        valid = (
            np.isfinite(forward)
            & np.isfinite(left)
        )

        valid &= (
            forward >= self.config.forward_min_m
        )

        valid &= (
            forward < self.config.forward_max_m
        )

        valid &= (
            left >= self.config.left_min_m
        )

        valid &= (
            left < self.config.left_max_m
        )

        map_height = self.config.height
        map_width = self.config.width

        output = np.full(
            (map_height, map_width),
            100,
            dtype=np.uint8,
        )

        observed = np.zeros(
            (map_height, map_width),
            dtype=bool,
        )

        if not np.any(valid):
            return output, observed

        forward_valid = forward[valid]
        left_valid = left[valid]
        cost_valid = pixel_cost[valid].astype(
            np.float64
        )

        grid_x = np.floor(
            (
                forward_valid
                - self.config.forward_min_m
            )
            / self.config.resolution_m
        ).astype(np.int64)

        grid_y = np.floor(
            (
                left_valid
                - self.config.left_min_m
            )
            / self.config.resolution_m
        ).astype(np.int64)

        inside = (
            (grid_x >= 0)
            & (grid_x < map_height)
            & (grid_y >= 0)
            & (grid_y < map_width)
        )

        grid_x = grid_x[inside]
        grid_y = grid_y[inside]
        cost_valid = cost_valid[inside]

        if grid_x.size == 0:
            return output, observed

        flat_index = (
            grid_x * map_width
            + grid_y
        )

        order = np.argsort(flat_index)

        flat_index = flat_index[order]
        cost_valid = cost_valid[order]

        unique_cells, starts = np.unique(
            flat_index,
            return_index=True,
        )

        ends = np.append(
            starts[1:],
            flat_index.size,
        )

        for cell, start, end in zip(
            unique_cells,
            starts,
            ends,
        ):
            cell_costs = cost_valid[start:end]

            # Conservative aggregation:
            # the highest observed cost dominates
            # the metric cell.
            cell_cost = np.max(cell_costs)

            cell_x = cell // map_width
            cell_y = cell % map_width

            output[
                cell_x,
                cell_y,
            ] = np.uint8(
                np.clip(
                    math.ceil(cell_cost),
                    0,
                    100,
                )
            )

            observed[
                cell_x,
                cell_y,
            ] = True

        return output, observed