def get_buffer_x() -> Optional[memoryview]:
    """
    Retrieve the scope's current x-axis buffer as a memoryview pointing to 16-bit integers.

    The buffer remains valid until the next call to sys_scope.get_buffer_x() or ctx.scope().
    """
    ...
