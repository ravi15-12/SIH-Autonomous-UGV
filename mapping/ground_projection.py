import math

import numpy as np

from mapping.camera_geometry import CameraGeometry


class GroundProjector:
    """
    Projects camera pixels onto a flat ground plane.

    Coordinate convention:
        X = forward from the UGV
        Y = left from the UGV
        Z = ground height

    The camera geometry is provisional until the physical
    camera is mounted and calibrated.
    """

    def __init__(self, geometry: CameraGeometry):
        geometry.validate()
        self.geometry = geometry

        self._pitch_rad = math.radians(
            geometry.pitch_deg
        )

        self._cos_pitch = math.cos(
            self._pitch_rad
        )

        self._sin_pitch = math.sin(
            self._pitch_rad
        )

        self._focal_length = (
            geometry.focal_length_px
        )

        self._cx = geometry.principal_x
        self._cy = geometry.principal_y

    def project_pixels(
        self,
        pixels_u,
        pixels_v,
    ):
        """
        Project image pixels onto the flat ground plane.

        Returns:
            Nx2 array:
                column 0 = forward distance in metres
                column 1 = left distance in metres

        Invalid ground intersections are represented by NaN.
        """

        u = np.asarray(
            pixels_u,
            dtype=np.float64,
        )

        v = np.asarray(
            pixels_v,
            dtype=np.float64,
        )

        if u.shape != v.shape:
            raise ValueError(
                "Pixel coordinate arrays must have "
                "the same shape."
            )

        if not np.all(np.isfinite(u)):
            raise ValueError(
                "Pixel u coordinates must be finite."
            )

        if not np.all(np.isfinite(v)):
            raise ValueError(
                "Pixel v coordinates must be finite."
            )

        if np.any(u < 0) or np.any(
            u >= self.geometry.width
        ):
            raise ValueError(
                "Pixel u coordinate is outside "
                "the configured image."
            )

        if np.any(v < 0) or np.any(
            v >= self.geometry.height
        ):
            raise ValueError(
                "Pixel v coordinate is outside "
                "the configured image."
            )

        normalized_x = (
            u - self._cx
        ) / self._focal_length

        normalized_y = (
            v - self._cy
        ) / self._focal_length

        ray_forward = np.ones_like(
            normalized_x
        )

        ray_left = normalized_x
        ray_down = normalized_y

        rotated_forward = (
            self._cos_pitch * ray_forward
            + self._sin_pitch * ray_down
        )

        rotated_down = (
            -self._sin_pitch * ray_forward
            + self._cos_pitch * ray_down
        )

        valid = (
            rotated_down > 1e-8
        )

        forward = np.full(
            u.shape,
            np.nan,
            dtype=np.float64,
        )

        left = np.full(
            u.shape,
            np.nan,
            dtype=np.float64,
        )

        forward[valid] = (
            self.geometry.height_m
            * rotated_forward[valid]
            / rotated_down[valid]
        )

        left[valid] = (
            self.geometry.height_m
            * ray_left[valid]
            / rotated_down[valid]
        )

        forward += (
            self.geometry.vertical_offset_m
        )

        left += (
            self.geometry.horizontal_offset_m
        )

        valid &= (
            forward >= 0.0
        )

        valid &= (
            forward <=
            self.geometry.max_ground_distance_m
        )

        forward[~valid] = np.nan
        left[~valid] = np.nan

        return np.column_stack(
            (forward, left)
        )

    def project_image(self):
        """
        Project every image pixel onto the flat ground plane.

        Invalid pixels are represented by NaN.
        """

        u, v = np.meshgrid(
            np.arange(
                self.geometry.width,
                dtype=np.float64,
            ),
            np.arange(
                self.geometry.height,
                dtype=np.float64,
            ),
        )

        return self.project_pixels(
            u.reshape(-1),
            v.reshape(-1),
        ).reshape(
            self.geometry.height,
            self.geometry.width,
            2,
        )
    def project_image_for_shape(
        self,
        target_height,
        target_width,
    ):
        """Project a perception image whose resolution differs
        from the physical camera resolution.

        Perception pixels are mapped back to the configured
        physical-camera pixel coordinates before projection."""

        if target_height <= 0 or target_width <= 0:
            raise ValueError(
                "Target image dimensions must be positive."
            )

        scale_x = (
            self.geometry.width
            / float(target_width)
        )

        scale_y = (
            self.geometry.height
            / float(target_height)
        )

        u, v = np.meshgrid(
            np.arange(
                target_width,
                dtype=np.float64,
            ),
            np.arange(
                target_height,
                dtype=np.float64,
            ),
        )

        u_camera = (
            (u + 0.5) * scale_x
        ) - 0.5

        v_camera = (
            (v + 0.5) * scale_y
        ) - 0.5

        return self.project_pixels(
            u_camera.reshape(-1),
            v_camera.reshape(-1),
        ).reshape(
            target_height,
            target_width,
            2,
        )