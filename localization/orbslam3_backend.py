from localization.backend import LocalizationBackend
from localization.pose import RobotPose


class ORBSLAM3Backend(LocalizationBackend):

    def __init__(self):
        self._initialized = False
        self._pose = RobotPose()

    def initialize(self) -> None:
        self._initialized = True
        self._pose = RobotPose()

    def process_frame(
        self,
        left_frame_rgb,
        right_frame_rgb,
        imu_data=None,
    ) -> RobotPose:
        if not self._initialized:
            raise RuntimeError(
                "ORB-SLAM3 backend is not initialized."
            )

        if left_frame_rgb is None:
            raise ValueError(
                "Left camera frame cannot be None."
            )

        if right_frame_rgb is None:
            raise ValueError(
                "Right camera frame cannot be None."
            )

        return self._pose

    def is_initialized(self) -> bool:
        return self._initialized

    def reset(self) -> None:
        self._pose = RobotPose()

    def shutdown(self) -> None:
        self._initialized = False
        self._pose = RobotPose()