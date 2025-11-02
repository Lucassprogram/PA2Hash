Concurrent Hash Table (chash)
=============================

Build
-----
1. Ensure a POSIX environment with `gcc`, `make`, and POSIX threads support.
2. Run `make` to compile the program.

Run
---
1. Place a `commands.txt` file in the root directory (same folder as the executable).
2. Execute `./chash`.
3. The program reads commands from `commands.txt`, writes execution details to `hash.log`, and appends search/print results to `output.txt`.

Features
--------
- Jenkins one-at-a-time hashing with a single linked list bucket (collisions are not expected per assignment spec).
- Reader/writer synchronization using `pthread_rwlock_t`.
- Per-command threading using the provided priority as the logical thread identifier.
- Structured logging for commands and lock state transitions (`hash.log`).
- Thread-safe writes to an output transcript (`output.txt`).
- Console feedback matching the spec excerpt (insert/update/delete/search/print).

File Overview
-------------
- `src/hash_table.c` & `include/hash_table.h`: data structure and core operations (lock must be held by caller).
- `src/command_processor.c`: worker routines that log, acquire locks, and execute operations.
- `src/commands.c`: parsing for `commands.txt`.
- `src/logger.c`: synchronized logging helpers.
- `src/output_writer.c`: synchronized output helper.
- `src/timestamp.c`: microsecond timestamp utility.
- `src/chash.c`: program entry point and threading bootstrap.

Testing
-------
- Provide your own `commands.txt` or adapt the included sample.
- Example: `make && ./chash`.
