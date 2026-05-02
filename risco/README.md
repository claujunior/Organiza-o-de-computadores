# Processador RISCO — Implementação em SystemC

## Arquitetura

Implementação do **datapath básico** do processador RISCO.

### Organização dos Módulos

![Diagrama do Datapath com Pipeline](Datapath.jpeg)

### Conjunto de Instruções (ISA)

| Opcode | Mnemônico | Tipo | Operação |
|--------|-----------|------|----------|
| 0000   | ADD       | R    | rd = rs1 + rs2 |
| 0001   | SUB       | R    | rd = rs1 - rs2 |
| 0010   | AND       | R    | rd = rs1 & rs2 |
| 0011   | OR        | R    | rd = rs1 \| rs2 |
| 0100   | XOR       | R    | rd = rs1 ^ rs2 |
| 0101   | NOT       | R    | rd = ~rs1 |
| 0110   | SHL       | R    | rd = rs1 << 1 |
| 0111   | SHR       | R    | rd = rs1 >> 1 |
| 1000   | ADDI      | I    | rd = rs1 + imm |
| 1001   | LOAD      | I    | rd = mem[rs1 + imm] |
| 1010   | STORE     | I    | mem[rs1 + imm] = rd |
| 1011   | BEQ       | I    | if (rd==rs1) PC += imm |
| 1100   | JUMP      | I    | PC = imm |

### Formato das Instruções (32 bits)

**Tipo R:**
```
 31      28 27    24 23    20 19    16 15        0
┌──────────┬────────┬────────┬────────┬──────────┐
│  opcode  │   rd   │  rs1   │  rs2   │  (livre) │
│  4 bits  │ 4 bits │ 4 bits │ 4 bits │  16 bits │
└──────────┴────────┴────────┴────────┴──────────┘
```

**Tipo I:**
```
 31      28 27    24 23    20 19                 0
┌──────────┬────────┬────────┬────────────────────┐
│  opcode  │   rd   │  rs1   │     imediato       │
│  4 bits  │ 4 bits │ 4 bits │      20 bits       │
└──────────┴────────┴────────┴────────────────────┘
```

### Registradores

| Registrador | Convenção |
|-------------|-----------|
| R0          | Zero (hardwired, sempre 0) |
| R1 – R13    | Uso geral |
| R14 (RA)    | Endereço de retorno |
| R15 (RB)    | Base pointer / Stack pointer |

## Arquivos

| Arquivo | Descrição |
|---------|-----------|
| `ula.h` | Unidade Lógica e Aritmética (8 operações) |
| `banco_reg.h` | Banco de 16 registradores (2 leituras / 1 escrita) |
| `memoria.h` | Memória genérica (instruções e dados) |
| `controle.h` | Unidade de controle / decodificador de instruções |
| `datapath.h` | Datapath com Pipeline (inclui o Regsitrador IF/EX e lógica de Flush) |
| `testbench.cpp` | Testbench com programa exemplo |
| `Makefile` | Script de compilação |

## Como Compilar e Executar

### Instalar SystemC (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install libsystemc-dev
```

### Compilar
```bash
make
```

### Executar simulação
```bash
make run
```

### Visualizar formas de onda (requer GTKWave)
```bash
sudo apt install gtkwave
make wave
```

## Exemplo de Programa

O testbench executa o seguinte programa:

```asm
ADDI R1, R0, 5     ; R1 = 5
ADDI R2, R0, 3     ; R2 = 3
ADD  R3, R1, R2    ; R3 = 8
STORE R3, 100(R0)  ; mem[100] = 8
JUMP 4             ; loop infinito
```

Saída esperada:
```
R1 = 5
R2 = 3
R3 = 8
mem[100] = 8
```
