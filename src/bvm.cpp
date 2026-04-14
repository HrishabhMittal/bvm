#include <bvm.hpp>
#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
using namespace bvm;
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "provide file to run" << std::endl;
        std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    auto code = load_bytecode(file);
    VM vm(code);
    for (int i = 2; i < argc; i++) {
        std::ifstream mod_file(argv[i], std::ios::binary);
        if (mod_file) {
            auto mod_code = load_bytecode(mod_file);
            vm.load_module(mod_code);
        } else {
            std::cerr << "couldnt open module: " << argv[i] << std::endl;
        }
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    vm.exec();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto status = vm.return_value();
    std::cout << "process exited with value: " << status << std::endl
              << "time taken: "
              << (std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time)).count() << "ms"
              << std::endl;
    return 0;
}
