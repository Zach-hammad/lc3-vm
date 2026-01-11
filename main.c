#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#define MEMORY_SIZE (1 << 16)
uint16_t memory[MEMORY_SIZE];

int running = 1;

uint16_t check_key() {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  struct timeval timeout = {0, 0};
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

struct termios original_tio;

void disable_input_buffering() {
  tcgetattr(STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
  tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}
enum {
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC,
  R_COND,
  R_COUNT
};
uint16_t reg[R_COUNT];

enum { FL_POS = 1 << 0, FL_ZRO = 1 << 1, FL_NEG = 1 << 2 };

enum {
  OP_BR = 0,
  OP_ADD,
  OP_LD,
  OP_ST,
  OP_JSR,
  OP_AND,
  OP_LDR,
  OP_STR,
  OP_RTI,
  OP_NOT,
  OP_LDI,
  OP_STI,
  OP_JMP,
  OP_RES,
  OP_LEA,
  OP_TRAP
};

enum {
  TRAP_GETC = 0x20,
  TRAP_OUT = 0x21,
  TRAP_PUTS = 0x22,
  TRAP_IN = 0x23,
  TRAP_PUTSP = 0x24,
  TRAP_HALT = 0x25
};

#define KBSR 0xFE00
#define KBDR 0xFE02
#define DSR 0xFE04
#define DDR 0xFE06
#define MCR 0xFFFE

uint16_t swap_bytes(uint16_t byte) {
  uint16_t low_byte = byte & 0xFF;
  uint16_t high_byte = (byte >> 8) & 0xFF;
  return (low_byte << 8) | high_byte;
}
void load_program(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    exit(1);
  }
  uint16_t origin;
  fread(&origin, sizeof(origin), 1, file);
  origin = swap_bytes(origin);
  reg[R_COND] = FL_ZRO;
  reg[R_PC] = origin;
  uint16_t value;
  while (fread(&value, sizeof(value), 1, file) == 1) {
    memory[origin++] = swap_bytes(value);
  }
  fclose(file);
}
uint16_t mem_read(uint16_t addr) {
  if (addr == KBSR) {
    if (check_key()) {
      memory[KBSR] = 0x8000;
    } else {
      memory[KBSR] = 0;
    }
    return memory[KBSR];
  } else if (addr == KBDR) {
    return (uint16_t)getchar();
  } else if (addr == DSR) {
    return 0x8000;
  } else if (addr == DDR) {
    return 0;
  }
  return memory[addr];
}

void mem_write(uint16_t addr, uint16_t val) {
  if (addr == DDR) {
    putchar((char)val);
    fflush(stdout);
  } else if (addr == MCR) {
    if ((val & 0x8000) == 0) {
      running = 0;
    }
    memory[addr] = val;
  } else {
    memory[addr] = val;
  }
}
void setcc(uint16_t result) {
  if (result >> 15 == 1) {
    reg[R_COND] = FL_NEG;
  } else if (result == 0) {
    reg[R_COND] = FL_ZRO;
  } else {
    reg[R_COND] = FL_POS;
  }
}

int16_t sign_extend(int num_bits, uint16_t res) {
  if ((res >> (num_bits - 1)) & 1) {
    res |= (0xFFFF << num_bits);
  }
  return res;
}
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Please input only file");
    exit(1);
  }
  disable_input_buffering();
  load_program(argv[1]);
  while (running) {
    uint16_t instruction = mem_read(reg[R_PC]);
    uint16_t op = instruction >> 12;
    reg[R_PC]++;

    switch (op) {
    case OP_BR: {
      uint16_t cond_flag = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      if (cond_flag & reg[R_COND]) {
        reg[R_PC] += pcoffset9;
      }
      break;
    }
    case OP_ADD: {
      uint16_t dr = (instruction >> 9) & 0x7;
      uint16_t sr1 = (instruction >> 6) & 0x7;
      uint16_t sr2 = instruction & 0x7;
      uint16_t bit5 = (instruction >> 5) & 0x1;
      if (bit5 == 0) {
        reg[dr] = reg[sr1] + reg[sr2];
      } else {
        int16_t imm5 = sign_extend(5, instruction & 0x1F);
        reg[dr] = reg[sr1] + imm5;
      }
      setcc(reg[dr]);
      break;
    }
    case OP_LD: {
      uint16_t dr = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      reg[dr] = mem_read(reg[R_PC] + pcoffset9);
      setcc(reg[dr]);
      break;
    }
    case OP_ST: {
      uint16_t sr = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      mem_write(reg[R_PC] + pcoffset9, reg[sr]);
      break;
    }
    case OP_JSR: {
      int16_t pcoffset11 = sign_extend(11, instruction & 0x7FF);
      uint16_t bit11 = (instruction >> 11) & 0x1;
      uint16_t baseR = (instruction >> 6) & 0x7;
      reg[R_R7] = reg[R_PC];
      if (bit11 == 0) {
        reg[R_PC] = reg[baseR];
      } else {
        reg[R_PC] += pcoffset11;
      }
      break;
    }
    case OP_AND: {
      uint16_t dr = (instruction >> 9) & 0x7;
      uint16_t sr1 = (instruction >> 6) & 0x7;
      uint16_t sr2 = instruction & 0x7;
      uint16_t bit5 = (instruction >> 5) & 0x1;
      if (bit5 == 0) {
        reg[dr] = reg[sr1] & reg[sr2];
      } else {
        int16_t imm5 = sign_extend(5, instruction & 0x1F);
        reg[dr] = reg[sr1] & imm5;
      }
      setcc(reg[dr]);
      break;
    }
    case OP_LDR: {
      uint16_t dr = (instruction >> 9) & 0x7;
      uint16_t baseR = (instruction >> 6) & 0x7;
      int16_t offset6 = sign_extend(6, instruction & 0x3F);
      reg[dr] = mem_read(reg[baseR] + offset6);
      setcc(reg[dr]);
      break;
    }
    case OP_STR: {
      uint16_t sr = (instruction >> 9) & 0x7;
      uint16_t baseR = (instruction >> 6) & 0x7;
      int16_t offset6 = sign_extend(6, instruction & 0x3F);
      mem_write(reg[baseR] + offset6, reg[sr]);
      break;
    }
    case OP_RTI:
      break;
    case OP_NOT: {
      uint16_t dr = (instruction >> 9) & 0x7;
      uint16_t sr = (instruction >> 6) & 0x7;
      reg[dr] = ~reg[sr];
      setcc(reg[dr]);
      break;
    }
    case OP_LDI: {
      uint16_t dr = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      reg[dr] = mem_read(mem_read(reg[R_PC] + pcoffset9));
      setcc(reg[dr]);
      break;
    }

    case OP_STI: {
      uint16_t sr = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      mem_write(mem_read(reg[R_PC] + pcoffset9), reg[sr]);
      break;
    }
    case OP_JMP: {
      uint16_t baseR = (instruction >> 6) & 0x7;
      reg[R_PC] = reg[baseR];
      break;
    }
    case OP_RES:
      break;
    case OP_LEA: {
      uint16_t dr = (instruction >> 9) & 0x7;
      int16_t pcoffset9 = sign_extend(9, instruction & 0x1FF);
      reg[dr] = reg[R_PC] + pcoffset9;
      setcc(reg[dr]);
      break;
    }
    case OP_TRAP: {
      uint16_t trapvect8 = instruction & 0xFF;
      switch (trapvect8) {
      case TRAP_GETC: {
        reg[R_R0] = getchar();
        break;
      }
      case TRAP_OUT: {
        putchar(reg[R_R0]);
        fflush(stdout);
        break;
      }
      case TRAP_PUTS: {
        uint16_t *mem = &memory[reg[R_R0]];
        while (*mem != 0x0000) {
          putchar(*mem);
          mem++;
        }
        fflush(stdout);
        break;
      }
      case TRAP_IN: {
        printf("Enter a character: ");
        reg[R_R0] = getchar();
        putchar(reg[R_R0]);
        fflush(stdout);
        break;
      }
      case TRAP_PUTSP: {
        uint16_t *mem = &memory[reg[R_R0]];
        uint16_t low_byte = *mem & 0xFF;
        uint16_t high_byte = (*mem >> 8) & 0xFF;
        while (*mem != 0) {
          putchar(low_byte);
          if (high_byte != 0) {
            putchar(high_byte);
          }
          mem++;
          low_byte = *mem & 0xFF;
          high_byte = (*mem >> 8) & 0xFF;
        }
        break;
      }
      case TRAP_HALT: {
        running = 0;
        break;
      }
      }
      break;
    }
    }
  }
  restore_input_buffering();
  return 0;
}
