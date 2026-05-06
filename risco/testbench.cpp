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

std::string decode(uint32_t instr) {
    uint8_t opcode = (instr >> 28) & 0xF;

    switch (opcode) {
        case 0x0: return "ADD";
        case 0x1: return "SUB";
        case 0x2: return "AND";
        case 0x3: return "OR";
        case 0x4: return "XOR";
        case 0x5: return "NOT";
        case 0x6: return "SHL";
        case 0x7: return "SHR";
        case 0x8: return "ADDI";
        case 0x9: return "LOAD";
        case 0xA: return "STORE";
        case 0xB: return "BEQ";
        case 0xC: return "JUMP";
        default:  return "UNK";
    }
}
int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    Datapath dut("dut");
    dut.clk(clk);
    dut.reset(reset);
    //contador
    sc_uint<32> programa[] = {

// CONTADOR
0x81000000,
0x81100001,
0x81100001,
0x81100001,

// MULTIPLICADOR
0x81000005,
0x82000003,
0x83000000,

0xB2000004,  // sai do loop quando R2==0
0x03310000,
0x822FFFFF,
0xC0000007,

// =================
// COMPARADOR
// =================
0x81000005,   // R1 = 5
0x82000003,   // R2 = 3
0x14210000,   // R4 = R1 - R2
0xB4000002,   // se igual pula
0xA100000A,   // STORE R1 (maior)
0xC0000012,   // pula pro fim
0xA100000A,   // STORE R1 (igual)

// =================
// HALT
// =================
0xC0000014

};

    dut.carregar_programa(programa, sizeof(programa)/sizeof(programa[0]));
    

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

   reset.write(true);
    sc_start(15, SC_NS);
    reset.write(false);

    // =========================
    // EXECUÇÃO PASSO A PASSO
    // =========================
    for (int i = 0; i < 31; i++) {
        sc_start(10, SC_NS);

        uint32_t if_instr = dut.instrucao.read();
        uint32_t ex_instr = dut.if_ex_instrucao.read();

        std::string if_op = decode(if_instr);
        std::string ex_op = decode(ex_instr);

        std::cout << "\n[ CICLO " << i << " ] =====================\n";

        // IF
        std::cout << "IF  -> PC=" << dut.pc.read()
                  << "  Instr=0x" << std::hex << if_instr
                  << " (" << if_op << ")\n";

        // EX
        std::cout << "EX  -> Instr=0x" << std::hex << ex_instr
                  << " (" << ex_op << ")\n";

        std::cout << std::dec;

        // ULA
        std::cout << "ULA -> A=" << dut.sig_ula_a.read()
                  << "  B=" << dut.sig_ula_b.read()
                  << "  RES=" << dut.sig_ula_res.read()
                  << "\n";

        // Escrita em registrador
        if (dut.sig_reg_write.read()) {
            std::cout << "REG -> R" << dut.sig_rd.read()
                      << " = " << dut.sig_wb_data.read() << "\n";
        }

        // Memória
        if (dut.sig_mem_write.read()) {
            std::cout << "MEM -> STORE valor="
                      << dut.sig_store_data.read() << "\n";
        }

        if (dut.sig_mem_read.read()) {
            std::cout << "MEM -> LOAD\n";
        }

        // Controle
        if (dut.sig_branch.read()) {
            std::cout << "BRANCH (zero=" << dut.sig_zero.read() << ")\n";
        }

        if (dut.sig_jump.read()) {
            std::cout << "JUMP\n";
        }

        std::cout << "-------------------------------------\n";
    }

    // =========================
    // ESTADO FINAL
    // =========================
    std::cout << "\n=== ESTADO FINAL ===\n";
    for (int i = 0; i < 8; i++) {
        std::cout << "R" << i << " = "
                  << dut.banco->regs[i].read() << "\n";
    }

    std::cout << "mem[10] = "
              << dut.mem_dados->mem[10] << "\n";

    sc_close_vcd_trace_file(tf);

    std::cout << "\nTrace salvo em risco_trace.vcd\n";
    std::cout << "Abra com: gtkwave risco_trace.vcd\n";

    return 0;
}