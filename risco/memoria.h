#ifndef MEMORIA_H
#define MEMORIA_H

#include <systemc.h>

// Memória genérica de 256 palavras de 32 bits (1KB)
// Endereçamento por palavra (word-addressed)
//
// CORREÇÃO:
//   dado_saida agora escreve 0 quando leitura=false, eliminando o
//   comportamento de latch que mantinha o último valor lido na saída.

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
            dado_saida.write(addr < 256 ? mem[addr] : sc_uint<32>(0));
        } else {
            dado_saida.write(0);  // sem comportamento de latch
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

    // Carrega programa/dados na memória antes da simulação
    void carregar(sc_uint<32> instrucoes[], int n) {
        for (int i = 0; i < n && i < 256; i++)
            mem[i] = instrucoes[i];
    }
};

#endif