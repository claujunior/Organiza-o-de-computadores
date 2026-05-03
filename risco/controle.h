#ifndef CONTROLE_H
#define CONTROLE_H

#include <systemc.h>

/*
 * Formato de instrução RISCO (32 bits) — máquina 3 endereços:
 *
 * Tipo R (registrador):
 *  [31-28] opcode | [27-24] rd | [23-20] rs1 | [19-16] rs2 | [15-0] não usado
 *
 * Tipo I (imediato):
 *  [31-28] opcode | [27-24] rd | [23-20] rs1 | [19-0] imediato (20 bits, com sinal)
 *
 * Opcodes:
 *  0000 = ADD   (R) — rd = rs1 + rs2
 *  0001 = SUB   (R) — rd = rs1 - rs2
 *  0010 = AND   (R) — rd = rs1 & rs2
 *  0011 = OR    (R) — rd = rs1 | rs2
 *  0100 = XOR   (R) — rd = rs1 ^ rs2
 *  0101 = NOT   (R) — rd = ~rs1
 *  0110 = SHL   (R) — rd = rs1 << 1
 *  0111 = SHR   (R) — rd = rs1 >> 1
 *  1000 = ADDI  (I) — rd = rs1 + sign_extend(imm)
 *  1001 = LOAD  (I) — rd = mem[rs1 + sign_extend(imm)]
 *  1010 = STORE (I) — mem[rs1 + sign_extend(imm)] = rd
 *  1011 = BEQ   (I) — if (rd == rs1) PC += sign_extend(imm)
 *  1100 = JUMP  (I) — PC = sign_extend(imm)  [salto absoluto]
 */

SC_MODULE(UnidadeControle) {
    sc_in<sc_uint<32>> instrucao;

    // Campos decodificados
    sc_out<sc_uint<4>>  opcode;
    sc_out<sc_uint<4>>  rd;
    sc_out<sc_uint<4>>  rs1;
    sc_out<sc_uint<4>>  rs2;
    sc_out<sc_uint<20>> imediato;

    // Sinais de controle
    sc_out<bool>       reg_write;   // Habilita escrita no banco
    sc_out<bool>       mem_read;    // Leitura de memória de dados
    sc_out<bool>       mem_write;   // Escrita em memória de dados
    sc_out<bool>       mem_to_reg;  // WB vem da memória (LOAD)
    sc_out<bool>       use_imm;     // ULA usa imediato em vez de rs2
    sc_out<bool>       branch;      // Instrução de branch (BEQ)
    sc_out<bool>       jump;        // Instrução de jump (JUMP)
    sc_out<sc_uint<4>> ula_op;      // Operação da ULA

    SC_CTOR(UnidadeControle) {
        SC_METHOD(decodificar);
        sensitive << instrucao;
    }

    void decodificar() {
        sc_uint<32> instr = instrucao.read();

        sc_uint<4>  op  = instr.range(31, 28);
        sc_uint<4>  dst = instr.range(27, 24);
        sc_uint<4>  s1  = instr.range(23, 20);
        sc_uint<4>  s2  = instr.range(19, 16);
        sc_uint<20> imm = instr.range(19, 0);

        opcode.write(op);
        rd.write(dst);
        rs1.write(s1);
        rs2.write(s2);
        imediato.write(imm);

        // Defaults
        reg_write.write(false);
        mem_read.write(false);
        mem_write.write(false);
        mem_to_reg.write(false);
        use_imm.write(false);
        branch.write(false);
        jump.write(false);
        ula_op.write(0);

        switch (op) {
            case 0x0: // ADD
            case 0x1: // SUB
            case 0x2: // AND
            case 0x3: // OR
            case 0x4: // XOR
            case 0x5: // NOT
            case 0x6: // SHL
            case 0x7: // SHR
                reg_write.write(true);
                ula_op.write(op);
                break;

            case 0x8: // ADDI
                reg_write.write(true);
                use_imm.write(true);
                ula_op.write(0x0); // ADD
                break;

            case 0x9: // LOAD
                reg_write.write(true);
                mem_read.write(true);
                mem_to_reg.write(true);
                use_imm.write(true);
                ula_op.write(0x0); // ADD para calcular endereço
                break;

            case 0xA: // STORE — mem[rs1 + imm] = rd
                mem_write.write(true);
                use_imm.write(true);
                ula_op.write(0x0); // ADD para calcular endereço
                break;

            case 0xB: // BEQ — if (rd == rs1) PC += imm
                // O Datapath usa o terceiro porto do banco para ler rd,
                // e compara com rs1_data via SUB na ULA.
                branch.write(true);
                ula_op.write(0x1); // SUB para comparar (zero flag)
                break;

            case 0xC: // JUMP — PC = imm (salto absoluto)
                jump.write(true);
                break;

            default:
                break;
        }
    }
};

#endif