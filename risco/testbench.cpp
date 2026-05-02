#include <systemc.h>
#include "datapath.h"
#include <iostream>
#include <iomanip>

/*
 * Programa de teste — calcula 5 + 3 e armazena em memória:
 *
 *  ADDI R1, R0, 5     ; R1 = 0 + 5 = 5
 *  ADDI R2, R0, 3     ; R2 = 0 + 3 = 3
 *  ADD  R3, R1, R2    ; R3 = R1 + R2 = 8
 *  STORE R3, R0, 100  ; mem[0 + 100] = R3
 *  JUMP 4             ; loop (fica no endereço 4)
 */

int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> reset;

    Datapath dut("dut");
    dut.clk(clk);
    dut.reset(reset);

    // Programa de teste
    sc_uint<32> programa[] = {
        0x81000005,  // ADDI R1, R0, 5
        0x82000003,  // ADDI R2, R0, 3
        0x03120000,  // ADD  R3, R1, R2
        0xA3000064,  // STORE R3, 100(R0)
        0xC0000004   // JUMP 4 (loop)
    };
    dut.carregar_programa(programa, 5);

    // Habilita VCD para visualização com GTKWave
    sc_trace_file *tf = sc_create_vcd_trace_file("risco_trace");
    sc_trace(tf, clk,                 "clk");
    sc_trace(tf, reset,               "reset");
    sc_trace(tf, dut.pc,              "PC");
    sc_trace(tf, dut.instrucao,       "instrucao_IF");
    sc_trace(tf, dut.if_ex_instrucao, "instrucao_EX"); // Novo sinal do pipeline no trace
    sc_trace(tf, dut.sig_rd,          "rd");
    sc_trace(tf, dut.sig_rs1,         "rs1");
    sc_trace(tf, dut.sig_rs2,         "rs2");
    sc_trace(tf, dut.sig_rs1_data,    "rs1_data");
    sc_trace(tf, dut.sig_rs2_data,    "rs2_data");
    sc_trace(tf, dut.sig_ula_res,     "ula_resultado");
    sc_trace(tf, dut.sig_zero,        "flag_zero");
    sc_trace(tf, dut.sig_wb_data,     "wb_data");
    sc_trace(tf, dut.sig_reg_write,   "reg_write");
    sc_trace(tf, dut.sig_mem_read,    "mem_read");
    sc_trace(tf, dut.sig_mem_write,   "mem_write");

    std::cout << "========================================" << std::endl;
    std::cout << "  Simulação Processador RISCO (Pipeline 2 Estágios)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::left
              << std::setw(8)  << "Tempo"
              << std::setw(6)  << "PC"
              << std::setw(14) << "Instrução EX"
              << std::setw(8)  << "RD"
              << std::setw(12) << "ULA_Res"
              << std::setw(10) << "WB_Data"
              << std::endl;
    std::cout << std::string(58, '-') << std::endl;

    // Reset
    reset.write(true);
    sc_start(15, SC_NS);
    reset.write(false);

    // Executa 12 ciclos (aumentado para compensar o preenchimento do pipeline)
    for (int i = 0; i < 12; i++) {
        sc_start(10, SC_NS);
        std::cout << std::left
                  << std::setw(6)  << sc_time_stamp()
                  << std::setw(6)  << dut.pc.read()
                  // Imprime a instrução no estágio EX, não a de IF
                  << "0x" << std::hex << std::setw(12) << dut.if_ex_instrucao.read()
                  << std::dec
                  << std::setw(8)  << dut.sig_rd.read()
                  << std::setw(12) << dut.sig_ula_res.read()
                  << std::setw(10) << dut.sig_wb_data.read()
                  << std::endl;
    }

    std::cout << std::string(58, '-') << std::endl;
    std::cout << "\nEstado dos Registradores ao final:" << std::endl;
    for (int i = 0; i < 8; i++) {
        std::cout << "  R" << i << " = " << dut.banco->regs[i] << std::endl;
    }

    std::cout << "\nMemória[100] = " << dut.mem_dados->mem[100]
              << " (esperado: 8)" << std::endl;

    sc_close_vcd_trace_file(tf);
    std::cout << "\nTrace salvo em risco_trace.vcd (abrir com GTKWave)" << std::endl;

    return 0;
}