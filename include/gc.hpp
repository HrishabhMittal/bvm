#pragma once
#include "opcode.hpp"
#include <vector>
namespace bvm {
union Value {
    uint64_t u64;
    int64_t i64;
    uint32_t u32;
    int32_t i32;
    double f64;
    float f32;
    void *ptr;
};
class GC {
public:
    std::vector<bool> stack_ptrs, variable_ptrs;
    std::vector<Value> stack, variables;
    std::vector<std::vector<size_t>> struct_offsets;
    std::vector<size_t> struct_len;
    std::vector<uint64_t> ptrs;
};
} // namespace bvm
