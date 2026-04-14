#pragma once
#include "bvm.hpp"
#include "opcode.hpp"
#include "vm.hpp"
namespace bvm {
inline bool is_conditional_jmp(OPCODE op) {
    switch (op) {
    case OPCODE::JE:
    case OPCODE::JNE:
    case OPCODE::JGT:
    case OPCODE::JLT:
    case OPCODE::JLE:
    case OPCODE::JGE:
    case OPCODE::JC:
    case OPCODE::JNC:
        return true;
    default:
        return false;
    }
}
class optimiser {
    program &prog;
    std::vector<instruction> &instructions;
    void mark_reachable(std::vector<bool> &reachable, size_t ip) {
        while (ip < instructions.size()) {
            if (reachable[ip]) {
                return;
            }
            reachable[ip] = true;
            if (instructions[ip].op == OPCODE::HALT || instructions[ip].op == OPCODE::RET) {
                return;
            } else if (is_conditional_jmp(instructions[ip].op) || instructions[ip].op == OPCODE::CALL) {
                mark_reachable(reachable, instructions[ip].operands[0]);
            } else if (instructions[ip].op == OPCODE::JMP) {
                mark_reachable(reachable, instructions[ip].operands[0]);
                return;
            }
            ip++;
        }
    }
    void unreachable_to_nop() {
        std::vector<bool> reachable(instructions.size(), false);
        mark_reachable(reachable, 0);
        for (const auto& [name, func_ip] : prog.exported_functions) {
            mark_reachable(reachable, func_ip);
        }
        for (size_t i = 0; i < instructions.size(); i++) {
            if (!reachable[i]) {
                instructions[i].op = OPCODE::NOP;
            }
        }
    }
    bool match_at(ssize_t ind, const std::vector<OPCODE> &v) {
        for (ssize_t i = 0; i < v.size(); i++) {
            if (i + ind >= instructions.size() || v[i] != instructions[i + ind].op)
                return false;
        }
        return true;
    }
    void remove_all_nops() {
        std::vector<instruction> compacted;
        std::vector<uint64_t> mapping(instructions.size(), 0);

        for (size_t i = 0; i < instructions.size(); i++) {
            mapping[i] = compacted.size();
            if (instructions[i].op != OPCODE::NOP) {
                compacted.push_back(instructions[i]);
            }
        }

        for (auto &inst : compacted) {
            switch (inst.op) {
            case OPCODE::JMP:
            case OPCODE::JE:
            case OPCODE::JNE:
            case OPCODE::JGT:
            case OPCODE::JLT:
            case OPCODE::JLE:
            case OPCODE::JGE:
            case OPCODE::JC:
            case OPCODE::JNC:
            case OPCODE::CALL:
                if (inst.operands[0] < mapping.size()) {
                    inst.operands[0] = mapping[inst.operands[0]];
                }
                break;
            default:
                break;
            }
        }
        for (auto &[name, ip] : prog.exported_functions) {
            ip = mapping[ip];
        }
        instructions.swap(compacted);
    }
    void peephole() {
        for (ssize_t i = 0; i < instructions.size(); i++) {
            if (match_at(i, {OPCODE::DECLARE, OPCODE::STORE})) {
                instructions[i].op = OPCODE::DECLARE_INIT;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::LOAD, OPCODE::PUSH, OPCODE::I32_ADD, OPCODE::STORE}) &&
                       instructions[i + 1].operands[0] == 1) {
                instructions[i].op = OPCODE::I32_INC;
                instructions[i + 1].op = OPCODE::NOP;
                instructions[i + 2].op = OPCODE::NOP;
                instructions[i + 3].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::LOAD, OPCODE::PUSH, OPCODE::I64_ADD, OPCODE::STORE}) &&
                       instructions[i + 1].operands[0] == 1) {
                instructions[i].op = OPCODE::I64_INC;
                instructions[i + 1].op = OPCODE::NOP;
                instructions[i + 2].op = OPCODE::NOP;
                instructions[i + 3].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::LOAD, OPCODE::PUSH, OPCODE::I32_SUB, OPCODE::STORE}) &&
                       instructions[i + 1].operands[0] == 1) {
                instructions[i].op = OPCODE::I32_DEC;
                instructions[i + 1].op = OPCODE::NOP;
                instructions[i + 2].op = OPCODE::NOP;
                instructions[i + 3].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::LOAD, OPCODE::PUSH, OPCODE::I64_SUB, OPCODE::STORE}) &&
                       instructions[i + 1].operands[0] == 1) {
                instructions[i].op = OPCODE::I64_DEC;
                instructions[i + 1].op = OPCODE::NOP;
                instructions[i + 2].op = OPCODE::NOP;
                instructions[i + 3].op = OPCODE::NOP;
            } else if (instructions[i].op == OPCODE::JMP && instructions[i].operands[0] == i + 1) {
                instructions[i].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PLT, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JGE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PLE, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JGT;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PGT, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JLE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PGE, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JLT;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PE, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JNE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PNE, OPCODE::JNC})) {
                instructions[i].op = OPCODE::JE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PLT, OPCODE::JC})) {
                instructions[i].op = OPCODE::JLT;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PLE, OPCODE::JC})) {
                instructions[i].op = OPCODE::JLE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PGT, OPCODE::JC})) {
                instructions[i].op = OPCODE::JGT;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PGE, OPCODE::JC})) {
                instructions[i].op = OPCODE::JGE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PE, OPCODE::JC})) {
                instructions[i].op = OPCODE::JE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            } else if (match_at(i, {OPCODE::PNE, OPCODE::JC})) {
                instructions[i].op = OPCODE::JNE;
                instructions[i].operands[0] = instructions[i + 1].operands[0];
                instructions[i + 1].op = OPCODE::NOP;
            }
        }
    }

  public:
    optimiser(program &prog) : prog(prog), instructions(prog.code) {}
    void run_all_optimisations() {
        peephole();
        unreachable_to_nop();
        remove_all_nops();
    }
};
} // namespace bvm
