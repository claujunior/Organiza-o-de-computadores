#ifndef DATAPATH_H
#define DATAPATH_H

#include <systemc.h>
#include "ula.h"
#include "banco_reg.h"
#include "memoria.h"
#include "controle.h"

SC_MODULE(Datapath) {
    sc_in<bool> clk;
    sc_in<bool> reset;

    // Sinais internos
    // =========================================================

    // PC
    sc_signal<sc_uint<32>> pc, pc_next, pc_plus1;

    // Instrução (Estágio Busca - IF)
    sc_signal<sc_uint<32>> instrucao;
    
    // Registrador de Pipeline (Barreira IF/EX)
    sc_signal<sc_uint<32>> if_ex_instrucao; 

    // Campos decodificados
    sc_signal<sc_uint<4>>  sig_opcode, sig_rd, sig_rs1, sig_rs2;
    sc_signal<sc_uint<20>> sig_imm;

    // Sinais de controle
    sc_signal<bool>        sig_reg_write, sig_mem_read, sig_mem_write;
    sc_signal<bool>        sig_mem_to_reg, sig_use_imm, sig_branch, sig_jump;
    sc_signal<sc_uint<4>>  sig_ula_op;
    sc_signal<bool>        sig_instr_read;

    // Banco de registradores
    sc_signal<sc_uint<32>> sig_rs1_data, sig_rs2_data;

    // ULA
    sc_signal<sc_uint<32>> sig_ula_b, sig_ula_res;
    sc_signal<bool>        sig_zero, sig_carry, sig_negativo;

    // Memória de dados
    sc_signal<sc_uint<32>> sig_mem_data;

    // Write-back
    sc_signal<sc_uint<32>> sig_wb_data;

    // Instâncias dos módulos
    // =========================================================
    UnidadeControle *ctrl;
    BancoRegistradores *banco;
    ULA *ula;
    Memoria *mem_instrucoes;
    Memoria *mem_dados;

    SC_CTOR(Datapath) {
        // --- Instancia módulos ---
        ctrl           = new UnidadeControle("ctrl");
        banco          = new BancoRegistradores("banco");
        ula            = new ULA("ula");
        mem_instrucoes = new Memoria("mem_instrucoes");
        mem_dados      = new Memoria("mem_dados");

        // --- Unidade de Controle ---
        // Lê do registrador de pipeline em vez da saída direta da memória
        ctrl->instrucao(if_ex_instrucao); 
        ctrl->opcode(sig_opcode);
        ctrl->rd(sig_rd);
        ctrl->rs1(sig_rs1);
        ctrl->rs2(sig_rs2);
        ctrl->imediato(sig_imm);
        ctrl->reg_write(sig_reg_write);
        ctrl->mem_read(sig_mem_read);
        ctrl->mem_write(sig_mem_write);
        ctrl->mem_to_reg(sig_mem_to_reg);
        ctrl->use_imm(sig_use_imm);
        ctrl->branch(sig_branch);
        ctrl->jump(sig_jump);
        ctrl->ula_op(sig_ula_op);

        // --- Banco de Registradores ---
        banco->clk(clk);
        banco->rs1_addr(sig_rs1);
        banco->rs2_addr(sig_rs2);
        banco->rd_addr(sig_rd);
        banco->rd_data(sig_wb_data);
        banco->we(sig_reg_write);
        banco->rs1_data(sig_rs1_data);
        banco->rs2_data(sig_rs2_data);

        // --- ULA ---
        ula->A(sig_rs1_data);
        ula->B(sig_ula_b);
        ula->ctrl(sig_ula_op);
        ula->resultado(sig_ula_res);
        ula->zero(sig_zero);
        ula->carry(sig_carry);
        ula->negativo(sig_negativo);

    // Memória de instruções
        sig_instr_read.write(true);
        mem_instrucoes->clk(clk);
        mem_instrucoes->endereco(pc);
        mem_instrucoes->dado_entrada(sig_ula_res); // não usado
        mem_instrucoes->leitura(sig_instr_read);   // sempre lendo
        mem_instrucoes->escrita(sig_zero);         // nunca escrevendo
        mem_instrucoes->dado_saida(instrucao);

        // --- Memória de Dados ---
        mem_dados->clk(clk);
        mem_dados->endereco(sig_ula_res);
        mem_dados->dado_entrada(sig_rs2_data);
        mem_dados->leitura(sig_mem_read);
        mem_dados->escrita(sig_mem_write);
        mem_dados->dado_saida(sig_mem_data);

        // --- Processos combinacionais ---
        SC_METHOD(mux_ula_b);
        sensitive << sig_use_imm << sig_rs2_data << sig_imm;

        SC_METHOD(mux_wb);
        sensitive << sig_mem_to_reg << sig_ula_res << sig_mem_data;

        SC_METHOD(calc_pc_next);
        sensitive << pc << sig_zero << sig_branch << sig_jump << sig_imm;

        // --- Processo síncrono: atualiza PC e Pipeline ---
        SC_METHOD(atualiza_pc);
        sensitive << clk.pos();
    }

    // MUX: segundo operando da ULA (registrador ou imediato)
    void mux_ula_b() {
        if (sig_use_imm.read())
            sig_ula_b.write(sc_uint<32>(sig_imm.read())); // zero-extend 20→32
        else
            sig_ula_b.write(sig_rs2_data.read());
    }

    // MUX: write-back (resultado ULA ou dado da memória)
    void mux_wb() {
        if (sig_mem_to_reg.read())
            sig_wb_data.write(sig_mem_data.read());
        else
            sig_wb_data.write(sig_ula_res.read());
    }

    // Calcula próximo PC
    void calc_pc_next() {
        sc_uint<32> cur = pc.read();
        sc_uint<32> imm = sc_uint<32>(sig_imm.read());

        if (sig_jump.read()) {
            pc_next.write(imm);                          // JUMP absoluto
        } else if (sig_branch.read() && sig_zero.read()) {
            pc_next.write(cur + imm);                   // BEQ tomado
        } else {
            pc_next.write(cur + 1);                     // Sequencial
        }
    }

    // Atualiza PC e Pipeline na borda do clock
    void atualiza_pc() {
        if (reset.read()) {
            pc.write(0);
            if_ex_instrucao.write(0); // Limpa o pipeline no reset
        } else {
            pc.write(pc_next.read());
            
            // Tratamento de Hazard de Controle (Flush)
            if (sig_jump.read() || (sig_branch.read() && sig_zero.read())) {
                // Se houve pulo, a instrução logo atrás é descartada 
                if_ex_instrucao.write(0); 
            } else {
                // Caso contrário, a instrução buscada avança para Execução
                if_ex_instrucao.write(instrucao.read());
            }
        }
    }

    // Método auxiliar para carregar programa na memória de instruções
    void carregar_programa(sc_uint<32> prog[], int n) {
        mem_instrucoes->carregar(prog, n);
    }
};

#endif