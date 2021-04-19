# LEBench-Sim
A cleaned-up gem5-compatible version of https://github.com/LinuxPerfStudy/LEBench

## How to Build
Build for native/gem5 execution:
```make```

Build for native/gem5 execution and measure latency with `rdtscp`:
```make EXTRA_FLAGS="-D USE_RDTSCP"```

Build with gem5 Region-of-Interests (ROI) hooks:
```make hook```

## How to Run
Run a pre-defined workload:
```sudo bin/LEBench-run <workload ID> <scale>```

`<workload ID>`, a non-negative integer, selects a pre-defined workload to execute.
`<scale>`, a positive integer, used to increase the number of iterations of the workload.
For example, by default, `context-switch` executes 200 times.
If the scale is set to `10`, it now will execute 2000 times.
It is an useful option if you are running it natively
and want to increase the number of measurements for a better precision.
But, it is better to keep it small if you are running it in gem5,
otherwise it will take a long time to finish.

The complete list of pre-defined workloads:
|ID |  Workload Name  |
|---| ----------------|
|0  | context-switch  |
|1  | small-read      |
|2  | med-read        |
|3  | big-read        |
|4  | small-write     |
|5  | med-write       |
|6  | large-write     |
|7  | mmap            |
|8  | munmap          |
|9  | fork            |
|10 | big-fork        |
|11 | thread-create   |
|12 | small-send      |
|13 | big-send        |
|14 | small-recv      |
|15 | big-recv        |
|16 | small-select    |
|17 | big-select      |
|18 | small-poll      |
|19 | big-poll        |
|20 | small-epoll     |
|21 | big-epoll       |
|22 | small-pagefault |
|23 | big-pagefault   |

## Timing Method
### clock_gettime (default)
Uses Linux's `clock_gettime` to measure latencies by default.
However, it is not recommended if you are running it in gem5,
in which case `clock_gettime` does not return accurate results.

### rdtscp
An alternative way to measure latencies is using `rdtscp` instruction.
It is useful if you are running it in gem5.
It is enabled by default for the build with gem5 hooks
but have to be enabled manually by compiling with `-D USE_RDTSCP`.

### no measurement
If you want to measure the performance by Cycles-Per-Instruction (CPI) in
gem5 instead of execution latency, this is a good choice.
It can be enabled with
```make EXTRA_FLAGS="-D DISABLE_TIMER"```.
It should be used with gem5 hooks to only measure the CPI within a ROI.

## Limitations
The project is still at an early stage.
Measurements are not stable for some workloads.
