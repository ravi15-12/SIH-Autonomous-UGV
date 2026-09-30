import cv2


class CameraStream:
    """
    Continuous frame-source interface.

    Supported sources:
    - USB/CSI camera: integer source such as 0
    - Video file: path to a video file
    """

    def __init__(
        self,
        source=0,
        width=1280,
        height=720,
        fps=30,
    ):
        self.source = source
        self.width = width
        self.height = height
        self.fps = fps

        self.capture = None

    def open(self):
        if self.capture is not None:
            raise RuntimeError("Frame source is already open")

        self.capture = cv2.VideoCapture(self.source)

        if not self.capture.isOpened():
            self.capture.release()
            self.capture = None
            raise RuntimeError(
                f"Unable to open frame source: {self.source}"
            )

        # Camera-only configuration.
        # Video files already have their own properties.
        if isinstance(self.source, int):
            self.capture.set(
                cv2.CAP_PROP_FRAME_WIDTH,
                self.width,
            )
            self.capture.set(
                cv2.CAP_PROP_FRAME_HEIGHT,
                self.height,
            )
            self.capture.set(
                cv2.CAP_PROP_FPS,
                self.fps,
            )

    def read(self):
        if self.capture is None:
            raise RuntimeError("Frame source is not open")

        success, frame_bgr = self.capture.read()

        if not success or frame_bgr is None:
            raise RuntimeError(
                "Frame source has no more frames"
            )

        if frame_bgr.ndim != 3 or frame_bgr.shape[2] != 3:
            raise RuntimeError(
                f"Invalid frame shape: {frame_bgr.shape}"
            )

        frame_rgb = cv2.cvtColor(
            frame_bgr,
            cv2.COLOR_BGR2RGB,
        )

        return frame_rgb

    def close(self):
        if self.capture is not None:
            self.capture.release()
            self.capture = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()