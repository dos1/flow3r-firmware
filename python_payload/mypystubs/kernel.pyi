class task:
    """Not exposed in the API, used by scheduler_snapshot"""

    #: Name of the task
    name: str

    #: One of RUNNING, READY, BLOCKED, SUSPENDED, DELETED, INVALID
    state: int

    #: The FreeRTOS task number
    number: int

    #: High water mark of stack usage by task, ie. highest ever
    #: recorded use of stack. The units seem arbitrary.
    stack_left: int

    #: The run time allocated to this task so far, as defined by the FreeRTOS
    #: run time stats clock. The units are arbitrary and should only be used
    #: comparatively against other task runtimes, and the global total runtime
    #: value from scheduler_stats.
    run_time: int

    #: bitmask of where this task is allowed to be scheduled. Bit
    #: 0 is core 0, bit 1 is core 1. The value 0b11 (3) means the
    #: task is allowed to run on any core.
    core_affinity: int

class scheduler_snapshot:
    """
    A snapshot of the FreeRTOS scheduler state. Will not update dynamically,
    instead needs to be re-created by calling scheduler_snapshot() again.
    """

    #: The total run time since boot as defined by the FreeRTOS run time stats
    #: clock
    total_runtime: int

    #: a list of tasks
    tasks: list[task]

class heap_kind_stats:
    """Not exposed in the API, used by heap_stats"""

    #: Total free bytes in the heap. Equivalent to multi_free_heap_size().
    total_free_bytes: int

    #: Total bytes allocated to data in the heap.
    total_allocated_bytes: int

    #: Size of the largest free block in the heap. This is the largest
    #: malloc-able size.
    largest_free_block: int

class heap_stats:
    #: Heap stats for general memory
    general: heap_kind_stats

    #: Heap stats for DMA-capable memory (i.e. not external ram)
    dma: heap_kind_stats

RUNNING: int
READY: int
BLOCKED: int
SUSPENDED: int
DELETED: int
INVALID: int

def freertos_sleep(ms: int) -> None: ...
def usb_connected() -> bool: ...
def usb_console_active() -> bool: ...
def hardware_version() -> str: ...
def firmware_version() -> str: ...
def i2c_scan() -> list[int]: ...
