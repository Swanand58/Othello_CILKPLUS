#Othello Implementation using CILK_PLUS

# Parallel Othello (Reversi) Game with Cilk Plus

## Overview

This repository contains a parallelized version of the classic strategy game Othello (also known as Reversi), implemented in Cilk Plus. The project's goal was to create a shared-memory parallel program that allows for computer vs. computer gameplay, with the ability to search multiple moves ahead to determine the best move based on a simple board evaluation strategy. The program is designed for the NOTS (nots.rice.edu) environment and utilizes Cilk Plus for parallelization, including the use of reducers to ensure deterministic behavior in move selection.

## Features

- Parallelized Othello game engine with customizable search depth for AI.
- Utilizes Intel Cilk Plus for efficient parallel computation.
- Supports both human vs. computer and computer vs. computer gameplay modes.
- Deterministic AI move selection using Cilk Plus reducers.
- Implements the negamax algorithm for AI decision-making.
- Includes tools and scripts for performance analysis and race condition detection.
  
## Setup Instructions

### Prerequisites

- Access to NOTS (nots.rice.edu) with a valid Rice University netid.
- Module system with access to Cilk Plus tools (`module load cilkplus`).
- Intel Cilk++ SDK and GNU Compiler Collection for compiling the source code.

### Compilation

1. Clone this repository to your local environment on NOTS.
2. Load the Cilk Plus module: `module load cilkplus`.
3. Compile the program: `make all`. This will generate the executable `othello` and a serialized version `othello-serial` for performance baseline measurements.

### Running the Program

1. To play a game, simply execute `./othello`. The game will prompt you to specify the players (human or computer) and, for computer players, the search depth.
2. For automated experiments, use the provided `submit.sbatch` script with SLURM: `sbatch submit.sbatch`.

## Experimentation and Analysis

This project includes tools and scripts for performance analysis:

- **Cilkscreen**: For detecting data races. Run with `cilkscreen ./othello < input`.
- **Cilkview**: For profiling parallelism. Profiles for lookahead depths 1-7 are provided.
- **HPCToolkit**: For detailed performance analysis (COMP 534 students).

Refer to the `experiments` directory for the results of these analyses.

## Acknowledgments

- This project was developed for COMP 422/534 at Rice University.
- Thanks to the course instructors Prof John Mellor Crummy and TAs for their guidance and support.

## Contact

For questions or support, please open an issue in this GitHub repository.
