import math
from dataclasses import dataclass

import yaml


@dataclass(frozen=True)
class LocalMapConfig:
    resolution_m: float
    forward_min_m: float
    forward_max_m: float
    left_min_m: float
    left_max_m: float

    @property
    def width(self):
        return int(
            math.ceil(
                (
                    self.left_max_m
                    - self.left_min_m
                )
                / self.resolution_m
            )
        )

    @property
    def height(self):
        return int(
            math.ceil(
                (
                    self.forward_max_m
                    - self.forward_min_m
                )
                / self.resolution_m
            )
        )

    def validate(self):
        values = (
            self.resolution_m,
            self.forward_min_m,
            self.forward_max_m,
            self.left_min_m,
            self.left_max_m,
        )

        if not all(
            math.isfinite(value)
            for value in values
        ):
            raise ValueError(
                "Local map configuration contains "
                "non-finite values."
            )

        if self.resolution_m <= 0.0:
            raise ValueError(
                "Local map resolution must be positive."
            )

        if self.forward_max_m <= self.forward_min_m:
            raise ValueError(
                "Invalid forward map bounds."
            )

        if self.left_max_m <= self.left_min_m:
            raise ValueError(
                "Invalid lateral map bounds."
            )

        if self.width <= 0 or self.height <= 0:
            raise ValueError(
                "Local map dimensions must be positive."
            )


def load_local_map_config(path):
    with open(
        path,
        "r",
        encoding="utf-8",
    ) as file:
        data = yaml.safe_load(file)

    if not isinstance(data, dict):
        raise ValueError(
            "Camera configuration must be a mapping."
        )

    local_map = data.get(
        "local_map"
    )

    if not isinstance(
        local_map,
        dict,
    ):
        raise ValueError(
            "Missing 'local_map' configuration."
        )

    config = LocalMapConfig(
        resolution_m=float(
            local_map["resolution_m"]
        ),
        forward_min_m=float(
            local_map["forward_min_m"]
        ),
        forward_max_m=float(
            local_map["forward_max_m"]
        ),
        left_min_m=float(
            local_map["left_min_m"]
        ),
        left_max_m=float(
            local_map["left_max_m"]
        ),
    )

    config.validate()

    return config