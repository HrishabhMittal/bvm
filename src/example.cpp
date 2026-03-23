#include <bvm.hpp>
#include <fstream>
#include <ios>
#include <iostream>
int main(int argc,char**argv) {
    if (argc!=2) {
        std::cout<<"provide file to dump example to"<<std::endl;
        std::cout<<"usage: "<<argv[0]<<" <filename>"<<std::endl;
        return 1;
    }
    std::ofstream file(argv[1],std::ios::binary);
    std::vector<instruction> example = {
        {OPCODE::PUSH,{5}},
        {OPCODE::CALL,{37}},    
        {OPCODE::PRINT_I64,{}}, 
        {OPCODE::PUSH,{5}},     
        {OPCODE::I32_TO_F64,{}},
        {OPCODE::PUSH,{2}},
        {OPCODE::I32_TO_F64,{}},
        {OPCODE::F64_DIV,{}},   
        {OPCODE::DUP,{}},       
        {OPCODE::F64_MULT,{}},  
        {OPCODE::PUSH,{314}},   
        {OPCODE::I32_TO_F64,{}},
        {OPCODE::PUSH,{100}},
        {OPCODE::I32_TO_F64,{}},
        {OPCODE::F64_DIV,{}},   
        {OPCODE::F64_MULT,{}},  
        {OPCODE::F64_TO_F32,{}},
        {OPCODE::PRINT_F32,{}}, 
        {OPCODE::PUSH,{4294967295}}, 
        {OPCODE::PUSH,{16}},
        {OPCODE::U32_SHR,{}},
        {OPCODE::PRINT_I32,{}},      
        {OPCODE::PUSH,{4}},
        {OPCODE::MALLOC,{}},
        {OPCODE::STORE,{0}},    
        {OPCODE::LOAD,{0}},     
        {OPCODE::PUSH,{0}},     
        {OPCODE::PUSH,{254}},   
        {OPCODE::I8_ASTORE,{}}, 
        {OPCODE::LOAD,{0}},     
        {OPCODE::PUSH,{0}},     
        {OPCODE::I8_ALOAD,{}},  
        {OPCODE::I32_WRAP_I64,{}},
        {OPCODE::PRINT_I32,{}}, 
        {OPCODE::LOAD,{0}},     
        {OPCODE::FREE,{}},      
        {OPCODE::HALT,{}},      
        {OPCODE::DUP,{}},       
        {OPCODE::PUSH,{2}},     
        {OPCODE::I64_CMP,{}},   
        {OPCODE::JLT,{47}},     
        {OPCODE::DUP,{}},       
        {OPCODE::PUSH,{1}},     
        {OPCODE::I64_SUB,{}},   
        {OPCODE::CALL,{37}},    
        {OPCODE::I64_MULT,{}},  
        {OPCODE::RET,{}},       
        {OPCODE::POP,{}},       
        {OPCODE::PUSH,{1}},     
        {OPCODE::RET,{}}        
    };
    dump_bytecode(example,file);
}
