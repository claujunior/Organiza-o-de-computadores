#ifndef MEMORIA_H
#define MEMORIA_H

#include <systemc.h>

// Memória unificada de 256 palavras de 32 bits (1KB)
// Endereçamento por palavra

SC_MODULE(Memoria) {
    sc_in<sc_uint<32>> endereco;
    sc_in<sc_uint<32>> dado_entrada;
    sc_in<bool>        leitura;
    sc_in<bool>        escrita;
    sc_in<bool>        clk;

    sc_out<sc_uint<32>> dado_saida;

    sc_uint<32> mem[256];

    SC_CTOR(Memoria) {
        for (int i = 0; i < 256; i++) mem[i] = 0;

        SC_METHOD(ler);
        sensitive << endereco << leitura;

        SC_METHOD(escrever);
        sensitive << clk.pos();
    }

    // Leitura combinacional
    void ler() {
        if (leitura.read()) {
            sc_uint<32> addr = endereco.read();
            if (addr < 256)
                dado_saida.write(mem[addr]);
            else
                dado_saida.write(0);
        }
    }

    // Escrita síncrona
    void escrever() {
        if (escrita.read()) {
            sc_uint<32> addr = endereco.read();
            if (addr < 256)
                mem[addr] = dado_entrada.read();
        }
    }

    // Método auxiliar para carregar programa
    void carregar(sc_uint<32> instrucoes[], int n) {
        for (int i = 0; i < n && i < 256; i++)
            mem[i] = instrucoes[i];
    }
};

#endif
