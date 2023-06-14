# SPDX-License-Identifier: BSD-3-Clause

__all__ = (
    'read',
    'write',
)

def read(addr: int, len: int) -> None | bytearray: ...
def write(addr: int, buff: bytearray) -> int: ...
