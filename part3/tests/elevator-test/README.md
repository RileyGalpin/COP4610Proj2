# Elevator User-Space Tests

These programs call the custom elevator system calls through `wrappers.h`.
They require a kernel with system calls 548, 549, and 550 assigned to the
project elevator calls.

Build without running kernel calls:

```bash
make
```

After booting the custom kernel and loading the elevator module:

```bash
./consumer --start
./producer 10
cat /proc/elevator
./consumer --stop
```

The producer generates valid floors 1 through 5 and random pet types 0 through
3. Do not run these programs against a kernel where syscall numbers 548 through
550 have not been assigned to this project. Our custom kernel will need to implement these.
