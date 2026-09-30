from dataclasses import dataclass
import math


@dataclass(frozen=True)
class IMUSample:
    timestamp_ns: int

    acceleration_x_mps2: float
    acceleration_y_mps2: float
    acceleration_z_mps2: float

    angular_velocity_x_rps: float
    angular_velocity_y_rps: float
    angular_velocity_z_rps: float

    def is_valid(self) -> bool:
        values = (
            self.acceleration_x_mps2,
            self.acceleration_y_mps2,
            self.acceleration_z_mps2,
            self.angular_velocity_x_rps,
            self.angular_velocity_y_rps,
            self.angular_velocity_z_rps,
        )

        return (
            self.timestamp_ns > 0
            and all(math.isfinite(value) for value in values)
        )