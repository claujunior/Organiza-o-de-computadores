# Processador RISCO — Implementação em SystemC

## Arquitetura

Implementação do **datapath básico** do processador RISCO (Inspirado no Intel 8086).

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
| `banco_reg.h` | Banco de 16 registradores (3 leituras / 1 escrita) |
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

### Visualizar formas de onda (requer GTKWave)
```bash
sudo apt install gtkwave
```

```bash
cd risco
make
./risco_sim
```

## Limitações Conhecidas do Pipeline

### Load-Use Hazard
O pipeline possui **2 estágios** (IF e EX+MEM+WB). O resultado de uma instrução
`LOAD` só é escrito no banco de registradores ao final do estágio EX. A instrução
**imediatamente seguinte** ao LOAD já está em IF nesse mesmo ciclo e ao entrar em
EX lerá o valor **antigo** do registrador — não o valor recém carregado.

**Solução:** inserir um NOP (`ADD R0, R0, R0` = `0x00000000`) entre o `LOAD` e
qualquer instrução que use o registrador destino.

```asm
LOAD  R1, 0(R0)      ; carrega mem[0] em R1
ADD   R0, R0, R0     ; NOP obrigatorio (evita hazard)
ADD   R2, R1, R3     ; agora R1 esta disponivel
```

### Branch Penalty
Quando um `BEQ` é tomado, a instrução já buscada no ciclo seguinte (em IF) é
descartada via **flush do pipeline**, resultando em **1 ciclo de bolha** por
branch tomado. Branches não tomados não introduzem penalidade.

### Diagrama do Datapath
O diagrama `Datapath.jpeg` representa a arquitetura original de 2 portos de
leitura. Após as correções, o banco de registradores possui **3 portos de
leitura** (rs1, rs2 e r3 para `rd`) e o estágio EX inclui um **MUX adicional
no operando A** da ULA (para suporte ao `BEQ`). Esses elementos não estão
refletidos na figura.