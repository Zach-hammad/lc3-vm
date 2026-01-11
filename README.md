# LC-3 Virtual Machine

A virtual machine implementation of the LC-3 (Little Computer 3) architecture in C.

## About

The LC-3 is a 16-bit educational computer architecture used in introductory computer science courses. This VM can run LC-3 machine code programs, including games like 2048 and Rogue.

## Features

- Complete implementation of all 15 LC-3 opcodes
- Memory-mapped I/O (keyboard and display)
- Trap routines (GETC, OUT, PUTS, IN, PUTSP, HALT)
- Terminal raw mode for real-time keyboard input

## Building

```bash
gcc -o lc3 main.c
```

## Usage

```bash
./lc3 <program.obj>
```

Example:
```bash
./lc3 2048.obj
```

## LC-3 Architecture

- 16-bit address space (65,536 memory locations)
- 8 general-purpose registers (R0-R7)
- Program Counter (PC) and Condition Register (COND)
- Condition flags: Negative, Zero, Positive

## Opcodes

| Opcode | Name | Description |
|--------|------|-------------|
| 0000 | BR | Conditional branch |
| 0001 | ADD | Add |
| 0010 | LD | Load |
| 0011 | ST | Store |
| 0100 | JSR | Jump to subroutine |
| 0101 | AND | Bitwise AND |
| 0110 | LDR | Load register |
| 0111 | STR | Store register |
| 1000 | RTI | Return from interrupt |
| 1001 | NOT | Bitwise NOT |
| 1010 | LDI | Load indirect |
| 1011 | STI | Store indirect |
| 1100 | JMP | Jump |
| 1101 | RES | Reserved |
| 1110 | LEA | Load effective address |
| 1111 | TRAP | System call |

## Test Programs

- `2048.obj` - The 2048 sliding tile game (included)

## Resources

- [LC-3 ISA Specification](https://www.jmeiners.com/lc3-vm/)
- [Introduction to Computing Systems: From Bits & Gates to C & Beyond](https://www.mheducation.com/highered/product/introduction-computing-systems-bits-gates-c-beyond-patt-patel/M9781260150537.html)
