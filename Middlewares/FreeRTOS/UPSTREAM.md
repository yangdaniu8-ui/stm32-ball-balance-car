# Vendored FreeRTOS kernel

- Upstream: https://github.com/FreeRTOS/FreeRTOS-Kernel
- Tag: V11.1.0
- Commit: dbf70559b27d39c1fdb68dfb9a32140b6a6777a0
- License: MIT; see LICENSE.md and upstream source headers.
- Port: portable/RVDS/ARM_CM3 (Keil ARMCC5, Cortex-M3).

Only tasks.c, queue.c, list.c, public headers and the selected port are compiled.
Tasks, queues and the idle task use static allocation. heap_4.c is supplied for
reference but is not compiled; dynamic allocation is disabled. No kernel source
has been locally modified.
