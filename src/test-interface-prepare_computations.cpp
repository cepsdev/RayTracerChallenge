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
    return name(s) == "prep_comps";
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
        /*for (auto ee : children(*as_struct_ptr(e))){
            if (!is<Ast_node_kind::structdef>(ee)) continue;
            auto shape{read_value<rt::Shape*>(*as_struct_ptr(ee))};
            if(shape) w.objects.push_back(shared_ptr<rt::Shape>{*shape});
        }*/
     } 
    }//for
    return pc;
}

template<> rt::prepare_computations_t fetch<rt::prepare_computations_t>(ceps::ast::node_t n)
{
    return fetch<rt::prepare_computations_t>(*as_struct_ptr(n));
}

template<> ceps::ast::node_t ast_rep<rt::prepare_computations_t>(rt::prepare_computations_t pc){
    auto result = mk_struct("prep_comps");
    /*auto objs{add_field(result,"objects", nullptr)};
    auto lights{add_field(result,"lights", nullptr)};
    for(auto pl : w.lights){
        children(as_struct_ref(lights)).push_back(ast_rep<>(pl));
    }
    for(auto o : w.objects){
        children(as_struct_ref(objs)).push_back(ast_rep<>(o.get()));
    }*/
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
