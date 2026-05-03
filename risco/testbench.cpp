#include <systemc.h>
#include "datapath.h"
#include <iostream>
#include <iomanip>

// =============================================================================
//  Programa de teste — cobertura completa da ISA e casos de borda:
//
//  End | Instrução             | Hex          | O que testa
//  ----|------------------------|--------------|-------------------------------
//   0  | ADDI R1, R0, 5        | 0x81000005   | Básico; R1 = 5
//   1  | ADDI R2, R0, 3        | 0x82000003   | Básico; R2 = 3
//   2  | ADD  R3, R1, R2       | 0x03120000   | RAW R1+R2; R3 = 8
//   3  | SUB  R4, R3, R1       | 0x14310000   | RAW encadeado; R4 = 3
//   4  | AND  R5, R3, R2       | 0x25320000   | Lógica AND; R5 = 8&3 = 0
//   5  | OR   R6, R3, R2       | 0x36320000   | Lógica OR;  R6 = 8|3 = 11
//   6  | XOR  R7, R3, R2       | 0x47320000   | Lógica XOR; R7 = 8^3 = 11
//   7  | NOT  R8, R1, --       | 0x58100000   | Lógica NOT; R8 = ~5 = 0xFFFFFFFA
//   8  | SHL  R9, R2, --       | 0x69200000   | Shift left; R9 = 3<<1 = 6
//   9  | SHR  R10, R3, --      | 0x7A300000   | Shift right; R10 = 8>>1 = 4
//  10  | ADDI R11, R0, -1      | 0x8B0FFFFF   | Imediato negativo (ext. sinal)
//  11  | ADD  R0, R1, R2       | 0x00120000   | Escrita em R0 (deve ficar 0)
//  12  | STORE R3, 10(R0)      | 0xA300000A   | STORE via 3o porto; mem[10]=8
//  13  | LOAD  R12, 10(R0)     | 0x9C00000A   | LOAD; R12 = mem[10] = 8
//  14  | ADD  R0, R0, R0       | 0x00000000   | NOP — evita load-use hazard
//  15  | BEQ  R3, R1, 1        | 0xB3100001   | BEQ NOT tomado (8 != 5)
//  16  | BEQ  R1, R1, 1        | 0xB1100001   | BEQ TOMADO (5==5) -> PC=18
//  17  | ADDI R13, R0, 99      | 0x8D000063   | DEVE SER IGNORADO (flush)
//  18  | JUMP 18               | 0xC0000012   | Halt (loop infinito)
//
//  Resultados esperados:
//    R0=0  R1=5  R2=3  R3=8  R4=3  R5=0  R6=11  R7=11
//    R8=0xFFFFFFFA  R9=6  R10=4  R11=0xFFFFFFFF  R12=8  R13=0
//    mem[10] = 8
//
//  LIMITACAO CONHECIDA — Load-Use Hazard:
//    No pipeline de 2 estagios, o resultado de LOAD so esta disponivel no
//    banco de registradores no ciclo seguinte ao da execucao do LOAD.
//    Se a instrucao imediatamente apos um LOAD ler o registrador destino,
//    ela lera o valor ANTIGO (hazard RAW sem forwarding).
//    Solucao: inserir um NOP (ADD R0,R0,R0 = 0x00000000) entre o LOAD e
//    o primeiro uso do registrador carregado.
//    Exemplo aplicado: addr 13 (LOAD R12) seguido de addr 14 (NOP).
// =============================================================================

