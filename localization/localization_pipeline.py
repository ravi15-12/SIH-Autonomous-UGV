from localization.backend import LocalizationBackend
from localization.localization_input import LocalizationInput
from localization.pose import RobotPose


class LocalizationPipeline:

    def __init__(self, backend: LocalizationBackend):
        self.backend = backend

    def initialize(self) -> None:
        self.backend.initialize()

    def process(
        self,
        localization_input: LocalizationInput,
    ) -> RobotPose:
        if not localization_input.is_valid():
            raise ValueError(
                "Invalid localization input."
            )

        return self.backend.process_frame(
            left_frame_rgb=localization_input.stereo_frame.left_frame_rgb,
            right_frame_rgb=localization_input.stereo_frame.right_frame_rgb,
            imu_data=localization_input.imu_samples,
        )

    def reset(self) -> None:
        self.backend.reset()

    def shutdown(self) -> None:
        self.backend.shutdown()

    def is_initialized(self) -> bool:
        return self.backend.is_initialized()