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
    size_t allocs = 0;
    const size_t GC_THRESHOLD = 4096;
    // writing would be much lower cortisol if i just made this std::list
    // but we love ourselves some cache friendliness
    std::vector<uint64_t> ptrs;
    void run() {
        mark();
        sweep();
        allocs = 0;
    }
    void mark() {
        // nolan: look mark, i scanned a stack.
        // mark: A STACK??
        // nolan: YES.
        for (size_t i = 0; i < stack.size(); i++)
            if (stack_ptrs[i]) {
                handle_marking_pointers(reinterpret_cast<uint64_t *>(stack[i].ptr));
            }
        for (size_t i = 0; i < variables.size(); i++)
            if (variable_ptrs[i]) {
                handle_marking_pointers(reinterpret_cast<uint64_t *>(variables[i].ptr));
            }
    }
    void handle_marking_pointers(uint64_t *ptr) {
        if (ptr == nullptr)
            return;
        uint64_t &meta_data = ptr[-1]; // holyy i've never used negative offsets, lesgoo
        if (meta_data & 1)
            return;
        meta_data |= 1;
        if (meta_data & 0b10) { // struct
            const uint64_t struct_id = meta_data >> 3;
            for (size_t i : struct_offsets[struct_id]) {
                handle_marking_pointers(reinterpret_cast<uint64_t *>(ptr[i]));
            }
        } else if (meta_data & 0b100) {
            // array with non primitives, insane i would have never spotted this bug by myself
            const uint64_t len = meta_data >> 3;
            // go fix ur segfaults boi
            const uint64_t size = (len - 8) / 8;
            for (size_t i = 0; i < size; i++) {
                handle_marking_pointers(reinterpret_cast<uint64_t *>(ptr[i]));
            }
        }
    }
    void sweep() {
        size_t last = 0;
        for (size_t i = 0; i < ptrs.size(); i++) {
            uint64_t *const pt = reinterpret_cast<uint64_t *>(ptrs[i]) - 1;
            if (pt[0] & 1) {
                pt[0] &= ~1ULL;
                ptrs[last++] = ptrs[i];
            } else {
                uint8_t *const dlt = reinterpret_cast<uint8_t *>(pt);
                delete[] dlt;
            }
        }
        ptrs.resize(last);
    }
};
} // namespace bvm
