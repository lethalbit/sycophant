# SPDX-License-Identifier: BSD-3-Clause

from typing import Collection

from enum import IntFlag

__all__ = (
	'mapentry_flags',
	'mapentry',
	'has_addr',
	'refresh',
	'get',
	'all',
)

class mapentry_flags(IntFlag):
	NONE   = 0b00000000,
	READ   = 0b00000001,
	WRITE  = 0b00000010,
	EXEC   = 0b00000100,
	PRIV   = 0b00001000,
	SHARED = 0b00010000,
	VIRT   = 0b00100000,
	BACKED = 0b01000000,

	def __repr__(self) -> str: ...

class mapentry:
	start: int = ...
	end: int = ...
	size: int = ...
	flags: mapentry_flags = ...
	offset: int = ...
	past: None | str = ...

	def __init__(self) -> None: ...

	def can_read(self) -> bool: ...
	def can_write(self) -> bool: ...
	def can_execute(self) -> bool: ...

	def is_rwx(self) -> bool: ...
	def is_backed(self) -> bool: ...
	def is_virtual(self) -> bool: ...

	def __repr__(self) -> str: ...

def has_addr(addr: int) -> bool: ...
def refresh() -> None: ...
def get(idx: int) -> mapentry: ...
def all() -> Collection[mapentry]: ...
