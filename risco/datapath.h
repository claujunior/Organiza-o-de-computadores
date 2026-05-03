#ifndef DATAPATH_H
#define DATAPATH_H

#include <systemc.h>
#include "ula.h"
#include "banco_reg.h"
#include "memoria.h"
#include "controle.h"

// =============================================================================
//  Datapath — Pipeline 2 Estágios (IF / EX+MEM+WB)
// =============================================================================
//  CORREÇÕES APLICADAS:
//
//  [Bug 1] Memória de instruções: sinal de leitura estava conectado ao pino
//          `reset` (lendo apenas durante reset) e escrita ao `zero` da ULA
//          (poderia corromper instruções).
//          FIX: sig_always_true  → leitura permanente da mem. de instruções
//               sig_always_false → escrita nunca ocorre na mem. de instruções
//
//  [Bug 2] STORE armazenava sig_rs2_data em vez do valor do registrador `rd`.
//          FIX: terceiro porto de leitura no banco (r3_addr / r3_data) lê `rd`.
//               mux_store_data() seleciona r3_data quando mem_write=true.
//
//  [Bug 3] Hazard RAW: leitura() do banco não reativava quando regs[] mudava.
//          FIX: regs[] virou sc_signal<sc_uint<32>> no banco; leitura() agora
//               está na lista de sensibilidade de todos os registradores,
//               atualizando as saídas automaticamente após qualquer escrita.
//
//  [Melhoria] Imediato com extensão de sinal (20 → 32 bits, signed).
//             Permite offsets negativos em LOAD/STORE e branches para trás.
//
//  [Melhoria] BEQ corrigido: ULA A = r3_data (rd), ULA B = rs1_data.
//             Resultado zero = (rd == rs1). mux_ula_a() realiza a seleção.
// =============================================================================

