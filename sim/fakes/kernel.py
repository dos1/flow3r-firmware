class FakeHeapKindStats:
    def __init__(self, kind):
        self.kind = kind
        self.total_free_bytes = 1337
        self.total_allocated_bytes = 1337
        self.largest_free_block = 1337

class FakeHeapStats:
    general = FakeHeapKindStats('general')
    dma = FakeHeapKindStats('dma')

def heap_stats():
    return FakeHeapStats()
