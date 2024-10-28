# ThinASM_OS
Arduino_Thin_Pre-Emptive_OS

This is a pre-emptive kernel. The context switch is very similar to FreeRTOS with some optimizations.
CreateTask is custom

Task names and stack variables must be hardcoded.
Stack memory will be included in compiler's dynamic memory allocation - stacks are explicitly defined.

Not mutex or semaphore support
Yield function

Note - yield can be used before shared data or memory access and the time slice can be managed to allow time for data synchronization (pushing workign registers to gloabl variables, etc)

Please comment!
