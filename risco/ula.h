#ifndef ULA_H
#define ULA_H

#include <systemc.h>

// Códigos de operação da ULA
#define ULA_ADD  0x0
#define ULA_SUB  0x1
#define ULA_AND  0x2
#define ULA_OR   0x3
#define ULA_XOR  0x4
#define ULA_NOT  0x5
#define ULA_SHL  0x6
#define ULA_SHR  0x7

SC_MODULE(ULA) {
    // Entradas
    sc_in<sc_uint<32>> A;       // Operando fonte 1
    sc_in<sc_uint<32>> B;       // Operando fonte 2
    sc_in<sc_uint<4>>  ctrl;    // Código da operação

    // Saídas
    sc_out<sc_uint<32>> resultado;
    sc_out<bool>        zero;    // Flag: resultado == 0
    sc_out<bool>        carry;   // Flag: carry out
    sc_out<bool>        negativo;// Flag: resultado < 0 (bit 31)

    SC_CTOR(ULA) {
        SC_METHOD(operar);
        sensitive << A << B << ctrl;
    }

    void operar() {
        sc_uint<33> res = 0;

        switch (ctrl.read()) {
            case ULA_ADD: res = A.read() + B.read(); break;
            case ULA_SUB: res = A.read() - B.read(); break;
            case ULA_AND: res = A.read() & B.read(); break;
            case ULA_OR:  res = A.read() | B.read(); break;
            case ULA_XOR: res = A.read() ^ B.read(); break;
            case ULA_NOT: res = ~A.read();            break;
            case ULA_SHL: res = A.read() << 1;        break;
            case ULA_SHR: res = A.read() >> 1;        break;
            default:      res = 0;                     break;
        }

        resultado.write(res.range(31, 0));
        zero.write(res.range(31, 0) == 0);
        carry.write(res[32]);
        negativo.write(res[31]);
    }
};

#endif