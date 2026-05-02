#ifndef BANCO_REG_H
#define BANCO_REG_H

#include <systemc.h>

// O RISCO possui 16 registradores de uso geral (R0..R15)
// R0 é sempre 0 (hardwired zero)
// RA (R14) = registrador de retorno
// RB (R15) = base pointer / stack pointer

SC_MODULE(BancoRegistradores) {
    // Entradas de leitura (2 portas de leitura — máquina 3 endereços)
    sc_in<sc_uint<4>>  rs1_addr;   // Endereço fonte 1
    sc_in<sc_uint<4>>  rs2_addr;   // Endereço fonte 2

    // Entradas de escrita (1 porta de escrita)
    sc_in<sc_uint<4>>  rd_addr;    // Endereço destino
    sc_in<sc_uint<32>> rd_data;    // Dado a escrever
    sc_in<bool>        we;         // Write Enable
    sc_in<bool>        clk;

    // Saídas de leitura
    sc_out<sc_uint<32>> rs1_data;
    sc_out<sc_uint<32>> rs2_data;

    sc_uint<32> regs[16];

    SC_CTOR(BancoRegistradores) {
        // Inicializa todos os registradores com 0
        for (int i = 0; i < 16; i++) regs[i] = 0;

        SC_METHOD(leitura);
        sensitive << rs1_addr << rs2_addr;

        SC_METHOD(escrita);
        sensitive << clk.pos();
    }

    // Leitura combinacional
    void leitura() {
        sc_uint<4> a1 = rs1_addr.read();
        sc_uint<4> a2 = rs2_addr.read();
        rs1_data.write(regs[a1]);
        rs2_data.write(regs[a2]);
    }

    // Escrita síncrona na borda de subida do clock
    void escrita() {
        if (we.read()) {
            sc_uint<4> dst = rd_addr.read();
            if (dst != 0) // R0 é sempre 0, não pode ser escrito
                regs[dst] = rd_data.read();
        }
    }
};

#endif
