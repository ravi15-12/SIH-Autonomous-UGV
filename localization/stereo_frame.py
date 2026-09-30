from dataclasses import dataclass


@dataclass(frozen=True)
class StereoFrame:
    left_frame_rgb: object
    right_frame_rgb: object
    timestamp_ns: int

    def is_valid(self) -> bool:
        return (
            self.left_frame_rgb is not None
            and self.right_frame_rgb is not None
            and self.timestamp_ns > 0
        )