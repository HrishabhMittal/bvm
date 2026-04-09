#pragma once
#include "gc.hpp"
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
    case OPCODE::STRUCT_SIZE:
    case OPCODE::PTR_AT:
    case OPCODE::MALLOC:
    case OPCODE::MALLOC_STRUCT:
        return true;
    default:
        return false;
    }
}

// one thing that i do have to think about is how to structure instruction
// so if i wanted to i coudl make every instruction except PUSH use a stack instead of loading from an operands
// maybe this will be more clear, maybe not ill have to think about whether i want to restructure this or not
class VM {
    GC gc;
    bool defining_struct = false;
    program prog;
    uint64_t instruction_ptr = 0;
    uint8_t cmp_flags[3] = {0};
    std::map<std::string, std::pair<size_t, uint64_t>> global_linker_table;
    std::vector<bvm::program> loaded_modules;

    size_t current_module = 0; // The active binary
    std::vector<std::pair<size_t, uint64_t>> call_stack;
    __attribute__((always_inline)) inline void push(Value val, bool is_ptr = false) {
        gc.stack.push_back(val);
        gc.stack_ptrs.push_back(is_ptr);
    }
    __attribute__((always_inline)) inline std::pair<Value, bool> pop() {
        if (gc.stack.empty()) {
            throw std::runtime_error("Stack underflow!");
        }
        Value val = gc.stack.back();
        bool is_ptr = gc.stack_ptrs.back();
        gc.stack.pop_back();
        gc.stack_ptrs.pop_back();
        return {val, is_ptr};
    }
    inline Value &access_from_top(size_t ind) {
        if (ind >= gc.stack.size())
            throw std::runtime_error("Negative stack index");
        ind = gc.stack.size() - 1 - ind;
        return gc.stack[ind];
    }
    inline Value &access_variable(int64_t ind) {
        // my new idea i wanna test out, lets me access variables relative to
        // the stack without using new instructions
        // ME FROM FUTURE: works absolutely flawlessly btw, im just an absolute genius
        // ME FROM EVEN MORE FUTURE: works flawlessly but u buffoon you resized the wrong stack, i fixed it

        // although compiler will never generate wrong assembly which causes this bug,
        // there's no point in not putting safety checks just in case
        if (gc.variables.empty()) {
            throw std::runtime_error("indexing an empty variable stack");
        }
        ind = (ind < 0) ? (gc.variables.size() + ind) : ind;
        return gc.variables[ind];
    }
    inline int64_t access_ptr_bool(int64_t ind) {
        if (gc.variables.empty()) {
            throw std::runtime_error("indexing an empty variable stack");
        }
        ind = (ind < 0) ? (gc.variables.size() + ind) : ind;
        return ind;
    }

