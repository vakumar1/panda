## PANDA DDR Join Optimization Algorithm

Implementation of the [PANDA](https://arxiv.org/abs/2402.02001) join - note that this library implements the DDR join evaluation and excludes the CQ join evaluation included at the end of the paper.

My notes on the paper (covering the background and my intuition required for implementing the algorithm) are linked below
- [x] [PANDA Part 1: Preliminaries and Degree](https://hackmd.io/@n9vXJ2dWSK-txnWnjmMPGQ/SyuKXWw91l)
- [x] [PANDA Part 2: The Core Shannon Inequality](https://hackmd.io/@n9vXJ2dWSK-txnWnjmMPGQ/S1tjXcCsyl)
- [x] [PANDA Part 3: The Core Algorithm](https://hackmd.io/@n9vXJ2dWSK-txnWnjmMPGQ/rJsugb7Jxg)

### Library Usage

- create an application that takes a dependency on the `//src:panda_lib` library
- construct a **PANDA subproblem** (the spec file section below illustrates how this is done for the example provided in the paper) - include from [here](https://github.com/vakumar1/panda/blob/main/src/model/panda.h)
- call `generate_ddr_feasible_output` to generate a **feasible output** for the passed PANDA subproblem - include from [here](https://github.com/vakumar1/panda/blob/main/src/panda.h)

### Spec Files

The PANDA subproblem may be directly parsed from a **YAML specification file** as illustrated in the example taken from the paper [here](https://github.com/vakumar1/panda/blob/main/examples/example1.cpp) - the specification file format used by the example defines the arguments to a PANDA subproblem. The example specification file is provided [here](https://github.com/vakumar1/panda/blob/main/examples/specs/example1.yaml). The file format requires the following fields:
- `GlobalSchema` - the list of **human-readable attribute/column names** covering all input tables - note that the order of the names must correspond exactly with the type parameterization of the PANDA subproblem
- `Tables` - a triple for each table in the input schema containing
  - the **name of the table CSV file** from which the table will be loaded (T)
  - the **degree constraint** on the table (n)
  - the corresponding **w parameter** in the Shannon inequality
- `OutputAttributes` - a list of **column name lists** - each list corresponds to a relation in the output schema (Z)
- `M` - a list of **monotonicity witnesses** - each monotonicity contains
  - a list of **column names** - the conditioned attributes Y in Y|X
  - a list of **column names** - the condition attributes X in Y|X
  - a **positive integer multiplicity** of the monotonicity
- `S` - a list of **submodularity witnesses** - each submodularity contains
  - a list of **column names** - the conditioned attributes Y in Y;Z|X
  - a list of **column names** - the conditioned attributes Z in Y;Z|X
  - a list of **column names** - the condition attributes X in Y;Z|X
