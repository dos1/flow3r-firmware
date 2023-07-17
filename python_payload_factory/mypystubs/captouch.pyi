from typing import Protocol, List


class CaptouchPetalPadsState(Protocol):
	@property
	def tip(self) -> bool: ...
	
	@property
	def base(self) -> bool: ...

	@property
	def cw(self) -> bool: ...

	@property
	def ccw(self) -> bool: ...


class CaptouchPetalState(Protocol):
	@property
	def pressed(self) -> bool: ...

	@property
	def top(self) -> bool: ...

	@property
	def bottom(self) -> bool: ...

	@property
	def pads(self) -> CaptouchPetalPadsState: ...


class CaptouchState(Protocol):
	@property
	def petals(self) -> List[CaptouchPetalState]:
		...


def read() -> CaptouchState:
	...

def calibration_active() -> bool:
	...