from pathlib import Path
from config.robot_config import RobotConfig

import yaml
import json


PROJECT_ROOT = Path(__file__).resolve().parent.parent


def load_yaml(filename: str) -> dict:
    """Load a YAML configuration file from the project config directory."""
    config_path = PROJECT_ROOT / "config" / filename

    if not config_path.exists():
        raise FileNotFoundError(
            f"Configuration file not found: {config_path}"
        )

    with config_path.open("r", encoding="utf-8") as file:
        data = yaml.safe_load(file)

    if not isinstance(data, dict):
        raise ValueError(
            f"Invalid YAML structure in: {config_path}"
        )

    return data


def _validate_finite_positive(
    value: object,
    field_name: str,
) -> None:
    """Validate that a configuration value is a positive finite number."""
    if not isinstance(value, (int, float)):
        raise ValueError(
            f"{field_name} must be a number"
        )

    if not float(value) > 0.0:
        raise ValueError(
            f"{field_name} must be greater than zero"
        )


def _validate_finite_non_negative(
    value: object,
    field_name: str,
) -> None:
    """Validate that a configuration value is a non-negative finite number."""
    if not isinstance(value, (int, float)):
        raise ValueError(
            f"{field_name} must be a number"
        )

    if float(value) < 0.0:
        raise ValueError(
            f"{field_name} must be non-negative"
        )


def load_robot_config() -> dict:
    """
    Load and validate the complete robot configuration.

    The returned dictionary retains the original YAML structure.
    Validation is performed here so malformed robot geometry or
    safety configuration is rejected before reaching the runtime.
    """
    config = load_yaml("robot.yaml")

    robot = config.get("robot")

    if not isinstance(robot, dict):
        raise ValueError(
            "robot.yaml must contain a 'robot' mapping"
        )

    required_positive_fields = (
        "wheel_diameter_m",
        "wheel_base_m",
        "track_width_m",
        "length_m",
        "width_m",
    )

    for field in required_positive_fields:
        if field not in robot:
            raise ValueError(
                f"robot.yaml missing required field: robot.{field}"
            )

        _validate_finite_positive(
            robot[field],
            f"robot.{field}",
        )

    required_non_negative_fields = (
        "front_overhang_m",
        "rear_overhang_m",
    )

    for field in required_non_negative_fields:
        if field not in robot:
            raise ValueError(
                f"robot.yaml missing required field: robot.{field}"
            )

        _validate_finite_non_negative(
            robot[field],
            f"robot.{field}",
        )

    safety = robot.get("safety")

    if not isinstance(safety, dict):
        raise ValueError(
            "robot.yaml must contain robot.safety"
        )

    if "additional_margin_m" not in safety:
        raise ValueError(
            "robot.yaml missing required field: "
            "robot.safety.additional_margin_m"
        )

    _validate_finite_non_negative(
        safety["additional_margin_m"],
        "robot.safety.additional_margin_m",
    )

    # Validate basic physical geometry relationships.
    if robot["wheel_base_m"] > robot["length_m"]:
        raise ValueError(
            "robot.wheel_base_m cannot exceed robot.length_m"
        )

    if robot["track_width_m"] > robot["width_m"]:
        raise ValueError(
            "robot.track_width_m cannot exceed robot.width_m"
        )

    if (
        robot["wheel_base_m"]
        + robot["front_overhang_m"]
        + robot["rear_overhang_m"]
        > robot["length_m"]
    ):
        raise ValueError(
            "robot.wheel_base_m + front_overhang_m + "
            "rear_overhang_m cannot exceed robot.length_m"
        )

    return config

def load_typed_robot_config() -> RobotConfig:
    """Load the validated YAML robot configuration into a typed object."""
    config = load_robot_config()
    robot = config["robot"]
    safety = robot["safety"]

    return RobotConfig(
        name=robot["name"],
        wheel_diameter_m=float(robot["wheel_diameter_m"]),
        wheel_base_m=float(robot["wheel_base_m"]),
        track_width_m=float(robot["track_width_m"]),
        length_m=float(robot["length_m"]),
        width_m=float(robot["width_m"]),
        front_overhang_m=float(robot["front_overhang_m"]),
        rear_overhang_m=float(robot["rear_overhang_m"]),
        additional_safety_margin_m=float(
            safety["additional_margin_m"]
        ),
        max_linear_velocity_mps=float(
            robot["max_linear_velocity_mps"]
        ),
        max_angular_velocity_radps=float(
            robot["max_angular_velocity_radps"]
        ),
    )

def export_cpp_robot_config(
    output_path: Path | None = None,
) -> Path:
    """Export the validated robot configuration for the C++ runtime."""

    config = load_typed_robot_config()
    config.validate()

    if output_path is None:
        output_path = PROJECT_ROOT / "config" / "cpp_robot_config.json"

    cpp_config = {
        "geometry": {
            "length_m": config.length_m,
            "width_m": config.width_m,
            "wheel_base_m": config.wheel_base_m,
            "track_width_m": config.track_width_m,
            "front_overhang_m": config.front_overhang_m,
            "rear_overhang_m": config.rear_overhang_m,
        },
        "additional_safety_margin_m": (
            config.additional_safety_margin_m
        ),
    }

    with output_path.open("w", encoding="utf-8") as file:
        json.dump(cpp_config, file, indent=2)
        file.write("\n")

    return output_path


def load_rellis_classes() -> dict:
    """Load the RELLIS-3D class configuration."""
    return load_yaml("rellis_classes.yaml")


def load_ugv_classes() -> dict:
    """Load the UGV class configuration."""
    return load_yaml("ugv_classes.yaml")


if __name__ == "__main__":
    robot = load_robot_config()
    rellis = load_rellis_classes()
    ugv = load_ugv_classes()

    print("Configuration loading successful.")
    print(f"Robot: {robot['robot']['name']}")
    print(
        f"Wheel diameter: "
        f"{robot['robot']['wheel_diameter_m']} m"
    )
    print(
        f"Robot dimensions: "
        f"{robot['robot']['length_m']} m × "
        f"{robot['robot']['width_m']} m"
    )
    print(
        f"Additional safety margin: "
        f"{robot['robot']['safety']['additional_margin_m']} m"
    )
    print(
        f"RELLIS classes: "
        f"{len(rellis['rellis_classes'])}"
    )
    print(
        f"UGV classes: "
        f"{len(ugv['ugv_classes'])}"
    )