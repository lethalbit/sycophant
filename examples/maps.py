# SPDX-License-Identifier: BSD-3-Clause
# load with `SYCOPHANT_MODULE=maps`

from sycophant.proc import maps


# Print all memory maps for the current process
for entry in  maps.all():
    print(f'Mapping: { entry.path if entry.is_backed() else "ANONYMOUS" }')
    print(f'  Range: {entry.start:016X}:{entry.end:016X}')
    print(f'  Prot:  {entry.flags}')
