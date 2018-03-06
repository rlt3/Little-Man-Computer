# Little Man Computer

The LMC is small computer architecture for learning the big picture about how
machines work. The architecture is a toy architecture so the virtual machine
and assembler were fairly simple to come up with. Building this prepared me to
implementing a [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) machine and
assembler too.

## How do I use it?

Simply clone the repository and run `make`. The code has no dependencies and
comes with some fun LMC assembly files like `hello.asm'. Run it like `./lmc
hello.asm`.

This architecture has an `input` instruction. My machine handles this by
expecting arguments to be put on the command line following the assembly file.
`subtract.asm' subtracts two numbers given on the command line. You would call
it like `./lmc subtract.asm 4 2`.

## What are the instructions?

The instructions can be found on this table at
[Wikipedia](https://en.wikipedia.org/wiki/Little_man_computer#Instructions).
