from dataclasses import dataclass

from localization.imu import IMUSample
from localization.stereo_frame import StereoFrame


@dataclass(frozen=True)
class LocalizationInput:
    stereo_frame: StereoFrame
    imu_samples: tuple[IMUSample, ...]

    def is_valid(self) -> bool:
        if not self.stereo_frame.is_valid():
            return False

        return all(
            sample.is_valid()
            for sample in self.imu_samples
        )