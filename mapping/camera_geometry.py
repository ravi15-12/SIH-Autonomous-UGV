import math
from dataclasses import dataclass

import yaml


@dataclass(frozen=True)
class CameraGeometry:
    height_m: float
    pitch_deg: float
    roll_deg: float
    yaw_deg: float
    horizontal_offset_m: float
    vertical_offset_m: float
    width: int
    height: int
    hfov_deg: float
    max_ground_distance_m: float

    @property
    def hfov_rad(self):
        return math.radians(self.hfov_deg)

    @property
    def focal_length_px(self):
        return (
            self.width / 2.0
        ) / math.tan(
            self.hfov_rad / 2.0
        )

    @property
    def principal_x(self):
        return self.width / 2.0

    @property
    def principal_y(self):
        return self.height / 2.0

    def validate(self):
        values = (
            self.height_m,
            self.pitch_deg,
            self.roll_deg,
            self.yaw_deg,
            self.horizontal_offset_m,
            self.vertical_offset_m,
            self.hfov_deg,
            self.max_ground_distance_m,
        )

        if not all(math.isfinite(v) for v in values):
            raise ValueError(
                "Camera geometry contains non-finite values."
            )

        if self.height_m <= 0.0:
            raise ValueError(
                "Camera height must be positive."
            )

        if self.width <= 0 or self.height <= 0:
            raise ValueError(
                "Camera resolution must be positive."
            )

        if not 1.0 <= self.hfov_deg < 179.0:
            raise ValueError(
                "Camera HFOV must be between 1 and 179 degrees."
            )

        if self.max_ground_distance_m <= 0.0:
            raise ValueError(
                "Maximum ground distance must be positive."
            )

        if self.focal_length_px <= 0.0:
            raise ValueError(
                "Invalid camera focal length."
            )


def load_camera_geometry(path):
    with open(path, "r", encoding="utf-8") as file:
        data = yaml.safe_load(file)

    if not isinstance(data, dict):
        raise ValueError(
            "Camera configuration must be a mapping."
        )

    camera = data.get("camera")

    if not isinstance(camera, dict):
        raise ValueError(
            "Missing 'camera' configuration."
        )

    geometry = CameraGeometry(
        height_m=float(camera["height_m"]),
        pitch_deg=float(camera["pitch_deg"]),
        roll_deg=float(camera["roll_deg"]),
        yaw_deg=float(camera["yaw_deg"]),
        horizontal_offset_m=float(
            camera["horizontal_offset_m"]
        ),
        vertical_offset_m=float(
            camera["vertical_offset_m"]
        ),
        width=int(camera["width"]),
        height=int(camera["height"]),
        hfov_deg=float(camera["hfov_deg"]),
        max_ground_distance_m=float(
            camera["max_ground_distance_m"]
        ),
    )

    geometry.validate()

    return geometry