SC_MODULE(Datapath) {
    sc_in<bool> clk;
    sc_in<bool> reset;

    // =========================================================
    //  Sinais internos
    // =========================================================

    // Sinais permanentes para controle de memória de instruções
    sc_signal<bool> sig_always_true;
    sc_signal<bool> sig_always_false;

    // PC
    sc_signal<sc_uint<32>> pc, pc_next;

    // Instrução buscada (Estágio IF)
    sc_signal<sc_uint<32>> instrucao;

    // Registrador de Pipeline IF/EX
    sc_signal<sc_uint<32>> if_ex_instrucao;

    // Campos decodificados
    sc_signal<sc_uint<4>>  sig_opcode, sig_rd, sig_rs1, sig_rs2;
    sc_signal<sc_uint<20>> sig_imm;

    // Sinais de controle
    sc_signal<bool>        sig_reg_write, sig_mem_read, sig_mem_write;
    sc_signal<bool>        sig_mem_to_reg, sig_use_imm, sig_branch, sig_jump;
    sc_signal<sc_uint<4>>  sig_ula_op;

    // Banco de registradores
    sc_signal<sc_uint<32>> sig_rs1_data, sig_rs2_data;
    sc_signal<sc_uint<32>> sig_rd_read_data; // terceiro porto: valor atual de rd

    // ULA
    sc_signal<sc_uint<32>> sig_ula_a;  // operando A após mux (rs1_data ou rd_data para BEQ)
    sc_signal<sc_uint<32>> sig_ula_b;  // operando B após mux (rs2_data, imm ou rs1_data para BEQ)
    sc_signal<sc_uint<32>> sig_ula_res;
    sc_signal<bool>        sig_zero, sig_carry, sig_negativo;

    // Memória de dados
    sc_signal<sc_uint<32>> sig_mem_data;
    sc_signal<sc_uint<32>> sig_store_data; // dado a escrever na memória (rd, via r3)

    // Write-back
    sc_signal<sc_uint<32>> sig_wb_data;

    // =========================================================
    //  Módulos
    // =========================================================
    UnidadeControle    *ctrl;
    BancoRegistradores *banco;
    ULA                *ula;
    Memoria            *mem_instrucoes;
    Memoria            *mem_dados;

    SC_CTOR(Datapath) {
        // --- Sinais permanentes (Bug 1) ---
        sig_always_true.write(true);
        sig_always_false.write(false);

        // --- Instancia módulos ---
        ctrl           = new UnidadeControle("ctrl");
        banco          = new BancoRegistradores("banco");
        ula            = new ULA("ula");
        mem_instrucoes = new Memoria("mem_instrucoes");
        mem_dados      = new Memoria("mem_dados");

        // --- Unidade de Controle ---
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
        banco->r3_addr(sig_rd);          // [Bug 2] terceiro porto lê rd
        banco->rd_addr(sig_rd);
        banco->rd_data(sig_wb_data);
        banco->we(sig_reg_write);
        banco->rs1_data(sig_rs1_data);
        banco->rs2_data(sig_rs2_data);
        banco->r3_data(sig_rd_read_data); // [Bug 2] valor de rd disponível

        // --- ULA ---
        // Agora usa sig_ula_a (muxado) em vez de sig_rs1_data diretamente
        ula->A(sig_ula_a);
        ula->B(sig_ula_b);
        ula->ctrl(sig_ula_op);
        ula->resultado(sig_ula_res);
        ula->zero(sig_zero);
        ula->carry(sig_carry);
        ula->negativo(sig_negativo);

        // --- Memória de Instruções ---
        // [Bug 1] leitura = always_true, escrita = always_false
        mem_instrucoes->clk(clk);
        mem_instrucoes->endereco(pc);
        mem_instrucoes->dado_entrada(sig_ula_res); // não usado
        mem_instrucoes->leitura(sig_always_true);  // CORRIGIDO: sempre habilitada
        mem_instrucoes->escrita(sig_always_false); // CORRIGIDO: nunca escreve
        mem_instrucoes->dado_saida(instrucao);

        // --- Memória de Dados ---
        // [Bug 2] dado_entrada agora vem de sig_store_data (muxado)
        mem_dados->clk(clk);
        mem_dados->endereco(sig_ula_res);
        mem_dados->dado_entrada(sig_store_data);   // CORRIGIDO: usa r3_data para STORE
        mem_dados->leitura(sig_mem_read);
        mem_dados->escrita(sig_mem_write);
        mem_dados->dado_saida(sig_mem_data);

        // --- Processos combinacionais ---
        SC_METHOD(mux_ula_a);
        sensitive << sig_branch << sig_rs1_data << sig_rd_read_data;

        SC_METHOD(mux_ula_b);
        sensitive << sig_branch << sig_use_imm << sig_rs2_data
                  << sig_rs1_data << sig_imm;

        SC_METHOD(mux_store_data);
        sensitive << sig_mem_write << sig_rd_read_data << sig_rs2_data;

        SC_METHOD(mux_wb);
        sensitive << sig_mem_to_reg << sig_ula_res << sig_mem_data;

        SC_METHOD(calc_pc_next);
        sensitive << pc << sig_zero << sig_branch << sig_jump << sig_imm;

        // --- Processo síncrono: atualiza PC e pipeline ---
        SC_METHOD(atualiza_pc);
        sensitive << clk.pos();
    }

    // =========================================================
    //  MUX: operando A da ULA
    //  Normal: A = rs1_data
    //  BEQ:    A = rd_data (r3) — compara rd com rs1
    // =========================================================
    void mux_ula_a() {
        if (sig_branch.read())
            sig_ula_a.write(sig_rd_read_data.read()); // BEQ: A = rd
        else
            sig_ula_a.write(sig_rs1_data.read());     // Normal: A = rs1
    }

    // =========================================================
    //  MUX: operando B da ULA
    //  BEQ:      B = rs1_data   (compara A=rd contra B=rs1)
    //  use_imm:  B = sign_extend(imm)  [CORRIGIDO: com extensão de sinal]
    //  padrão:   B = rs2_data
    // =========================================================
    void mux_ula_b() {
        if (sig_branch.read()) {
            // BEQ: B = rs1_data para que ULA compute rd - rs1
            sig_ula_b.write(sig_rs1_data.read());
        } else if (sig_use_imm.read()) {
            // Extensão de sinal de 20 para 32 bits (CORRIGIDO)
            sc_int<20> simm = sc_int<20>(sig_imm.read());
            sig_ula_b.write(sc_uint<32>((sc_int<32>)simm));
        } else {
            sig_ula_b.write(sig_rs2_data.read());
        }
    }

    // =========================================================
    //  MUX: dado a escrever na memória de dados
    //  STORE (mem_write=true): usa r3_data = regs[rd]   [Bug 2 corrigido]
    //  outros: usa rs2_data (não chega a ser escrito)
    // =========================================================
    void mux_store_data() {
        if (sig_mem_write.read())
            sig_store_data.write(sig_rd_read_data.read()); // rd → mem
        else
            sig_store_data.write(sig_rs2_data.read());
    }

    // =========================================================
    //  MUX: write-back (resultado ULA ou dado da memória)
    // =========================================================
    void mux_wb() {
        if (sig_mem_to_reg.read())
            sig_wb_data.write(sig_mem_data.read());
        else
            sig_wb_data.write(sig_ula_res.read());
    }

    // =========================================================
    //  Calcula próximo PC
    //  JUMP:         PC = sign_extend(imm)     [salto absoluto]
    //  BEQ tomado:   PC = PC + sign_extend(imm)
    //  sequencial:   PC = PC + 1
    // =========================================================
    void calc_pc_next() {
        sc_uint<32> cur = pc.read();
        // Extensão de sinal do imediato para cálculo de salto
        sc_int<20>  simm = sc_int<20>(sig_imm.read());
        sc_uint<32> offset = sc_uint<32>((sc_int<32>)simm);

        if (sig_jump.read()) {
            pc_next.write(offset);                      // JUMP absoluto
        } else if (sig_branch.read() && sig_zero.read()) {
            pc_next.write(cur + offset);                // BEQ tomado
        } else {
            pc_next.write(cur + 1);                     // Sequencial
        }
    }

    // =========================================================
    //  Atualiza PC e pipeline na borda do clock
    // =========================================================
    void atualiza_pc() {
        if (reset.read()) {
            pc.write(0);
            if_ex_instrucao.write(0);
        } else {
            pc.write(pc_next.read());

            // Flush do pipeline em caso de salto 
            if (sig_jump.read() || (sig_branch.read() && sig_zero.read())) {
                if_ex_instrucao.write(0);
            } else {
                if_ex_instrucao.write(instrucao.read());
            }
        }
    }

    void carregar_programa(sc_uint<32> prog[], int n) {
        mem_instrucoes->carregar(prog, n);
    }
};

#endif