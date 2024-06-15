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

template<> bool check<rt::prepare_computations_t>(ceps::ast::Struct & s)
{
    return name(s) == "prepare_computations";
}

template<> bool check<rt::prepare_computations_t>(ceps::ast::node_t n)
{
    if (!is<Ast_node_kind::structdef>(n)) return false;
    return check<rt::prepare_computations_t>(*as_struct_ptr(n));
}

template<> rt::prepare_computations_t fetch<rt::prepare_computations_t>(ceps::ast::Struct& s)
{
    rt::prepare_computations_t pc{};
    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "object" && children(*as_struct_ptr(e)).size() ){
        auto shape{read_value<rt::Shape*>(*as_struct_ptr(children(*as_struct_ptr(e))[0]))};
        if (shape) pc.object = *shape;
     } else if (name(*as_struct_ptr(e)) == "t" && children(*as_struct_ptr(e)).size() ){
        auto t{fetch<rt::tuple_t::val_t>(children(*as_struct_ptr(e))[0])};
        if (t) pc.t = t;
     } else if (name(*as_struct_ptr(e)) == "point" && children(*as_struct_ptr(e)).size() ){
        auto p{fetch<rt::tuple_t>(children(*as_struct_ptr(e))[0])};
        pc.point = p;
     } else if (name(*as_struct_ptr(e)) == "eyev" && children(*as_struct_ptr(e)).size() ){
        auto v{fetch<rt::tuple_t>(children(*as_struct_ptr(e))[0])};
        pc.eyev = v;
     } else if (name(*as_struct_ptr(e)) == "normal_v" && children(*as_struct_ptr(e)).size() ){
        auto v{fetch<rt::tuple_t>(children(*as_struct_ptr(e))[0])};
        pc.normal_v = v;
     } else if (name(*as_struct_ptr(e)) == "inside" && children(*as_struct_ptr(e)).size() ){
        auto b{fetch<bool>(children(*as_struct_ptr(e))[0])};
        pc.inside = b;
     } 
 
    }//for
    return pc;
}

template<> rt::prepare_computations_t fetch<rt::prepare_computations_t>(ceps::ast::node_t n)
{
    return fetch<rt::prepare_computations_t>(*as_struct_ptr(n));
}

template<> ceps::ast::node_t ast_rep<rt::prepare_computations_t>(rt::prepare_computations_t pc){
    //Assumptions w.r.t. pc
    ((pc.t));
    ((pc.object));
    ((pc.point));
    ((pc.eyev));
    ((pc.normal_v));
    ((pc.inside));

    auto result = mk_struct("prepare_computations");
    add_field(result,"t", ast_rep(pc.t));
    if (pc.object) add_field(result,"object", ast_rep(pc.object));
    else add_field(result,"object", mk_undef());
    add_field(result,"point", ast_rep<rt::tuple_t>(pc.point));
    add_field(result,"eyev", ast_rep<rt::tuple_t>(pc.eyev));
    add_field(result,"normal_v", ast_rep<rt::tuple_t>(pc.normal_v));
    add_field(result,"inside", ast_rep<bool>(pc.inside));
    return result;
}
 

namespace test_interface{
    using namespace ceps::ast;
    using namespace std;
    using op_t = node_t (*) (node_struct_t);
    extern map<string, op_t> ops;   
    void register_ops(rt::prepare_computations_t);
} 

static node_t handle_prepare_computations(Struct* pc){
    auto inter{read_value<rt::intersection>(0,*pc)};
    auto ray{read_value<rt::ray_t>(1,*pc)};
    node_t result {};
    if (!inter || !ray){
        result = mk_struct("error");
    }
    return ast_rep(rt::prepare_computations(*inter,*ray));
}

void test_interface::register_ops(rt::prepare_computations_t){
    ops["prepare_computations"] = handle_prepare_computations;            
}    

///// rt::World <<<<<<
