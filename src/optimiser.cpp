#include <bvm.hpp>

int main(int argc, char **argv) {
    if (argc != 3)
        return 1;
    std::ifstream file(argv[1], std::ios::binary);
    auto v = bvm::load_bytecode(file);
    file.close();
    bvm::optimiser op(v);
    op.run_all_optimisations();
    std::ofstream out_file(argv[2], std::ios::binary);
    bvm::dump_bytecode(v,out_file);
    out_file.close();
}
