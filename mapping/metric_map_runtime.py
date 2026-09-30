from pathlib import Path

from mapping.camera_geometry import load_camera_geometry
from mapping.ground_projection import GroundProjector
from mapping.local_map_config import load_local_map_config
from mapping.metric_local_map import MetricLocalMapBuilder
from mapping.metric_map_publisher import MetricMapPublisher


class MetricMapRuntime:
    """
    Converts perception outputs into a metric local cost map
    and publishes the result through synchronized shared memory.

    Camera geometry and local-map geometry are loaded from
    config/camera.yaml.
    """

    def __init__(
        self,
        config_path="config/camera.yaml",
        shared_memory_name="ugv_metric_map",
        ready_semaphore="ugv_metric_map_ready",
        free_semaphore="ugv_metric_map_free",
    ):
        self.config_path = Path(config_path)

        self.camera_geometry = load_camera_geometry(
            self.config_path
        )

        self.local_map_config = load_local_map_config(
            self.config_path
        )

        self.projector = GroundProjector(
            self.camera_geometry
        )

        self.builder = MetricLocalMapBuilder(
            self.projector,
            self.local_map_config,
        )

        self.publisher = MetricMapPublisher(
            name=shared_memory_name,
            ready_semaphore=ready_semaphore,
            free_semaphore=free_semaphore,
        )

        self.running = False

    def start(self):
        if self.running:
            raise RuntimeError(
                "Metric map runtime is already running."
            )

        self.publisher.create()
        self.running = True

    def process(
        self,
        segmentation,
        confidence,
        depth,
        roughness,
        timestamp_ns=None,
    ):
        if not self.running:
            raise RuntimeError(
                "Metric map runtime is not running."
            )

        cost_map, observed = self.builder.build(
            segmentation=segmentation,
            confidence=confidence,
            depth=depth,
            roughness=roughness,
        )

        sequence = self.publisher.publish(
            cost_map=cost_map,
            observed=observed,
            timestamp_ns=timestamp_ns,
        )

        return sequence

    def stop(self):
        if not self.running:
            return

        try:
            self.publisher.unlink()
        finally:
            self.publisher.close()
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