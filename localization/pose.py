from dataclasses import dataclass
import math


@dataclass(frozen=True)
class RobotPose:
    x_m: float = 0.0
    y_m: float = 0.0
    yaw_rad: float = 0.0

    def is_valid(self) -> bool:
        return (
            math.isfinite(self.x_m)
            and math.isfinite(self.y_m)
            and math.isfinite(self.yaw_rad)
        )