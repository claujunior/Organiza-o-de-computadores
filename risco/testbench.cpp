#include <systemc.h>
#include "datapath.h"
#include <iostream>
#include <iomanip>

// =============================================================================
//  Programa de teste — exercita os bugs corrigidos:
//
//  Endereço | Instrução          | Binário      | Motivo do teste
//  ---------|--------------------|--------------|---------------------------------
//    0      | ADDI R1, R0, 5    | 0x81000005   | Básico: R1 = 5
//    1      | ADDI R2, R0, 3    | 0x82000003   | Básico: R2 = 3
//    2      | ADD  R3, R1, R2   | 0x03120000   | RAW hazard: lê R1 e R2 imediatos
//    3      | ADDI R4, R3, 10   | 0x8431000A   | RAW hazard encadeado: lê R3
//    4      | STORE R3, 100(R0) | 0xA3000064   | Bug 2: STORE armazena rd (R3=8)
//    5      | LOAD  R5, 100(R0) | 0x95000064   | Verifica STORE: R5 deve ser 8
//    6      | SUB   R6, R4, R1  | 0x16410000   | R6 = R4(18) - R1(5) = 13
//    7      | JUMP  7           | 0xC0000007   | Loop infinito (halt)
//
//  Resultados esperados:
//    R1 = 5, R2 = 3, R3 = 8, R4 = 18, R5 = 8, R6 = 13
//    mem[100] = 8
// =============================================================================

int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    Datapath dut("dut");
    dut.clk(clk);
    dut.reset(reset);

    sc_uint<32> programa[] = {
        0x81000005,  // ADDI R1, R0, 5       — R1 = 5
        0x82000003,  // ADDI R2, R0, 3       — R2 = 3
        0x03120000,  // ADD  R3, R1, R2      — R3 = 8  (RAW: R1 e R2)
        0x8430000A,  // ADDI R4, R3, 10      — R4 = 18 (RAW: R3)
        0xA3000064,  // STORE R3, 100(R0)    — mem[100] = R3 = 8
        0x95000064,  // LOAD  R5, 100(R0)    — R5 = mem[100] = 8
        0x16410000,  // SUB   R6, R4, R1     — R6 = 18-5 = 13 (RAW: R4 e R1)
        0xC0000007   // JUMP  7              — loop infinito
    };
    dut.carregar_programa(programa, 8);

    // Habilita VCD para GTKWave
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

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Processador RISCO — Simulação (Pipeline 2 Estágios)" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::left
              << std::setw(10) << "Tempo"
              << std::setw(5)  << "PC"
              << std::setw(14) << "Instr EX"
              << std::setw(6)  << "RD"
              << std::setw(12) << "ULA_A"
              << std::setw(12) << "ULA_B"
              << std::setw(12) << "ULA_Res"
              << std::setw(10) << "WB_Data"
              << std::endl;
    std::cout << std::string(81, '-') << std::endl;

    // Reset
    reset.write(true);
    sc_start(15, SC_NS);
    reset.write(false);

    // Executa 16 ciclos (8 instruções + 2 de pipeline fill + folga)
    for (int i = 0; i < 16; i++) {
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
                  << std::setw(10) << dut.sig_wb_data.read()
                  << std::endl;
    }

    // =========================================================
    //  Verificação dos resultados
    // =========================================================
    std::cout << std::string(81, '-') << std::endl;
    std::cout << "\n=== Estado dos Registradores ===" << std::endl;

    const char* nomes[] = {
        "R0 (zero)", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
        "R8", "R9", "R10", "R11", "R12", "R13", "R14(RA)", "R15(RB)"
    };
    uint32_t esperados[] = {0, 5, 3, 8, 18, 8, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool tudo_ok = true;

    for (int i = 0; i < 16; i++) {
        uint32_t val = (uint32_t)dut.banco->regs[i].read();
        bool ok = (val == esperados[i]);
        if (val != 0 || esperados[i] != 0) { // só imprime relevantes
            std::cout << "  " << std::left << std::setw(10) << nomes[i]
                      << " = " << std::setw(5) << val
                      << " (esperado: " << esperados[i] << ")  "
                      << (ok ? "OK" : "FALHOU") << std::endl;
            if (!ok) tudo_ok = false;
        }
    }

    std::cout << "\n=== Memória ===" << std::endl;
    uint32_t mem100 = (uint32_t)dut.mem_dados->mem[100];
    bool mem_ok = (mem100 == 8);
    std::cout << "  mem[100] = " << mem100 << " (esperado: 8)  "
              << (mem_ok ? "OK" : "FALHOU") << std::endl;
    if (!mem_ok) tudo_ok = false;

    std::cout << "\n=== Resultado Final ===" << std::endl;
    std::cout << (tudo_ok ? "  TODOS OS TESTES PASSARAM" : "  FALHA EM UM OU MAIS TESTES")
              << std::endl;

    sc_close_vcd_trace_file(tf);
    std::cout << "\nTrace salvo em risco_trace.vcd (visualizar com: gtkwave risco_trace.vcd)"
              << std::endl;

    return tudo_ok ? 0 : 1;
}