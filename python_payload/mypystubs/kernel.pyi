class task:
    """Not exposed in the API, used by scheduler_snapshot"""

    #: Name of the task
    name: str

    #: One of RUNNING, READY, BLOCKED, SUSPENDED, DELETED, INVALID
    state: int

    #: man i don't even know
    number: int

    #: you get the idea
    stack_left: int
    run_time: int
    core_affinity: int

class scheduler_snapshot:
    total_runtime: int
    tasks: list[task]

class heap_kind_stats:
    """Not exposed in the API, used by heap_stats"""

    total_free_bytes: int
    total_allocated_bytes: int
    largest_free_block: int

class heap_stats:
    general: heap_kind_stats
    dma: heap_kind_stats

RUNNING: int
READY: int
BLOCKED: int
SUSPENDED: int
DELETED: int
INVALID: int
