#pragma once
#include "opcode.hpp"
#include "vm.hpp"
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
namespace bvm {
inline void write_string(const std::string &str, std::ofstream &out) {
    uint64_t size = str.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    if (size > 0) {
        out.write(str.c_str(), size);
    }
}
inline std::string read_string(std::ifstream &in) {
    uint64_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (size == 0)
        return "";
    std::string str(size, '\0');
    in.read(&str[0], size);
    return str;
}
inline program load_bytecode(std::ifstream &in) {
    program out;
    uint8_t byte;
    uint64_t size;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.header.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        out.header[i] = byte;
    }
    uint64_t map_size;
    in.read(reinterpret_cast<char *>(&map_size), sizeof(map_size));
    for (uint64_t i = 0; i < map_size; i++) {
        std::string name = read_string(in);
        uint64_t ip;
        in.read(reinterpret_cast<char *>(&ip), sizeof(ip));
        out.exported_functions[name] = ip;
    }
    in.read(reinterpret_cast<char *>(&map_size), sizeof(map_size));
    for (uint64_t i = 0; i < map_size; i++) {
        std::string name = read_string(in);
        uint64_t index;
        in.read(reinterpret_cast<char *>(&index), sizeof(index));
        out.exported_globals[name] = index;
    }
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.code.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        const uint32_t takes = takes_operand(static_cast<OPCODE>(byte));
        instruction instr;
        instr.op = static_cast<OPCODE>(byte);
        in.read(reinterpret_cast<char *>(instr.operands), sizeof(uint64_t)*takes);
        out.code[i] = instr;
    }
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    out.data.resize(size);
    for (uint64_t i = 0; i < size; i++) {
        in.read(reinterpret_cast<char *>(&byte), sizeof(byte));
        out.data[i] = byte;
    }
    return out;
}
inline void dump_bytecode(const program &ins, std::ofstream &out) {
    uint64_t size = ins.header.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (char i : ins.header) {
        out.write(&i, sizeof(i));
    }
    uint64_t map_size = ins.exported_functions.size();
    out.write(reinterpret_cast<const char *>(&map_size), sizeof(map_size));
    for (const auto &[name, ip] : ins.exported_functions) {
        write_string(name, out);
        out.write(reinterpret_cast<const char *>(&ip), sizeof(ip));
    }
    map_size = ins.exported_globals.size();
    out.write(reinterpret_cast<const char *>(&map_size), sizeof(map_size));
    for (const auto &[name, index] : ins.exported_globals) {
        write_string(name, out);
        out.write(reinterpret_cast<const char *>(&index), sizeof(index));
    }
    size = ins.code.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (auto &i : ins.code) {
        out.write(reinterpret_cast<const char *>(&i.op), sizeof(i.op));
        const uint32_t takes = takes_operand(i.op);
        for (int j=0;j<takes;j++) {
            out.write(reinterpret_cast<const char *>(&i.operands[j]), sizeof(uint64_t));
        }
    }
    size = ins.data.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (char i : ins.data) {
        out.write(&i, sizeof(i));
    }
}
} // namespace bvm