int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    Datapath dut("dut");
    dut.clk(clk);
    dut.reset(reset);

    sc_uint<32> programa[] = {
        0x81000005,  //  0: ADDI R1,  R0, 5       — R1 = 5
        0x82000003,  //  1: ADDI R2,  R0, 3       — R2 = 3
        0x03120000,  //  2: ADD  R3,  R1, R2      — R3 = 8  (RAW)
        0x14310000,  //  3: SUB  R4,  R3, R1      — R4 = 3  (RAW encadeado)
        0x25320000,  //  4: AND  R5,  R3, R2      — R5 = 0  (8 & 3)
        0x36320000,  //  5: OR   R6,  R3, R2      — R6 = 11 (8 | 3)
        0x47320000,  //  6: XOR  R7,  R3, R2      — R7 = 11 (8 ^ 3)
        0x58100000,  //  7: NOT  R8,  R1          — R8 = ~5 = 0xFFFFFFFA
        0x69200000,  //  8: SHL  R9,  R2          — R9 = 6  (3 << 1)
        0x7A300000,  //  9: SHR  R10, R3          — R10 = 4 (8 >> 1)
        0x8B0FFFFF,  // 10: ADDI R11, R0, -1      — R11 = 0xFFFFFFFF (ext. sinal)
        0x00120000,  // 11: ADD  R0,  R1, R2      — R0 deve ficar 0 (hardwired)
        0xA300000A,  // 12: STORE R3, 10(R0)      — mem[10] = 8
        0x9C00000A,  // 13: LOAD  R12, 10(R0)     — R12 = 8
        0x00000000,  // 14: NOP (ADD R0,R0,R0)    — evita load-use hazard
        0xB3100001,  // 15: BEQ  R3, R1, 1        — 8!=5 -> NOT tomado -> PC=16
        0xB1100001,  // 16: BEQ  R1, R1, 1        — 5==5 -> TOMADO    -> PC=18
        0x8D000063,  // 17: ADDI R13, R0, 99      — IGNORADO (flush do BEQ)
        0xC0000012,  // 18: JUMP 18               — halt (loop infinito)
    };
    dut.carregar_programa(programa, 19);

    // --- Trace VCD para GTKWave ---
    sc_trace_file *tf = sc_create_vcd_trace_file("risco_trace");
    sc_trace(tf, clk,                  "clk");
    sc_trace(tf, reset,                "reset");
    sc_trace(tf, dut.pc,               "PC");
    sc_trace(tf, dut.instrucao,        "instrucao_IF");
    sc_trace(tf, dut.if_ex_instrucao,  "instrucao_EX");
    sc_trace(tf, dut.sig_rd,           "rd");
    sc_trace(tf, dut.sig_rs1,          "rs1");
    sc_trace(tf, dut.sig_rs2,          "rs2");
    sc_trace(tf, dut.sig_rs1_data,     "rs1_data");
    sc_trace(tf, dut.sig_rs2_data,     "rs2_data");
    sc_trace(tf, dut.sig_rd_read_data, "rd_read_data");
    sc_trace(tf, dut.sig_ula_a,        "ula_A");
    sc_trace(tf, dut.sig_ula_b,        "ula_B");
    sc_trace(tf, dut.sig_ula_res,      "ula_resultado");
    sc_trace(tf, dut.sig_zero,         "flag_zero");
    sc_trace(tf, dut.sig_wb_data,      "wb_data");
    sc_trace(tf, dut.sig_reg_write,    "reg_write");
    sc_trace(tf, dut.sig_mem_read,     "mem_read");
    sc_trace(tf, dut.sig_mem_write,    "mem_write");
    sc_trace(tf, dut.sig_store_data,   "store_data");
    sc_trace(tf, dut.sig_branch,       "branch");
    sc_trace(tf, dut.sig_jump,         "jump");

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Processador RISCO -- Simulacao (Pipeline 2 Estagios)" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::left
              << std::setw(10) << "Tempo"
              << std::setw(5)  << "PC"
              << std::setw(14) << "Instr EX"
              << std::setw(6)  << "RD"
              << std::setw(12) << "ULA_A"
              << std::setw(12) << "ULA_B"
              << std::setw(12) << "ULA_Res"
              << std::setw(12) << "WB_Data"
              << std::endl;
    std::cout << std::string(83, '-') << std::endl;

    // Reset
    reset.write(true);
    sc_start(15, SC_NS);
    reset.write(false);

    // 25 ciclos: 19 instrucoes + fill de pipeline + folga no halt
    for (int i = 0; i < 25; i++) {
        sc_start(10, SC_NS);
        std::cout << std::left
                  << std::setw(10) << sc_time_stamp()
                  << std::setw(5)  << dut.pc.read()
                  << "0x" << std::hex << std::setfill('0') << std::setw(8)
                  << dut.if_ex_instrucao.read() << "  "
                  << std::dec << std::setfill(' ')
                  << std::setw(6)  << dut.sig_rd.read()
                  << std::setw(12) << dut.sig_ula_a.read()
                  << std::setw(12) << dut.sig_ula_b.read()
                  << std::setw(12) << dut.sig_ula_res.read()
                  << std::setw(12) << dut.sig_wb_data.read()
                  << std::endl;
    }

    // =========================================================
    //  Verificacao dos resultados
    // =========================================================
    std::cout << std::string(83, '-') << std::endl;

    uint32_t regs[16];
    for (int i = 0; i < 16; i++)
        regs[i] = (uint32_t)dut.banco->regs[i].read();
    uint32_t mem10 = (uint32_t)dut.mem_dados->mem[10];

    struct Caso { const char* desc; uint32_t obtido; uint32_t esperado; };
    Caso casos[] = {
        // Registradores basicos
        { "R1  ADDI +5              ", regs[1],  5           },
        { "R2  ADDI +3              ", regs[2],  3           },
        { "R3  ADD  R1+R2  (RAW)    ", regs[3],  8           },
        { "R4  SUB  R3-R1  (RAW)    ", regs[4],  3           },
        // Instrucoes logicas
        { "R5  AND  8&3             ", regs[5],  0           },
        { "R6  OR   8|3             ", regs[6],  11          },
        { "R7  XOR  8^3             ", regs[7],  11          },
        { "R8  NOT  ~5              ", regs[8],  0xFFFFFFFA  },
        // Shifts
        { "R9  SHL  3<<1            ", regs[9],  6           },
        { "R10 SHR  8>>1            ", regs[10], 4           },
        // Imediato negativo
        { "R11 ADDI R0,-1 (ext.sin) ", regs[11], 0xFFFFFFFF  },
        // Hardwired R0
        { "R0  apos ADD R0,R1,R2    ", regs[0],  0           },
        // LOAD / STORE
        { "mem[10] STORE R3         ", mem10,    8           },
        { "R12 LOAD  mem[10]        ", regs[12], 8           },
        // BEQ: R13 nao deve ser escrito (flush)
        { "R13 BEQ flush (esperado 0)", regs[13], 0          },
    };

    int total  = sizeof(casos) / sizeof(casos[0]);
    int passou = 0;

    std::cout << "\n=== Verificacao Detalhada ===" << std::endl;
    std::cout << std::left
              << std::setw(32) << "Teste"
              << std::setw(14) << "Obtido"
              << std::setw(14) << "Esperado"
              << "Resultado" << std::endl;
    std::cout << std::string(72, '-') << std::endl;

    for (int i = 0; i < total; i++) {
        bool ok = (casos[i].obtido == casos[i].esperado);
        if (ok) passou++;
        std::cout << std::left
                  << std::setw(32) << casos[i].desc
                  << "0x" << std::hex << std::setfill('0') << std::setw(8)
                  << casos[i].obtido   << "    "
                  << "0x" << std::setw(8)
                  << casos[i].esperado << "  "
                  << std::dec << std::setfill(' ')
                  << (ok ? "OK" : "FALHOU")
                  << std::endl;
    }

    std::cout << std::string(72, '-') << std::endl;
    std::cout << "\n=== Resultado Final: " << passou << "/" << total
              << " testes passaram ===" << std::endl;
    if (passou == total)
        std::cout << "  TODOS OS TESTES PASSARAM" << std::endl;
    else
        std::cout << "  FALHA EM UM OU MAIS TESTES" << std::endl;

    std::cout << "\n[AVISO] Load-Use Hazard: instrucao imediatamente apos LOAD\n"
                 "        le o valor antigo do registrador. Inserir NOP entre\n"
                 "        LOAD e o primeiro uso do resultado (ver addr 14).\n"
              << std::endl;

    sc_close_vcd_trace_file(tf);
    std::cout << "Trace salvo em risco_trace.vcd  "
                 "(visualizar: gtkwave risco_trace.vcd)" << std::endl;

    return (passou == total) ? 0 : 1;
}