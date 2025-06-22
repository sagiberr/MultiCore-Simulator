# MultiCore-Simulator

A pipelined, multi-core processor simulator with MESI-based cache coherence, written in C++.

##  Overview

This project simulates a 4-core processor architecture, each with its own pipeline and cache, communicating over a shared memory bus. It implements:

- 5-stage pipelined CPU per core (Fetch, Decode, Execute, Memory, Write-Back)
- Cache coherence using the **MESI protocol**
- Round-robin bus arbitration for memory access
- Instruction set execution from hexadecimal input
- Performance statistics and trace generation

The simulator was developed as part of the *Computer Architecture* course at Tel Aviv University.

##  Features

-  Fully pipelined execution per core  
-  Memory access via cache, with hit/miss behavior  
-  MESI coherence protocol handling `Modified`, `Exclusive`, `Shared`, `Invalid` states  
-  Instruction/data trace generation  
-  Stall handling for hazards and memory delays  
-  Round-robin bus arbitration

##  Test Programs

The project includes three benchmark programs written in simulated assembly:

### 1.  Sequential Counter  
- All four cores increment a shared counter in order.
- Tests synchronization, stalls, and write coherence.

### 2.  Vector Addition (Serial)  
- A single core performs element-wise addition of two arrays.
- Tests memory reads, ALU operations, and write-back logic.

### 3.  Vector Addition (Parallel)  
- All four cores split the workload to add vectors in parallel.
- Tests MESI protocol under concurrent memory access.
