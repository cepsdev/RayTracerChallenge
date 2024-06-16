/*
MIT License

Copyright (c) 2023,2024 Tomas Prerovsky

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "rt.hpp"
#include "test-interface-base.hpp"
using namespace ceps::ast;
using namespace ceps::interpreter;
using namespace std;

namespace test_interface{
    using namespace ceps::ast;
    using namespace std;
    using op_t = node_t (*) (node_struct_t);
    extern map<string, op_t> ops;
    void register_ops(rt::camera_t);
}


static node_t handle_camera(Struct* op){
    auto from{read_value<rt::tuple_t>(0,*op)};    
    auto to{read_value<rt::tuple_t>(1,*op)};
    auto up{read_value<rt::tuple_t>(2,*op)};
    if (!from || !to || !up) return mk_struct("error");
    return ast_rep(rt::view_transformation(*from, *to, *up));
}

void test_interface::register_ops(rt::camera_t){
    ops["camera"] = handle_camera;
}    

///// rt::World <<<<<<
