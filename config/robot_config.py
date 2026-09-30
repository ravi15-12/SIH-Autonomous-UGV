from dataclasses import dataclass
import math


@dataclass(frozen=True)
class RobotConfig:
    name: str

    wheel_diameter_m: float
    wheel_base_m: float
    track_width_m: float

    length_m: float
    width_m: float

    front_overhang_m: float
    rear_overhang_m: float

    additional_safety_margin_m: float

    max_linear_velocity_mps: float
    max_angular_velocity_radps: float

    def validate(self) -> None:
        """Validate the physical and safety configuration."""

        positive_fields = {
            "wheel_diameter_m": self.wheel_diameter_m,
            "wheel_base_m": self.wheel_base_m,
            "track_width_m": self.track_width_m,
            "length_m": self.length_m,
            "width_m": self.width_m,
            "max_linear_velocity_mps": self.max_linear_velocity_mps,
            "max_angular_velocity_radps": self.max_angular_velocity_radps,
        }

        for name, value in positive_fields.items():
            if not math.isfinite(value) or value <= 0.0:
                raise ValueError(
                    f"{name} must be a finite value greater than zero"
                )

        non_negative_fields = {
            "front_overhang_m": self.front_overhang_m,
            "rear_overhang_m": self.rear_overhang_m,
            "additional_safety_margin_m": self.additional_safety_margin_m,
        }

        for name, value in non_negative_fields.items():
            if not math.isfinite(value) or value < 0.0:
                raise ValueError(
                    f"{name} must be a finite non-negative value"
                )

        if not self.name.strip():
            raise ValueError("name must not be empty")

        if self.wheel_base_m > self.length_m:
            raise ValueError(
                "wheel_base_m cannot exceed length_m"
            )

        if self.track_width_m > self.width_m:
            raise ValueError(
                "track_width_m cannot exceed width_m"
            )

        if (
            self.wheel_base_m
            + self.front_overhang_m
            + self.rear_overhang_m
            > self.length_m
        ):
            raise ValueError(
                "wheel_base_m + front_overhang_m + "
                "rear_overhang_m cannot exceed length_m"
            )