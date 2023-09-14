# Memory Management Unit Simulator (with Eviction)

## Overview

This project is a Memory Management Unit (MMU) simulator implemented in C. It simulates the functionality of an MMU, which manages the mapping between virtual memory and physical memory. The simulator also incorporates an eviction mechanism to handle memory overflow situations. This README provides an overview of the project's purpose, features, and usage.

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Usage](#usage)
- [Implementation Details](#implementation-details)
- [Contact Information](#contact-information)

## Features

- **Virtual Memory Management:** The simulator creates a page table to map virtual memory addresses to physical memory addresses.
  
- **Dynamic Memory Allocation:** It uses `posix_memalign()` to allocate physical memory pages with proper alignment.
  
- **Eviction Policy:** When physical memory is full, the simulator uses a First-In, First-Out (FIFO) eviction scheme to move pages from physical memory to disk.

- **Read and Write Operations:** The simulator supports read and write operations on virtual memory, ensuring that data is properly managed in both physical memory and on disk.

## Project Structure

- `mmusim.c`: The main C file containing the MMU simulator implementation.

## Usage

To compile and run the MMU simulator, follow these steps:

1. Clone the repository to your local machine:

   ```
   git clone https://github.com/yourusername/memory-management-simulator.git
   ```

2. Navigate to the project directory:

   ```
   cd memory-management-simulator
   ```

3. Compile the simulator using the provided `Makefile`:

   ```
   make
   ```

4. Run the simulator with the appropriate command-line arguments:

   ```
   ./mmusim [pagesize] [vmpc] [pmpc]
   ```

   - `[pagesize]`: The size of a memory page (must be a power of 2).
   - `[vmpc]`: The number of virtual pages of memory.
   - `[pmpc]`: The number of physical pages of memory.

5. Use the simulator by entering commands as prompted, such as `readbyte` and `writebyte`.

## Implementation Details

For a detailed explanation of the implementation and code structure, please refer to the source code in `mmusim.c`. The code includes comments and explanations for each function and major component.

## Contact Information

If you have any questions or would like to discuss this project further, please feel free to contact me:

- **Name:** Jonathan Zirman
- **Email:** [jdzirman@mail.yu.edu](mailto:jdzirman@mail.yu.edu)
- **GitHub:** [github.com/yzirman](https://github.com/yzirman)

Thank you for reviewing this project!
```