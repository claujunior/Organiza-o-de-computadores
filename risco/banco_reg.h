#ifndef BANCO_REG_H
#define BANCO_REG_H

#include <systemc.h>

// Possui 16 registradores (R0..R15)
// R0 é sempre 0
// RA (R14) = registrador de retorno
// RB (R15) = base pointer / stack pointer
//
// CORREÇÕES:
//   [Bug 3] regs[] agora é sc_signal<sc_uint<32>> em vez de sc_uint<32> puro.
//            leitura() fica na lista de sensibilidade de cada reg, garantindo
//            que a saída seja atualizada imediatamente após escrita síncrona,
//            eliminando o hazard RAW sem precisar de stall ou forwarding externo.
//   [Bug 2] Terceiro porto de leitura (r3_addr / r3_data) adicionado para que
//            o Datapath possa ler o registrador `rd` separadamente, necessário
//            para STORE (dados a armazenar) e BEQ (comparação rd == rs1).

SC_MODULE(BancoRegistradores) {
    // --- Portas de leitura ---
    sc_in<sc_uint<4>>  rs1_addr;   // Fonte 1 (ULA operando A)
    sc_in<sc_uint<4>>  rs2_addr;   // Fonte 2 (ULA operando B)
    sc_in<sc_uint<4>>  r3_addr;    // Terceiro porto: lê rd (para STORE e BEQ)

    // --- Porta de escrita ---
    sc_in<sc_uint<4>>  rd_addr;
    sc_in<sc_uint<32>> rd_data;
    sc_in<bool>        we;
    sc_in<bool>        clk;

    // --- Saídas de leitura ---
    sc_out<sc_uint<32>> rs1_data;
    sc_out<sc_uint<32>> rs2_data;
    sc_out<sc_uint<32>> r3_data;   // Saída do terceiro porto

    // Registradores internos como sc_signal para reatividade automática
    sc_signal<sc_uint<32>> regs[16];

    SC_CTOR(BancoRegistradores) {
        for (int i = 0; i < 16; i++) regs[i].write(0);

        SC_METHOD(leitura);
        sensitive << rs1_addr << rs2_addr << r3_addr;
        // Adicionando todos os registradores à sensibilidade:
        // quando escrita() atualiza regs[i], leitura() é re-disparado
        // automaticamente, sem necessidade de forwarding externo.
        for (int i = 0; i < 16; i++) sensitive << regs[i];

        SC_METHOD(escrita);
        sensitive << clk.pos();
    }

    // Leitura combinacional — reativa a mudanças de endereço E de conteúdo
    void leitura() {
        sc_uint<4> a1 = rs1_addr.read();
        sc_uint<4> a2 = rs2_addr.read();
        sc_uint<4> a3 = r3_addr.read();
        rs1_data.write(regs[a1].read());
        rs2_data.write(regs[a2].read());
        r3_data.write(regs[a3].read());
    }

    // Escrita síncrona na borda de subida do clock
    void escrita() {
        if (we.read()) {
            sc_uint<4> dst = rd_addr.read();
            if (dst != 0)               // R0 é sempre 0
                regs[dst].write(rd_data.read());
        }
    }
};

#endif