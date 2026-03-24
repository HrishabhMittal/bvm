#include <bvm.hpp>
#include <fstream>
#include <ios>
#include <iostream>
using namespace bvm;
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "provide file to run" << std::endl;
        std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    auto code = load_bytecode(file);
    VM vm(code);
    vm.exec();
    auto status = vm.return_value();
    std::cout << "process exited with value " << status << std::endl;
    return 0;
}
