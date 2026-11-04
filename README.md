# [COP4610Proj2]

[Description]

## Group Members
- **[John Simson]**: [School Email]
- **[Riley Galpin]**: [School Email]
- **[Taylor Vaughn]**: [School Email]

## Kernel Versions & Ubuntu Versions:
Everyone Run: 
```bash
lsb_release -a
uname -r
uname -m
```

- **John Simson**: 
```bash
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 26.04.1 LTS
Release:	26.04
Codename:	resolute
7.0.0-42-generic
x86_64
```

- **Riley Galpin**: 
- **Taylor Vaughn**:

## Division of Labor

Team Members:
1. Riley Galpin
2. Taylor Vaughn
3. John Simson 

Part 1: System Call Tracing [Done: Y , Tested: Y]
- Riley Galpin

Part 2: Timer Kernel Module [Done: , Tested:]
- Taylor Vaughn

Part 3a: Adding System Calls [Done: Y, Tested: Y]
- Riley Galpin, John Simson

Part 3b: Kernel Compilation [Done: Y , Tested: Y]
- Riley Galpin

Part 3c: Threads [Done: , Tested:]
- John Simson

Part 3d: Linked List [Done: , Tested:]
- Taylor Vaughn, John Simson

Part 3e: Mutexes [Done: , Tested:]
-Taylor Vaughn, John Simson

Part 3f: Scheduling Algorithm [Done: , Tested:]
- John Simson 

## General Dev Dependency Graph

PHASE 1
┌─────────┬─────────┬─────────┐
│ Part 1  │ Part 2  │ Part 3a │
└─────────┴─────────┴────┬────┘
                         ↓
PHASE 2                Part 3b
                         ↓
PHASE 3       ┌──────────┼──────────┐
              ↓          ↓          ↓
             3c         3d         3e
           Threads     Lists      Mutexes
              └──────────┼──────────┘
                         ↓
PHASE 4                 3f
                    Scheduling
                         ↓
                  Integration/Test

## File Listing

## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Riley Galpin

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

### Taylor Vaughn

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


### John Simson

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-03 | [Description of task]  |
| 2026-11-04 | [Description of task]  |
| 2026-11-05 | [Description of task]  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |


## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Extra Credit
- **Extra Credit 1:**: [Extra Credit Option]
- **Extra Credit 2:**: [Extra Credit Option]
- **Extra Credit 3:**: [Extra Credit Option]

## Considerations
[Description]
