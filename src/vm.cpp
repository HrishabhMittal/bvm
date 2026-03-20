#include <cstdint>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>

enum class OPCODE {
    NOP,
    HALT,

    PUSH,
    POP,
    DUP,

    LOAD,
    STORE,

    IADD, ISUB, IMULT, IDIV,
    LADD, LSUB, LMULT, LDIV,
    FADD, FSUB, FMULT, FDIV,
    DADD, DSUB, DMULT, DDIV,

    AND, OR, XOR, NOT, SHL, SHR,

    ICMP,
    FCMP,

    JMP, JE, JNE, JGT, JLT, JLE, JGE,
    
    PRINT_I
};

struct instruction {
    OPCODE op;
    size_t operands[3];
};

class VM {
    std::vector<uint8_t> stack;
    std::vector<uint64_t> variables;
    std::vector<instruction> instructions;
    int64_t instruction_ptr = 0;
    uint8_t cmp_flags[3] = {0};
    template<typename T>
    void push(T val) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        stack.insert(stack.end(), bytes, bytes + sizeof(T));
    }
    template<typename T>
    T pop() {
        if (stack.size() < sizeof(T)) {
            throw std::runtime_error("Stack underflow!");
        }
        T val;
        std::memcpy(&val, stack.data() + stack.size() - sizeof(T), sizeof(T));
        stack.resize(stack.size() - sizeof(T));
        return val;
    }
public:
    VM(const std::vector<instruction>& ins) : instructions(ins) {
        stack.reserve(8192);
        instructions.reserve(instructions.size() + 1024);
        variables.resize(256, 0);
    }

    void add_instruction(instruction i) {
        instructions.push_back(i);
    }

    void exec() {
        while (instruction_ptr < instructions.size()) {
            instruction i = instructions[instruction_ptr++];

            switch (i.op) {
                case OPCODE::NOP: 
                    break;
                case OPCODE::HALT: 
                    return;
                case OPCODE::PUSH: 
                    push<uint64_t>(i.operands[0]); 
                    break;
                case OPCODE::POP:  
                    pop<uint64_t>();
                    break;
                case OPCODE::DUP: {
                    uint64_t val = pop<uint64_t>();
                    push<uint64_t>(val);
                    push<uint64_t>(val);
                    break;
                }
                case OPCODE::LOAD:
                    push<uint64_t>(variables[i.operands[0]]);
                    break;
                case OPCODE::STORE:
                    variables[i.operands[0]] = pop<uint64_t>();
                    break;
                case OPCODE::IADD: {
                    auto b = pop<int32_t>();
                    auto a = pop<int32_t>();
                    push<int32_t>(a + b);
                    break;
                }
                case OPCODE::ISUB: {
                    auto b = pop<int32_t>();
                    auto a = pop<int32_t>();
                    push<int32_t>(a - b);
                    break;
                }
                case OPCODE::IMULT: {
                    auto b = pop<int32_t>();
                    auto a = pop<int32_t>();
                    push<int32_t>(a * b);
                    break;
                }
                case OPCODE::IDIV: {
                    auto b = pop<int32_t>();
                    auto a = pop<int32_t>();
                    push<int32_t>(a / b);
                    break;
                }
                case OPCODE::LADD: {
                    auto b = pop<int64_t>();
                    auto a = pop<int64_t>();
                    push<int64_t>(a + b);
                    break;
                }
                case OPCODE::LSUB: {
                    auto b = pop<int64_t>();
                    auto a = pop<int64_t>();
                    push<int64_t>(a - b);
                    break;
                }
                case OPCODE::LMULT: {
                    auto b = pop<int64_t>();
                    auto a = pop<int64_t>();
                    push<int64_t>(a * b);
                    break;
                }
                case OPCODE::LDIV: {
                    auto b = pop<int64_t>();
                    auto a = pop<int64_t>();
                    push<int64_t>(a / b);
                    break;
                }
                case OPCODE::FADD: {
                    auto b = pop<float>();
                    auto a = pop<float>();
                    push<float>(a + b);
                    break;
                }
                case OPCODE::FSUB: {
                    auto b = pop<float>();
                    auto a = pop<float>();
                    push<float>(a - b);
                    break;
                }
                case OPCODE::FMULT: {
                    auto b = pop<float>();
                    auto a = pop<float>();
                    push<float>(a * b);
                    break;
                }
                case OPCODE::FDIV: {
                    auto b = pop<float>();
                    auto a = pop<float>();
                    push<float>(a / b);
                    break;
                }
                case OPCODE::DADD: {
                    auto b = pop<double>();
                    auto a = pop<double>();
                    push<double>(a + b);
                    break;
                }
                case OPCODE::DSUB: {
                    auto b = pop<double>();
                    auto a = pop<double>();
                    push<double>(a - b);
                    break;
                }
                case OPCODE::DMULT: {
                    auto b = pop<double>();
                    auto a = pop<double>();
                    push<double>(a * b);
                    break;
                }
                case OPCODE::DDIV: {
                    auto b = pop<double>();
                    auto a = pop<double>();
                    push<double>(a / b);
                    break;
                }
                case OPCODE::AND: {
                    auto b = pop<uint64_t>();
                    auto a = pop<uint64_t>();
                    push<uint64_t>(a & b);
                    break;
                }
                case OPCODE::OR: {
                    auto b = pop<uint64_t>();
                    auto a = pop<uint64_t>();
                    push<uint64_t>(a | b);
                    break;
                }
                case OPCODE::XOR: {
                    auto b = pop<uint64_t>();
                    auto a = pop<uint64_t>();
                    push<uint64_t>(a ^ b);
                    break;
                }
                case OPCODE::NOT: {
                    auto a = pop<uint64_t>();
                    push<uint64_t>(~a);
                    break;
                }
                case OPCODE::SHL: {
                    auto b = pop<uint64_t>();
                    auto a = pop<uint64_t>();
                    push<uint64_t>(a << b);
                    break;
                }
                case OPCODE::SHR: {
                    auto b = pop<uint64_t>();
                    auto a = pop<uint64_t>();
                    push<uint64_t>(a >> b);
                    break;
                }
                case OPCODE::ICMP: {
                    auto b = pop<int32_t>(); 
                    auto a = pop<int32_t>();
                    cmp_flags[0] = (a == b);
                    cmp_flags[1] = (a < b);
                    cmp_flags[2] = (a > b);
                    break;
                }
                case OPCODE::FCMP: {
                    auto b = pop<float>(); 
                    auto a = pop<float>();
                    cmp_flags[0] = (a == b);
                    cmp_flags[1] = (a < b);
                    cmp_flags[2] = (a > b);
                    break;
                }
                case OPCODE::JMP:
                    instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JE:
                    if (cmp_flags[0]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JNE:
                    if (!cmp_flags[0]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JLT:
                    if (cmp_flags[1]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JGT:
                    if (cmp_flags[2]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JLE:
                    if (cmp_flags[1] || cmp_flags[0]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::JGE:
                    if (cmp_flags[2] || cmp_flags[0]) instruction_ptr = i.operands[0];
                    break;
                case OPCODE::PRINT_I: {
                    std::cout << pop<int32_t>() << std::endl;
                    break;
                }
            }
        }
    }
};
