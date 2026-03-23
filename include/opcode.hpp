#pragma once
#include <cstdint>
#include <string>
#include <array>
namespace bvm {
    enum class OPCODE : uint8_t {
        NOP = 0,
        HALT,
       
        PUSH, POP,
        DUP, SWAP, OVER,
        CALL, RET,
    
    //    ALLOCA,
        MALLOC, FREE,
        LOAD, STORE,
       
        I8_ALOAD,  U8_ALOAD,  
        I16_ALOAD, U16_ALOAD, 
        I32_ALOAD,           
        U32_ALOAD,           
        I64_ALOAD,           
       
        I8_ASTORE, 
        I16_ASTORE,
        I32_ASTORE,
        I64_ASTORE,
       
        I32_ADD, I32_SUB, I32_MULT, 
        I32_DIV, U32_DIV,
        I32_MOD, U32_MOD,
       
        I64_ADD, I64_SUB, I64_MULT,
        I64_DIV, U64_DIV, 
        I64_MOD, U64_MOD,
       
        F32_ADD, F32_SUB, F32_MULT, F32_DIV,
        F64_ADD, F64_SUB, F64_MULT, F64_DIV,
       
        I32_AND, I32_OR, I32_XOR, I32_NOT, I32_SHL,
        I64_AND, I64_OR, I64_XOR, I64_NOT, I64_SHL,
       
        I32_SHR, U32_SHR,
        I64_SHR, U64_SHR,

        PUSH_CMP,

        I32_CMP, U32_CMP,
        I64_CMP, U64_CMP,
        F32_CMP, F64_CMP,
       
        I32_EXTEND_I64, 
        U32_EXTEND_I64, 
        I32_WRAP_I64,   
       
        I32_TO_F32, U32_TO_F32,
        I32_TO_F64, U32_TO_F64,
        I64_TO_F32, U64_TO_F32,
        I64_TO_F64, U64_TO_F64,
        
        F32_TO_I32, F32_TO_U32,
        F64_TO_I32, F64_TO_U32,
        F32_TO_I64, F32_TO_U64,
        F64_TO_I64, F64_TO_U64,
       
        F32_TO_F64,
        F64_TO_F32,
       
        JMP, JE, JNE, JGT, JLT, JLE, JGE, JC, JNC,
        
        PRINT_I32, PRINT_U32, PRINT_I64, PRINT_U64, PRINT_F32, PRINT_F64,
    
        END_GEN_ENUM_NAMES
    };
    template <auto op>
    constexpr std::string_view enum_name_const() {
        std::string_view sv = __PRETTY_FUNCTION__;
        auto start = sv.find('=') + 2;
        auto end = sv.find(';');
        auto len = end - start;
        return sv.substr(start, len);
    }
    template <std::size_t... Is>
    constexpr auto generate_enum_names(std::index_sequence<Is...>) {
        return std::array<std::string_view, sizeof...(Is)>{
            enum_name_const<static_cast<OPCODE>(Is)>()...
        };
    }
    inline std::string_view enum_name(OPCODE op) {
        static constexpr auto names = generate_enum_names(std::make_index_sequence<static_cast<size_t>(OPCODE::END_GEN_ENUM_NAMES)>{});
        return names[static_cast<std::size_t>(op)];
    }
    template <typename T>
    constexpr std::string_view type_name() {
        std::string_view sv = __PRETTY_FUNCTION__;
        auto start = sv.find('=') + 2; 
        auto end = sv.find_last_of(']'); 
        return sv.substr(start, end - start);
    }
}
