from abc import ABC, abstractmethod

from localization.pose import RobotPose


class LocalizationBackend(ABC):

    @abstractmethod
    def initialize(self) -> None:
        pass

    @abstractmethod
    def process_frame(
        self,
        left_frame_rgb,
        right_frame_rgb,
        imu_data=None,
    ) -> RobotPose:
        pass

    @abstractmethod
    def is_initialized(self) -> bool:
        pass

    @abstractmethod
    def reset(self) -> None:
        pass

    @abstractmethod
    def shutdown(self) -> None:
        pass