  public:
    VM(const program &p) : prog(p) {
        gc.stack.reserve(8192);
        gc.stack_ptrs.reserve(8192);
        call_stack.reserve(1024);
        gc.variables.reserve(256);
        gc.variable_ptrs.reserve(256);
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
        if (gc.stack.size() == 0)
            return 0;
        else if (gc.stack.size() == 1)
            return gc.stack.back().i64;
        else
            throw std::runtime_error("program didn't exit properly");
    }
    void exec() {
        static void *dispatch_table[] = {[static_cast<size_t>(OPCODE::NOP)] = &&op_nop,
                                         [static_cast<size_t>(OPCODE::HALT)] = &&op_halt,

                                         [static_cast<size_t>(OPCODE::PUSH)] = &&op_push,
                                         [static_cast<size_t>(OPCODE::POP)] = &&op_pop,
                                         [static_cast<size_t>(OPCODE::DUP)] = &&op_dup,
                                         [static_cast<size_t>(OPCODE::SWAP)] = &&op_swap,
                                         [static_cast<size_t>(OPCODE::OVER)] = &&op_over,
                                         [static_cast<size_t>(OPCODE::CALL)] = &&op_call,
                                         [static_cast<size_t>(OPCODE::CALL_EXTERN)] = &&op_call_extern,
                                         [static_cast<size_t>(OPCODE::RET)] = &&op_ret,
                                         [static_cast<size_t>(OPCODE::DECLARE)] = &&op_declare,
                                         [static_cast<size_t>(OPCODE::UNDECLARE)] = &&op_undeclare,

                                         [static_cast<size_t>(OPCODE::MALLOC)] = &&op_malloc,
                                         [static_cast<size_t>(OPCODE::MALLOC_STRUCT)] = &&op_malloc_struct,
                                         [static_cast<size_t>(OPCODE::DEF_STRUCT)] = &&op_def_struct,
                                         [static_cast<size_t>(OPCODE::PTR_AT)] = &&op_ptr_at,
                                         [static_cast<size_t>(OPCODE::STRUCT_SIZE)] = &&op_struct_size,
                                         [static_cast<size_t>(OPCODE::END_STRUCT)] = &&op_end_struct,

                                         [static_cast<size_t>(OPCODE::LOAD)] = &&op_load,
                                         [static_cast<size_t>(OPCODE::STORE)] = &&op_store,

                                         [static_cast<size_t>(OPCODE::STRING_FROM)] = &&op_string_from,

                                         [static_cast<size_t>(OPCODE::I8_ALOAD)] = &&op_i8_aload,
                                         [static_cast<size_t>(OPCODE::U8_ALOAD)] = &&op_u8_aload,
                                         [static_cast<size_t>(OPCODE::I16_ALOAD)] = &&op_i16_aload,
                                         [static_cast<size_t>(OPCODE::U16_ALOAD)] = &&op_u16_aload,
                                         [static_cast<size_t>(OPCODE::I32_ALOAD)] = &&op_i32_aload,
                                         [static_cast<size_t>(OPCODE::U32_ALOAD)] = &&op_u32_aload,
                                         [static_cast<size_t>(OPCODE::I64_ALOAD)] = &&op_i64_aload,

                                         [static_cast<size_t>(OPCODE::I8_ASTORE)] = &&op_i8_astore,
                                         [static_cast<size_t>(OPCODE::I16_ASTORE)] = &&op_i16_astore,
                                         [static_cast<size_t>(OPCODE::I32_ASTORE)] = &&op_i32_astore,
                                         [static_cast<size_t>(OPCODE::I64_ASTORE)] = &&op_i64_astore,

                                         [static_cast<size_t>(OPCODE::BOOL_NOT)] = &&op_bool_not,

                                         [static_cast<size_t>(OPCODE::I32_NEGATE)] = &&op_i32_negate,
                                         [static_cast<size_t>(OPCODE::I32_ADD)] = &&op_i32_add,
                                         [static_cast<size_t>(OPCODE::I32_SUB)] = &&op_i32_sub,
                                         [static_cast<size_t>(OPCODE::I32_MULT)] = &&op_i32_mult,
                                         [static_cast<size_t>(OPCODE::I32_DIV)] = &&op_i32_div,
                                         [static_cast<size_t>(OPCODE::U32_DIV)] = &&op_u32_div,
                                         [static_cast<size_t>(OPCODE::I32_MOD)] = &&op_i32_mod,
                                         [static_cast<size_t>(OPCODE::U32_MOD)] = &&op_u32_mod,

                                         [static_cast<size_t>(OPCODE::I64_NEGATE)] = &&op_i64_negate,
                                         [static_cast<size_t>(OPCODE::I64_ADD)] = &&op_i64_add,
                                         [static_cast<size_t>(OPCODE::I64_SUB)] = &&op_i64_sub,
                                         [static_cast<size_t>(OPCODE::I64_MULT)] = &&op_i64_mult,
                                         [static_cast<size_t>(OPCODE::I64_DIV)] = &&op_i64_div,
                                         [static_cast<size_t>(OPCODE::U64_DIV)] = &&op_u64_div,
                                         [static_cast<size_t>(OPCODE::I64_MOD)] = &&op_i64_mod,
                                         [static_cast<size_t>(OPCODE::U64_MOD)] = &&op_u64_mod,

                                         [static_cast<size_t>(OPCODE::F32_NEGATE)] = &&op_f32_negate,
                                         [static_cast<size_t>(OPCODE::F32_ADD)] = &&op_f32_add,
                                         [static_cast<size_t>(OPCODE::F32_SUB)] = &&op_f32_sub,
                                         [static_cast<size_t>(OPCODE::F32_MULT)] = &&op_f32_mult,
                                         [static_cast<size_t>(OPCODE::F32_DIV)] = &&op_f32_div,

                                         [static_cast<size_t>(OPCODE::F64_NEGATE)] = &&op_f64_negate,
                                         [static_cast<size_t>(OPCODE::F64_ADD)] = &&op_f64_add,
                                         [static_cast<size_t>(OPCODE::F64_SUB)] = &&op_f64_sub,
                                         [static_cast<size_t>(OPCODE::F64_MULT)] = &&op_f64_mult,
                                         [static_cast<size_t>(OPCODE::F64_DIV)] = &&op_f64_div,

                                         [static_cast<size_t>(OPCODE::I32_AND)] = &&op_i32_and,
                                         [static_cast<size_t>(OPCODE::I32_OR)] = &&op_i32_or,
                                         [static_cast<size_t>(OPCODE::I32_XOR)] = &&op_i32_xor,
                                         [static_cast<size_t>(OPCODE::I32_NOT)] = &&op_i32_not,
                                         [static_cast<size_t>(OPCODE::I32_SHL)] = &&op_i32_shl,
                                         [static_cast<size_t>(OPCODE::I64_AND)] = &&op_i64_and,
                                         [static_cast<size_t>(OPCODE::I64_OR)] = &&op_i64_or,
                                         [static_cast<size_t>(OPCODE::I64_XOR)] = &&op_i64_xor,
                                         [static_cast<size_t>(OPCODE::I64_NOT)] = &&op_i64_not,
                                         [static_cast<size_t>(OPCODE::I64_SHL)] = &&op_i64_shl,

                                         [static_cast<size_t>(OPCODE::I32_SHR)] = &&op_i32_shr,
                                         [static_cast<size_t>(OPCODE::U32_SHR)] = &&op_u32_shr,
                                         [static_cast<size_t>(OPCODE::I64_SHR)] = &&op_i64_shr,
                                         [static_cast<size_t>(OPCODE::U64_SHR)] = &&op_u64_shr,

                                         [static_cast<size_t>(OPCODE::I32_CMP)] = &&op_i32_cmp,
                                         [static_cast<size_t>(OPCODE::U32_CMP)] = &&op_u32_cmp,
                                         [static_cast<size_t>(OPCODE::I64_CMP)] = &&op_i64_cmp,
                                         [static_cast<size_t>(OPCODE::U64_CMP)] = &&op_u64_cmp,
                                         [static_cast<size_t>(OPCODE::F32_CMP)] = &&op_f32_cmp,
                                         [static_cast<size_t>(OPCODE::F64_CMP)] = &&op_f64_cmp,

                                         [static_cast<size_t>(OPCODE::I32_EXTEND_I64)] = &&op_i32_extend_i64,
                                         [static_cast<size_t>(OPCODE::U32_EXTEND_I64)] = &&op_u32_extend_i64,
                                         [static_cast<size_t>(OPCODE::I32_WRAP_I64)] = &&op_i32_wrap_i64,

                                         [static_cast<size_t>(OPCODE::I32_TO_F32)] = &&op_i32_to_f32,
                                         [static_cast<size_t>(OPCODE::U32_TO_F32)] = &&op_u32_to_f32,
                                         [static_cast<size_t>(OPCODE::I32_TO_F64)] = &&op_i32_to_f64,
                                         [static_cast<size_t>(OPCODE::U32_TO_F64)] = &&op_u32_to_f64,
                                         [static_cast<size_t>(OPCODE::I64_TO_F32)] = &&op_i64_to_f32,
                                         [static_cast<size_t>(OPCODE::U64_TO_F32)] = &&op_u64_to_f32,
                                         [static_cast<size_t>(OPCODE::I64_TO_F64)] = &&op_i64_to_f64,
                                         [static_cast<size_t>(OPCODE::U64_TO_F64)] = &&op_u64_to_f64,

                                         [static_cast<size_t>(OPCODE::F32_TO_I32)] = &&op_f32_to_i32,
                                         [static_cast<size_t>(OPCODE::F32_TO_U32)] = &&op_f32_to_u32,
                                         [static_cast<size_t>(OPCODE::F64_TO_I32)] = &&op_f64_to_i32,
                                         [static_cast<size_t>(OPCODE::F64_TO_U32)] = &&op_f64_to_u32,
                                         [static_cast<size_t>(OPCODE::F32_TO_I64)] = &&op_f32_to_i64,
                                         [static_cast<size_t>(OPCODE::F32_TO_U64)] = &&op_f32_to_u64,
                                         [static_cast<size_t>(OPCODE::F64_TO_I64)] = &&op_f64_to_i64,
                                         [static_cast<size_t>(OPCODE::F64_TO_U64)] = &&op_f64_to_u64,

                                         [static_cast<size_t>(OPCODE::F32_TO_F64)] = &&op_f32_to_f64,
                                         [static_cast<size_t>(OPCODE::F64_TO_F32)] = &&op_f64_to_f32,

                                         [static_cast<size_t>(OPCODE::PE)] = &&op_pe,
                                         [static_cast<size_t>(OPCODE::PNE)] = &&op_pne,
                                         [static_cast<size_t>(OPCODE::PGT)] = &&op_pgt,
                                         [static_cast<size_t>(OPCODE::PLT)] = &&op_plt,
                                         [static_cast<size_t>(OPCODE::PLE)] = &&op_ple,
                                         [static_cast<size_t>(OPCODE::PGE)] = &&op_pge,
                                         [static_cast<size_t>(OPCODE::JMP)] = &&op_jmp,
                                         [static_cast<size_t>(OPCODE::JE)] = &&op_je,
                                         [static_cast<size_t>(OPCODE::JNE)] = &&op_jne,
                                         [static_cast<size_t>(OPCODE::JGT)] = &&op_jgt,
                                         [static_cast<size_t>(OPCODE::JLT)] = &&op_jlt,
                                         [static_cast<size_t>(OPCODE::JLE)] = &&op_jle,
                                         [static_cast<size_t>(OPCODE::JGE)] = &&op_jge,
                                         [static_cast<size_t>(OPCODE::JC)] = &&op_jc,
                                         [static_cast<size_t>(OPCODE::JNC)] = &&op_jnc,

                                         [static_cast<size_t>(OPCODE::PRINT_I32)] = &&op_print_i32,
                                         [static_cast<size_t>(OPCODE::PRINT_U32)] = &&op_print_u32,
                                         [static_cast<size_t>(OPCODE::PRINT_I64)] = &&op_print_i64,
                                         [static_cast<size_t>(OPCODE::PRINT_U64)] = &&op_print_u64,
                                         [static_cast<size_t>(OPCODE::PRINT_F32)] = &&op_print_f32,
                                         [static_cast<size_t>(OPCODE::PRINT_F64)] = &&op_print_f64,
                                         [static_cast<size_t>(OPCODE::PRINT_STRING)] = &&op_print_string,

                                         [static_cast<size_t>(OPCODE::END_GEN_ENUM_NAMES)] = &&op_end_gen_enum_names};

        const instruction *i;
        #define DISPATCH()                                                                                                     \
            {                                                                                                                  \
                if (current_module >= loaded_modules.size() || instruction_ptr >= loaded_modules[current_module].code.size())  \
                    return;                                                                                                    \
                i = &loaded_modules[current_module].code[instruction_ptr++];                                                   \
                goto *dispatch_table[static_cast<size_t>(i->op)];                                                              \
            }
        
        op_end_gen_enum_names:
        op_nop:
            DISPATCH();
        op_halt:
            return;
        op_push: {
            Value v;
            v.u64 = i->operands[0];
            push(v);
            DISPATCH();
        }
        op_bool_not: {
            Value v = pop().first;
            v.u64 = !v.u64;
            push(v);
            DISPATCH();
        }
        op_pop:
            pop();
            DISPATCH();
        op_dup: {
            push(access_from_top(0), gc.stack_ptrs[gc.stack_ptrs.size() - 1]);
            DISPATCH();
        }
        op_swap: {
            if (gc.stack.size() < 2)
                throw std::runtime_error("stack underflow");
            std::swap(gc.stack[gc.stack.size() - 1], gc.stack[gc.stack.size() - 2]);

            // bcoz ofc bitset doesnt implement shite
            bool v = gc.stack_ptrs[gc.stack_ptrs.size() - 1];
            gc.stack_ptrs[gc.stack_ptrs.size() - 1] = gc.stack_ptrs[gc.stack_ptrs.size() - 2];
            gc.stack_ptrs[gc.stack_ptrs.size() - 2] = v;
            DISPATCH();
        }
        op_over: {
            push(access_from_top(1), gc.stack_ptrs[gc.stack_ptrs.size() - 2]);
            DISPATCH();
        }
        op_call: {
            call_stack.push_back({current_module, instruction_ptr});
            instruction_ptr = i->operands[0];
            DISPATCH();
        }
        op_call_extern: {

            uint64_t str_ptr = i->operands[0];

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
        }
        DISPATCH();
        op_ret: {
            if (call_stack.empty())
                return;
            auto context = call_stack.back();
            call_stack.pop_back();
            current_module = context.first;
            instruction_ptr = context.second;
            DISPATCH();
        }
        op_declare: {
            Value v;
            v.u64 = 0;
            gc.variables.push_back(v);
            gc.variable_ptrs.push_back(false);
            DISPATCH();
        }
        op_undeclare: {
            const uint64_t num = gc.variables.size() - i->operands[0];
            gc.variables.resize(num);
            gc.variable_ptrs.resize(num);
            DISPATCH();
        }
        op_load: {
            int64_t idx = std::bit_cast<int64_t>(i->operands[0]);
            push(access_variable(idx), gc.variable_ptrs[access_ptr_bool(idx)]);
            DISPATCH();
        }
        op_store: {
            auto x = pop();
            access_variable(i->operands[0]) = x.first;
            gc.variable_ptrs[access_ptr_bool(i->operands[0])] = x.second;
            DISPATCH();
        }
        // okay i should prob document a lil
        // metadata: 64 bits
        // len: 61 bits | is_pointer_array: 1 bit | is_struct: 1 bit | mark_n_sweep: 1 bit
        //                ^^^^^^^^^^^^^^^^^^^^^^^
        //           "to scan or not to scan, that is the question" - Hrishabh Mittal
        op_malloc: {
            size_t size_in_bytes = pop().first.u64 + 8;
            uint64_t *ptr = reinterpret_cast<uint64_t *>(new uint8_t[size_in_bytes]());
            // std::cout << "MALLOC: " << ptr << size_in_bytes << std::endl;
            gc.ptrs.push_back(reinterpret_cast<uint64_t>(&ptr[1]));
            uint64_t ptr_array = i->operands[0] ? 0b100 : 0;
            ptr[0] = (size_in_bytes << 3) | ptr_array;
            Value v;
            v.ptr = &ptr[1];
            push(v, true);
            gc.allocs++;
            if (gc.allocs >= gc.GC_THRESHOLD) {
                gc.run();
            }
            DISPATCH();
        }
        op_malloc_struct: {
            size_t size_in_bytes = gc.struct_len[i->operands[0]] + 8;
            uint64_t *ptr = reinterpret_cast<uint64_t *>(new uint8_t[size_in_bytes]());
            // std::cout << "MALLOC: " << ptr << size_in_bytes << std::endl;
            gc.ptrs.push_back(reinterpret_cast<uint64_t>(&ptr[1]));
            ptr[0] = (i->operands[0] << 3) | 0b10;
            Value v;
            v.ptr = &ptr[1];
            push(v, true);
            gc.allocs++;
            if (gc.allocs >= gc.GC_THRESHOLD) {
                gc.run();
            }
            DISPATCH();
        }
        op_def_struct: {
            defining_struct = true;
            gc.struct_offsets.push_back({});
            DISPATCH();
        }
        op_ptr_at: {
            if (defining_struct)
                gc.struct_offsets.back().push_back(i->operands[0]);
            else
                throw std::runtime_error("PTR_AT found outside struct definition.");
            DISPATCH();
        }
        op_struct_size: {
            // todo: not allow multiple structsize in one struct def, also make sure to check if struct size was
            // added in END_STRUCT.
            if (defining_struct)
                gc.struct_len.push_back(i->operands[0]);
            else
                throw std::runtime_error("STRUCT_SIZE found outside struct definition.");
            DISPATCH();
        }
        op_end_struct: {
            defining_struct = false;
            DISPATCH();
        }
        op_string_from: {
            // todo: remove this, replace with something better
            uint64_t *str = new uint64_t[2];
            str[0] = reinterpret_cast<uint64_t>(&loaded_modules[current_module].data[i->operands[0]]);
            str[1] = pop().first.u64;
            Value v;
            v.ptr = str;
            push(v);
            DISPATCH();
        }
        op_i8_aload: {
            auto index = pop().first.u64;
            int8_t *arr = reinterpret_cast<int8_t *>(pop().first.ptr);
            Value res;
            res.i64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_u8_aload: {
            auto index = pop().first.u64;
            uint8_t *arr = reinterpret_cast<uint8_t *>(pop().first.ptr);
            Value res;
            res.u64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_i16_aload: {
            auto index = pop().first.u64;
            int16_t *arr = reinterpret_cast<int16_t *>(pop().first.ptr);
            Value res;
            res.i64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_u16_aload: {
            auto index = pop().first.u64;
            uint16_t *arr = reinterpret_cast<uint16_t *>(pop().first.ptr);
            Value res;
            res.u64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_i32_aload: {
            auto index = pop().first.u64;
            int32_t *arr = reinterpret_cast<int32_t *>(pop().first.ptr);
            Value res;
            res.i64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_u32_aload: {
            auto index = pop().first.u64;
            uint32_t *arr = reinterpret_cast<uint32_t *>(pop().first.ptr);
            Value res;
            res.u64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_i64_aload: {
            auto index = pop().first.u64;
            uint64_t *arr = reinterpret_cast<uint64_t *>(pop().first.ptr);
            Value res;
            res.u64 = arr[index];
            push(res);
            DISPATCH();
        }
        op_i8_astore: {
            auto val = pop().first;
            auto index = pop().first.u64;
            reinterpret_cast<int8_t *>(pop().first.ptr)[index] = static_cast<int8_t>(val.i32);
            DISPATCH();
        }
        op_i16_astore: {
            auto val = pop().first;
            auto index = pop().first.u64;
            reinterpret_cast<int16_t *>(pop().first.ptr)[index] = static_cast<int16_t>(val.i32);
            DISPATCH();
        }
        op_i32_astore: {
            auto val = pop().first;
            auto index = pop().first.u64;
            reinterpret_cast<int32_t *>(pop().first.ptr)[index] = val.i32;
            DISPATCH();
        }
        op_i64_astore: {
            auto val = pop().first;
            auto index = pop().first.u64;
            reinterpret_cast<uint64_t *>(pop().first.ptr)[index] = val.u64;
            DISPATCH();
        }
        op_i32_negate: {
            Value r = pop().first;
            r.i32 = -r.i32;
            push(r);
            DISPATCH();
        }
        op_i32_add: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 + b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_sub: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 - b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_mult: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 * b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 / b.i32;
            push(r);
            DISPATCH();
        }
        op_u32_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u32 = a.u32 / b.u32;
            push(r);
            DISPATCH();
        }
        op_i32_mod: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 % b.i32;
            push(r);
            DISPATCH();
        }
        op_u32_mod: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u32 = a.u32 % b.u32;
            push(r);
            DISPATCH();
        }
        op_i64_negate: {
            Value r = pop().first;
            r.i64 = -r.i64;
            push(r);
            DISPATCH();
        }
        op_i64_add: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 + b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_sub: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 - b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_mult: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 * b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 / b.i64;
            push(r);
            DISPATCH();
        }
        op_u64_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = a.u64 / b.u64;
            push(r);
            DISPATCH();
        }
        op_i64_mod: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 % b.i64;
            push(r);
            DISPATCH();
        }
        op_u64_mod: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = a.u64 % b.u64;
            push(r);
            DISPATCH();
        }
        op_f32_negate: {
            Value r = pop().first;
            r.f32 = -r.f32;
            push(r);
            DISPATCH();
        }
        op_f32_add: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = a.f32 + b.f32;
            push(r);
            DISPATCH();
        }
        op_f32_sub: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = a.f32 - b.f32;
            push(r);
            DISPATCH();
        }
        op_f32_mult: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = a.f32 * b.f32;
            push(r);
            DISPATCH();
        }
        op_f32_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = a.f32 / b.f32;
            push(r);
            DISPATCH();
        }
        op_f64_negate: {
            Value r = pop().first;
            r.f64 = -r.f64;
            push(r);
            DISPATCH();
        }
        op_f64_add: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = a.f64 + b.f64;
            push(r);
            DISPATCH();
        }
        op_f64_sub: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = a.f64 - b.f64;
            push(r);
            DISPATCH();
        }
        op_f64_mult: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = a.f64 * b.f64;
            push(r);
            DISPATCH();
        }
        op_f64_div: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = a.f64 / b.f64;
            push(r);
            DISPATCH();
        }
        op_i32_and: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 & b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_or: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 | b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_xor: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 ^ b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_not: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = ~a.i32;
            push(r);
            DISPATCH();
        }
        op_i32_shl: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 << b.i32;
            push(r);
            DISPATCH();
        }
        op_i32_shr: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = a.i32 >> b.i32;
            push(r);
            DISPATCH();
        }
        op_u32_shr: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u32 = a.u32 >> b.u32;
            push(r);
            DISPATCH();
        }
        op_i64_and: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 & b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_or: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 | b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_xor: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 ^ b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_not: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = ~a.i64;
            push(r);
            DISPATCH();
        }
        op_i64_shl: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 << b.i64;
            push(r);
            DISPATCH();
        }
        op_i64_shr: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = a.i64 >> b.i64;
            push(r);
            DISPATCH();
        }
        op_u64_shr: {
            Value b = pop().first;
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = a.u64 >> b.u64;
            push(r);
            DISPATCH();
        }
        op_i32_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.i32 == b.i32);
            cmp_flags[LT] = (a.i32 < b.i32);
            cmp_flags[GT] = (a.i32 > b.i32);
            DISPATCH();
        }
        op_u32_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.u32 == b.u32);
            cmp_flags[LT] = (a.u32 < b.u32);
            cmp_flags[GT] = (a.u32 > b.u32);
            DISPATCH();
        }
        op_i64_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.i64 == b.i64);
            cmp_flags[LT] = (a.i64 < b.i64);
            cmp_flags[GT] = (a.i64 > b.i64);
            DISPATCH();
        }
        op_u64_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.u64 == b.u64);
            cmp_flags[LT] = (a.u64 < b.u64);
            cmp_flags[GT] = (a.u64 > b.u64);
            DISPATCH();
        }
        op_f32_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.f32 == b.f32);
            cmp_flags[LT] = (a.f32 < b.f32);
            cmp_flags[GT] = (a.f32 > b.f32);
            DISPATCH();
        }
        op_f64_cmp: {
            Value b = pop().first;
            Value a = pop().first;
            cmp_flags[E] = (a.f64 == b.f64);
            cmp_flags[LT] = (a.f64 < b.f64);
            cmp_flags[GT] = (a.f64 > b.f64);
            DISPATCH();
        }
        op_i32_extend_i64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = static_cast<int64_t>(a.i32);
            push(r);
            DISPATCH();
        }
        op_u32_extend_i64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = static_cast<uint64_t>(a.u32);
            push(r);
            DISPATCH();
        }
        op_i32_wrap_i64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = 0;
            r.i32 = static_cast<int32_t>(a.i64);
            push(r);
            DISPATCH();
        }
        op_i32_to_f64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = static_cast<double>(a.i32);
            push(r);
            DISPATCH();
        }
        op_u32_to_f64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = static_cast<double>(a.u32);
            push(r);
            DISPATCH();
        }
        op_i64_to_f32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = static_cast<float>(a.i64);
            push(r);
            DISPATCH();
        }
        op_i64_to_f64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = static_cast<double>(a.i64);
            push(r);
            DISPATCH();
        }
        op_u64_to_f32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = static_cast<float>(a.u64);
            push(r);
            DISPATCH();
        }
        op_u64_to_f64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = static_cast<double>(a.u64);
            push(r);
            DISPATCH();
        }
        op_f64_to_i32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i32 = static_cast<int32_t>(a.f64);
            push(r);
            DISPATCH();
        }
        op_f64_to_u32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u32 = static_cast<uint32_t>(a.f64);
            push(r);
            DISPATCH();
        }
        op_f64_to_i64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = static_cast<int64_t>(a.f64);
            push(r);
            DISPATCH();
        }
        op_f64_to_u64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = static_cast<uint64_t>(a.f64);
            push(r);
            DISPATCH();
        }
        op_f32_to_i64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = static_cast<int64_t>(a.f32);
            push(r);
            DISPATCH();
        }
        op_f32_to_u64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = static_cast<uint64_t>(a.f32);
            push(r);
            DISPATCH();
        }
        op_i32_to_f32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = static_cast<float>(a.i32);
            push(r);
            DISPATCH();
        }
        op_u32_to_f32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = static_cast<float>(a.u32);
            push(r);
            DISPATCH();
        }
        op_f32_to_i32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.i64 = 0;
            r.i32 = static_cast<int32_t>(a.f32);
            push(r);
            DISPATCH();
        }
        op_f32_to_u32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.u64 = 0;
            r.u32 = static_cast<uint32_t>(a.f32);
            push(r);
            DISPATCH();
        }
        op_f32_to_f64: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f64 = static_cast<double>(a.f32);
            push(r);
            DISPATCH();
        }
        op_f64_to_f32: {
            Value a = pop().first;
            Value r;
            r.u64 = 0;
            r.f32 = static_cast<float>(a.f64);
            push(r);
            DISPATCH();
        }

        // kinda temporary instructions, ill fix later hehe
        // update: nvm they seem kinda permanent
        op_pe: {
            Value r;
            r.u64 = cmp_flags[E] ? 1 : 0;
            push(r);
            DISPATCH();
        }
        op_pne: {
            Value r;
            r.u64 = cmp_flags[E] ? 0 : 1;
            push(r);
            DISPATCH();
        }
        op_plt: {
            Value r;
            r.u64 = cmp_flags[LT] ? 1 : 0;
            push(r);
            DISPATCH();
        }
        op_pgt: {
            Value r;
            r.u64 = cmp_flags[GT] ? 1 : 0;
            push(r);
            DISPATCH();
        }
        op_ple: {
            Value r;
            r.u64 = (cmp_flags[LT] || cmp_flags[E]) ? 1 : 0;
            push(r);
            DISPATCH();
        }
        op_pge: {
            Value r;
            r.u64 = (cmp_flags[GT] || cmp_flags[E]) ? 1 : 0;
            push(r);
            DISPATCH();
        }
        op_jc: {
            bool jmp = pop().first.u64;
            if (jmp)
                instruction_ptr = i->operands[0];
            DISPATCH();
        }
        op_jnc: {
            bool jmp = pop().first.u64;
            if (!jmp)
                instruction_ptr = i->operands[0];
            DISPATCH();
        }
        op_jmp:
            instruction_ptr = i->operands[0];
            DISPATCH();
        op_je:
            if (cmp_flags[E])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_jne:
            if (!cmp_flags[E])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_jlt:
            if (cmp_flags[LT])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_jgt:
            if (cmp_flags[GT])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_jle:
            if (cmp_flags[LT] || cmp_flags[E])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_jge:
            if (cmp_flags[GT] || cmp_flags[E])
                instruction_ptr = i->operands[0];
            DISPATCH();
        op_print_u32:
            std::cout << pop().first.u32;
            DISPATCH();
        op_print_u64:
            std::cout << pop().first.u64;
            DISPATCH();
        op_print_f64:
            std::cout << pop().first.f64;
            DISPATCH();
        op_print_i32:
            std::cout << pop().first.i32;
            DISPATCH();
        op_print_f32:
            std::cout << pop().first.f32;
            DISPATCH();
        op_print_i64:
            std::cout << pop().first.i64;
            DISPATCH();
        op_print_string: {
            // todo: remove this for something better (THINK MARK THINK)
            uint64_t *str = reinterpret_cast<uint64_t *>(pop().first.ptr);
            char *ptr = reinterpret_cast<char *>(str[0]);
            uint64_t len = str[1];
            std::cout << std::string_view(ptr, len);
            DISPATCH();
        }
    }
};
} // namespace bvm
