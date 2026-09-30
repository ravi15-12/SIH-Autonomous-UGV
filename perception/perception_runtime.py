import argparse
import time

from perception.camera_stream import CameraStream
from perception.full_prediction import predict_frame
from mapping.metric_map_runtime import MetricMapRuntime


class PerceptionRuntime:
    def __init__(
        self,
        camera_source=0,
        camera_width=1280,
        camera_height=720,
        camera_fps=30,
        metric_map_shared_memory_name="ugv_metric_map",
        metric_map_ready_semaphore="ugv_metric_map_ready",
        metric_map_free_semaphore="ugv_metric_map_free",
    ):
        self.camera = CameraStream(
            source=camera_source,
            width=camera_width,
            height=camera_height,
            fps=camera_fps,
        )

        self.metric_map_runtime = MetricMapRuntime(
            shared_memory_name=metric_map_shared_memory_name,
            ready_semaphore=metric_map_ready_semaphore,
            free_semaphore=metric_map_free_semaphore,
        )

        self.running = False

    def start(self):
        if self.running:
            raise RuntimeError(
                "Perception runtime is already running"
            )

        self.camera.open()
        self.metric_map_runtime.start()
        self.running = True

    def process_next_frame(self):
        if not self.running:
            raise RuntimeError(
                "Perception runtime is not running"
            )

        frame_rgb = self.camera.read()

        perception = predict_frame(
            frame_rgb
        )

        timestamp_ns = time.time_ns()

        metric_map_sequence = self.metric_map_runtime.process(
            segmentation=perception["segmentation"],
            confidence=perception["confidence"],
            depth=perception["depth"],
            roughness=perception["roughness"],
            timestamp_ns=timestamp_ns,
        )

        return metric_map_sequence

    def stop(self):
        if not self.running:
            return

        try:
            self.metric_map_runtime.stop()
        finally:
            self.camera.close()
            self.running = False

    def __enter__(self):
        self.start()
        return self

    def __exit__(
        self,
        exc_type,
        exc_value,
        traceback,
    ):
        self.stop()


def main():
    parser = argparse.ArgumentParser(
        description="UGV perception and metric-map runtime"
    )

    parser.add_argument(
        "--source",
        default=0,
        help="Camera index or video-file path",
    )

    parser.add_argument(
        "--width",
        type=int,
        default=1280,
    )

    parser.add_argument(
        "--height",
        type=int,
        default=720,
    )

    parser.add_argument(
        "--fps",
        type=int,
        default=30,
    )

    parser.add_argument(
        "--metric-map-shared-memory",
        default="ugv_metric_map",
    )

    parser.add_argument(
        "--metric-map-ready-semaphore",
        default="ugv_metric_map_ready",
    )

    parser.add_argument(
        "--metric-map-free-semaphore",
        default="ugv_metric_map_free",
    )

    args = parser.parse_args()

    try:
        source = int(args.source)
    except ValueError:
        source = args.source

    runtime = PerceptionRuntime(
        camera_source=source,
        camera_width=args.width,
        camera_height=args.height,
        camera_fps=args.fps,
        metric_map_shared_memory_name=args.metric_map_shared_memory,
        metric_map_ready_semaphore=args.metric_map_ready_semaphore,
        metric_map_free_semaphore=args.metric_map_free_semaphore,
    )

    runtime.start()

    try:
        while True:
            metric_map_sequence = (
                runtime.process_next_frame()
            )

            print(
                f"Published metric_map="
                f"{metric_map_sequence}"
            )

    except RuntimeError as error:
        if str(error) != "Frame source has no more frames":
            raise

    except KeyboardInterrupt:
        pass

    finally:
        runtime.stop()


if __name__ == "__main__":
    main()