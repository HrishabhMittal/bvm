#pragma once
#include "opcode.hpp"
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace bvm {
enum { GT = 0, E, LT };
struct instruction {
    OPCODE op;
    uint64_t operands[1];
};
struct program {
    std::string header;
    std::map<std::string, uint64_t> exported_functions;
    std::map<std::string, uint64_t> exported_globals;
    std::vector<instruction> code;
    std::string data;
};
inline std::ostream &operator<<(std::ostream &out, const OPCODE &i) {
    out << enum_name(i);
    return out;
}
union Value {
    uint64_t u64;
    int64_t i64;
    uint32_t u32;
    int32_t i32;
    double f64;
    float f32;
    void *ptr;
};
inline bool takes_operand(OPCODE op) {
    switch (op) {
    case OPCODE::PUSH:
    case OPCODE::CALL:
    case OPCODE::CALL_EXTERN:
    case OPCODE::LOAD:
    case OPCODE::STORE:
    case OPCODE::JMP:
    case OPCODE::JE:
    case OPCODE::JNE:
    case OPCODE::JLT:
    case OPCODE::JGT:
    case OPCODE::JLE:
    case OPCODE::JGE:
    case OPCODE::JC:
    case OPCODE::JNC:
    case OPCODE::UNDECLARE:
    case OPCODE::STRING_FROM:
        return true;
    default:
        return false;
    }
}

// one thing that i do have to think about is how to structure instruction
// so if i wanted to i coudl make every instruction except PUSH use a stack instead of loading from an operands
// maybe this will be more clear, maybe not ill have to think about whether i want to restructure this or not
class VM {
    std::vector<Value> stack;
    std::vector<std::vector<size_t>> struct_offsets;
    bool defining_struct = false;
    std::vector<Value> variables;
    program prog;
    uint64_t instruction_ptr = 0;
    uint8_t cmp_flags[3] = {0};
    std::map<std::string, std::pair<size_t, uint64_t>> global_linker_table;
    std::vector<bvm::program> loaded_modules;

    size_t current_module = 0; // The active binary
    std::vector<std::pair<size_t, uint64_t>> call_stack;
    inline void push(Value val) { stack.push_back(val); }
    inline Value pop() {
        if (stack.empty()) {
            throw std::runtime_error("Stack underflow!");
        }
        Value val = stack.back();
        stack.pop_back();
        return val;
    }
    inline Value &access_from_top(size_t ind) {
        if (ind >= stack.size())
            throw std::runtime_error("Negative stack index");
        ind = stack.size() - 1 - ind;
        return stack[ind];
    }
    inline Value &access_variable(int64_t ind) {
        // my new idea i wanna test out, lets me access variables relative to
        // the stack without using new instructions
        // ME FROM FUTURE: works absolutely flawlessly btw, im just an absolute genius
        // ME FROM EVEN MORE FUTURE: works flawlessly but u buffoon you resized the wrong stack, i fixed it
        while (ind < 0)
            ind += variables.size();
        if (ind >= variables.size()) {
            size_t cursize = variables.size();
            if (cursize == 0) {
                cursize = 256;
            }
            size_t oldsize = cursize;
            while (cursize <= ind)
                cursize *= 2;
            variables.resize(cursize);
        }
        return variables[ind];
    }

  public:
    VM(const program &p) : prog(p) {
        stack.reserve(8192);
        call_stack.reserve(1024);
        variables.reserve(256);
        loaded_modules.push_back(p);
    }
    void load_module(const program &p) {
        size_t index = loaded_modules.size();
        loaded_modules.push_back(p);
        for (const auto &[name, ip] : p.exported_functions) {
            global_linker_table[name] = {index, ip};
        }
    }
    int64_t return_value() {
        if (stack.size() == 0)
            return 0;
        else if (stack.size() == 1)
            return stack.back().i64;
        else
            throw std::runtime_error("program didn't exit properly");
    }
    void exec() {
        while (current_module < loaded_modules.size() && instruction_ptr < loaded_modules[current_module].code.size()) {
            instruction i = loaded_modules[current_module].code[instruction_ptr++];
            // std::cout << i.op << " [Stack Size:" << stack.size() << "]\n";
            // std::cout << "[";
            // for (auto i : stack)
            //     std::cout << i.u64 << ", ";
            // std::cout << "]" << std::endl;
            switch (i.op) {
            case OPCODE::END_GEN_ENUM_NAMES:
            case OPCODE::NOP:
                break;
            case OPCODE::HALT:
                return;
            case OPCODE::PUSH: {
                Value v;
                v.u64 = i.operands[0];
                push(v);
                break;
            }
            case OPCODE::BOOL_NOT: {
                Value v = pop();
                v.u64 = !v.u64;
                push(v);
                break;
            }
            case OPCODE::POP:
                pop();
                break;
            case OPCODE::DUP: {
                push(access_from_top(0));
                break;
            }
            case OPCODE::SWAP: {
                if (stack.size() < 2)
                    throw std::runtime_error("stack underflow");
                std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                break;
            }
            case OPCODE::OVER: {
                push(access_from_top(1));
                break;
            }
            case OPCODE::CALL: {
                call_stack.push_back({current_module, instruction_ptr});
                instruction_ptr = i.operands[0];
                break;
            }
            case OPCODE::CALL_EXTERN: {

                uint64_t str_ptr = i.operands[0];

                std::string target_func = "";

                const std::string &data_sec = loaded_modules[current_module].data;
                while (str_ptr < data_sec.size() && data_sec[str_ptr] != '\0') {
                    target_func += data_sec[str_ptr++];
                }
                if (!global_linker_table.count(target_func)) {
                    throw std::runtime_error("Linker Error: Undefined external reference to " + target_func);
                }

                auto location = global_linker_table[target_func];
                call_stack.push_back({current_module, instruction_ptr});
                current_module = location.first;
                instruction_ptr = location.second;
                break;
            }
            case OPCODE::RET: {
                if (call_stack.empty())
                    return;
                auto context = call_stack.back();
                call_stack.pop_back();
                current_module = context.first;
                instruction_ptr = context.second;
                break;
            }
            case OPCODE::DECLARE: {
                Value v;
                v.u64 = 0;
                variables.push_back(v);
                break;
            }
            case OPCODE::UNDECLARE: {
                const uint64_t num = variables.size() - i.operands[0];
                variables.resize(num);
                break;
            }
            case OPCODE::LOAD:
                push(access_variable(std::bit_cast<int64_t>(i.operands[0])));
                break;
            case OPCODE::STORE:
                access_variable(i.operands[0]) = pop();
                break;
            // case OPCODE::ALLOCA: {
            //     size_t size_in_bytes=pop().u64;
            //     Value v;
            //     size_t old_size=stack.size();
            //     stack.insert(stack.end(),size_in_bytes,{});
            //
            //     might cause a dangling pointer if stack is reallocated,
            //     figure out a better way v.ptr=&stack[old_size]; push(v);
            // }
            case OPCODE::MALLOC: {
                auto size_in_bytes = pop().u64;
                uint8_t *ptr = new uint8_t[size_in_bytes]();
                Value v;
                v.ptr = ptr;
                push(v);
                break;
            }
            case OPCODE::DEF_STRUCT: {
                defining_struct = true;
                struct_offsets.push_back({});
            }
            case OPCODE::PTR_AT: {
                if (defining_struct)
                    struct_offsets.back().push_back(i.operands[0]);
                else
                    throw std::runtime_error("PTR_AT found outside struct definition.");
            }
            case OPCODE::END_STRUCT: {
                defining_struct = false;
            }
            case OPCODE::STRING_FROM: {
                // todo: remove this, replace with something better
                uint64_t *str = new uint64_t[2];
                str[0] = reinterpret_cast<uint64_t>(&loaded_modules[current_module].data[i.operands[0]]);
                str[1] = pop().u64;
                Value v;
                v.ptr = str;
                push(v);
                break;
            }
            case OPCODE::I8_ALOAD: {
                auto index = pop().u64;
                int8_t *arr = reinterpret_cast<int8_t *>(pop().ptr);
                Value res;
                res.i64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::U8_ALOAD: {
                auto index = pop().u64;
                uint8_t *arr = reinterpret_cast<uint8_t *>(pop().ptr);
                Value res;
                res.u64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::I16_ALOAD: {
                auto index = pop().u64;
                int16_t *arr = reinterpret_cast<int16_t *>(pop().ptr);
                Value res;
                res.i64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::U16_ALOAD: {
                auto index = pop().u64;
                uint16_t *arr = reinterpret_cast<uint16_t *>(pop().ptr);
                Value res;
                res.u64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::I32_ALOAD: {
                auto index = pop().u64;
                int32_t *arr = reinterpret_cast<int32_t *>(pop().ptr);
                Value res;
                res.i64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::U32_ALOAD: {
                auto index = pop().u64;
                uint32_t *arr = reinterpret_cast<uint32_t *>(pop().ptr);
                Value res;
                res.u64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::I64_ALOAD: {
                auto index = pop().u64;
                uint64_t *arr = reinterpret_cast<uint64_t *>(pop().ptr);
                Value res;
                res.u64 = arr[index];
                push(res);
                break;
            }
            case OPCODE::I8_ASTORE: {
                auto val = pop();
                auto index = pop().u64;
                reinterpret_cast<int8_t *>(pop().ptr)[index] = static_cast<int8_t>(val.i32);
                break;
            }
            case OPCODE::I16_ASTORE: {
                auto val = pop();
                auto index = pop().u64;
                reinterpret_cast<int16_t *>(pop().ptr)[index] = static_cast<int16_t>(val.i32);
                break;
            }
            case OPCODE::I32_ASTORE: {
                auto val = pop();
                auto index = pop().u64;
                reinterpret_cast<int32_t *>(pop().ptr)[index] = val.i32;
                break;
            }
            case OPCODE::I64_ASTORE: {
                auto val = pop();
                auto index = pop().u64;
                reinterpret_cast<uint64_t *>(pop().ptr)[index] = val.u64;
                break;
            }
            case OPCODE::I32_NEGATE: {
                Value r = pop();
                r.i32 = -r.i32;
                push(r);
                break;
            }
            case OPCODE::I32_ADD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 + b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_SUB: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 - b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_MULT: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 * b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 / b.i32;
                push(r);
                break;
            }
            case OPCODE::U32_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u32 = a.u32 / b.u32;
                push(r);
                break;
            }
            case OPCODE::I32_MOD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 % b.i32;
                push(r);
                break;
            }
            case OPCODE::U32_MOD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u32 = a.u32 % b.u32;
                push(r);
                break;
            }
            case OPCODE::I64_NEGATE: {
                Value r = pop();
                r.i64 = -r.i64;
                push(r);
                break;
            }
            case OPCODE::I64_ADD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 + b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_SUB: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 - b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_MULT: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 * b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 / b.i64;
                push(r);
                break;
            }
            case OPCODE::U64_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = a.u64 / b.u64;
                push(r);
                break;
            }
            case OPCODE::I64_MOD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 % b.i64;
                push(r);
                break;
            }
            case OPCODE::U64_MOD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = a.u64 % b.u64;
                push(r);
                break;
            }
            case OPCODE::F32_NEGATE: {
                Value r = pop();
                r.f32 = -r.f32;
                push(r);
                break;
            }
            case OPCODE::F32_ADD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = a.f32 + b.f32;
                push(r);
                break;
            }
            case OPCODE::F32_SUB: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = a.f32 - b.f32;
                push(r);
                break;
            }
            case OPCODE::F32_MULT: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = a.f32 * b.f32;
                push(r);
                break;
            }
            case OPCODE::F32_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = a.f32 / b.f32;
                push(r);
                break;
            }
            case OPCODE::F64_NEGATE: {
                Value r = pop();
                r.f64 = -r.f64;
                push(r);
                break;
            }
            case OPCODE::F64_ADD: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = a.f64 + b.f64;
                push(r);
                break;
            }
            case OPCODE::F64_SUB: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = a.f64 - b.f64;
                push(r);
                break;
            }
            case OPCODE::F64_MULT: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = a.f64 * b.f64;
                push(r);
                break;
            }
            case OPCODE::F64_DIV: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = a.f64 / b.f64;
                push(r);
                break;
            }
            case OPCODE::I32_AND: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 & b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_OR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 | b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_XOR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 ^ b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_NOT: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = ~a.i32;
                push(r);
                break;
            }
            case OPCODE::I32_SHL: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 << b.i32;
                push(r);
                break;
            }
            case OPCODE::I32_SHR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = a.i32 >> b.i32;
                push(r);
                break;
            }
            case OPCODE::U32_SHR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u32 = a.u32 >> b.u32;
                push(r);
                break;
            }
            case OPCODE::I64_AND: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 & b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_OR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 | b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_XOR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 ^ b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_NOT: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = ~a.i64;
                push(r);
                break;
            }
            case OPCODE::I64_SHL: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 << b.i64;
                push(r);
                break;
            }
            case OPCODE::I64_SHR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = a.i64 >> b.i64;
                push(r);
                break;
            }
            case OPCODE::U64_SHR: {
                Value b = pop();
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = a.u64 >> b.u64;
                push(r);
                break;
            }
            case OPCODE::I32_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.i32 == b.i32);
                cmp_flags[LT] = (a.i32 < b.i32);
                cmp_flags[GT] = (a.i32 > b.i32);
                break;
            }
            case OPCODE::U32_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.u32 == b.u32);
                cmp_flags[LT] = (a.u32 < b.u32);
                cmp_flags[GT] = (a.u32 > b.u32);
                break;
            }
            case OPCODE::I64_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.i64 == b.i64);
                cmp_flags[LT] = (a.i64 < b.i64);
                cmp_flags[GT] = (a.i64 > b.i64);
                break;
            }
            case OPCODE::U64_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.u64 == b.u64);
                cmp_flags[LT] = (a.u64 < b.u64);
                cmp_flags[GT] = (a.u64 > b.u64);
                break;
            }
            case OPCODE::F32_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.f32 == b.f32);
                cmp_flags[LT] = (a.f32 < b.f32);
                cmp_flags[GT] = (a.f32 > b.f32);
                break;
            }
            case OPCODE::F64_CMP: {
                Value b = pop();
                Value a = pop();
                cmp_flags[E] = (a.f64 == b.f64);
                cmp_flags[LT] = (a.f64 < b.f64);
                cmp_flags[GT] = (a.f64 > b.f64);
                break;
            }
            case OPCODE::I32_EXTEND_I64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = static_cast<int64_t>(a.i32);
                push(r);
                break;
            }
            case OPCODE::U32_EXTEND_I64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = static_cast<uint64_t>(a.u32);
                push(r);
                break;
            }
            case OPCODE::I32_WRAP_I64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = 0;
                r.i32 = static_cast<int32_t>(a.i64);
                push(r);
                break;
            }
            case OPCODE::I32_TO_F64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = static_cast<double>(a.i32);
                push(r);
                break;
            }
            case OPCODE::U32_TO_F64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = static_cast<double>(a.u32);
                push(r);
                break;
            }
            case OPCODE::I64_TO_F32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = static_cast<float>(a.i64);
                push(r);
                break;
            }
            case OPCODE::I64_TO_F64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = static_cast<double>(a.i64);
                push(r);
                break;
            }
            case OPCODE::U64_TO_F32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = static_cast<float>(a.u64);
                push(r);
                break;
            }
            case OPCODE::U64_TO_F64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = static_cast<float>(a.u64);
                push(r);
                break;
            }
            case OPCODE::F64_TO_I32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i32 = static_cast<int32_t>(a.f64);
                push(r);
                break;
            }
            case OPCODE::F64_TO_U32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u32 = static_cast<uint32_t>(a.f64);
                push(r);
                break;
            }
            case OPCODE::F64_TO_I64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = static_cast<int64_t>(a.f64);
                push(r);
                break;
            }
            case OPCODE::F64_TO_U64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = static_cast<uint64_t>(a.f64);
                push(r);
                break;
            }
            case OPCODE::F32_TO_I64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = static_cast<int64_t>(a.f32);
                push(r);
                break;
            }
            case OPCODE::F32_TO_U64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = static_cast<uint64_t>(a.f32);
                push(r);
                break;
            }
            case OPCODE::I32_TO_F32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = static_cast<float>(a.i32);
                push(r);
                break;
            }
            case OPCODE::U32_TO_F32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = static_cast<float>(a.u32);
                push(r);
                break;
            }
            case OPCODE::F32_TO_I32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.i64 = 0;
                r.i32 = static_cast<int32_t>(a.f32);
                push(r);
                break;
            }
            case OPCODE::F32_TO_U32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.u64 = 0;
                r.u32 = static_cast<uint32_t>(a.f32);
                push(r);
                break;
            }
            case OPCODE::F32_TO_F64: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f64 = static_cast<double>(a.f32);
                push(r);
                break;
            }
            case OPCODE::F64_TO_F32: {
                Value a = pop();
                Value r;
                r.u64 = 0;
                r.f32 = static_cast<float>(a.f64);
                push(r);
                break;
            }

            // kinda temporary instructions, ill fix later hehe
            // update: nvm they seem kinda permanent
            case OPCODE::PE: {
                Value r;
                r.u64 = cmp_flags[E] ? 1 : 0;
                push(r);
                break;
            }
            case OPCODE::PNE: {
                Value r;
                r.u64 = cmp_flags[E] ? 0 : 1;
                push(r);
                break;
            }
            case OPCODE::PLT: {
                Value r;
                r.u64 = cmp_flags[LT] ? 1 : 0;
                push(r);
                break;
            }
            case OPCODE::PGT: {
                Value r;
                r.u64 = cmp_flags[GT] ? 1 : 0;
                push(r);
                break;
            }
            case OPCODE::PLE: {
                Value r;
                r.u64 = (cmp_flags[LT] || cmp_flags[E]) ? 1 : 0;
                push(r);
                break;
            }
            case OPCODE::PGE: {
                Value r;
                r.u64 = (cmp_flags[GT] || cmp_flags[E]) ? 1 : 0;
                push(r);
                break;
            }
            case OPCODE::JC: {
                bool jmp = pop().u64;
                if (jmp)
                    instruction_ptr = i.operands[0];
                break;
            }
            case OPCODE::JNC: {
                bool jmp = pop().u64;
                if (!jmp)
                    instruction_ptr = i.operands[0];
                break;
            }
            case OPCODE::JMP:
                instruction_ptr = i.operands[0];
                break;
            case OPCODE::JE:
                if (cmp_flags[E])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::JNE:
                if (!cmp_flags[E])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::JLT:
                if (cmp_flags[LT])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::JGT:
                if (cmp_flags[GT])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::JLE:
                if (cmp_flags[LT] || cmp_flags[E])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::JGE:
                if (cmp_flags[GT] || cmp_flags[E])
                    instruction_ptr = i.operands[0];
                break;
            case OPCODE::PRINT_U32:
                std::cout << pop().u32;
                break;
            case OPCODE::PRINT_U64:
                std::cout << pop().u64;
                break;
            case OPCODE::PRINT_F64:
                std::cout << pop().f64;
                break;
            case OPCODE::PRINT_I32:
                std::cout << pop().i32;
                break;
            case OPCODE::PRINT_F32:
                std::cout << pop().f32;
                break;
            case OPCODE::PRINT_I64:
                std::cout << pop().i64;
                break;
            case OPCODE::PRINT_STRING: {
                // todo: remove this for something better (THINK MARK THINK)
                uint64_t *str = reinterpret_cast<uint64_t *>(pop().ptr);
                char *ptr = reinterpret_cast<char *>(str[0]);
                uint64_t len = str[1];
                std::cout << std::string_view(ptr, len);
                break;
            }
            }
        }
    }
};
} // namespace bvm
