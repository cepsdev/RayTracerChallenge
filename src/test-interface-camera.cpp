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


template<> bool check<rt::camera_t>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "camera";
}

template<> bool check<rt::camera_t>(ceps::ast::node_t n)
{
    using namespace ceps::ast;
    if (!is<Ast_node_kind::structdef>(n)) return false;
    return check<rt::camera_t>(*as_struct_ptr(n));
}

template<> rt::camera_t fetch<rt::camera_t>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;
    decltype(rt::camera_t::hsize) hsize{};
    decltype(rt::camera_t::vsize) vsize{};
    decltype(rt::camera_t::field_of_view) field_of_view{};
    optional<decltype(rt::camera_t::transform)> t;
    
    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "hsize" && children(*as_struct_ptr(e)).size() && is<Ast_node_kind::int_literal>(children(*as_struct_ptr(e))[0])){
         hsize = value(as_int_ref(children(*as_struct_ptr(e))[0]));
     }
     else if (name(*as_struct_ptr(e)) == "vsize" && children(*as_struct_ptr(e)).size() && is<Ast_node_kind::int_literal>(children(*as_struct_ptr(e))[0])){
         vsize = value(as_int_ref(children(*as_struct_ptr(e))[0]));
     } else if (name(*as_struct_ptr(e)) == "field_of_view" && children(*as_struct_ptr(e)).size() && is<Ast_node_kind::float_literal>(children(*as_struct_ptr(e))[0])){
         field_of_view = value(as_double_ref(children(*as_struct_ptr(e))[0]));
     } else if (name(*as_struct_ptr(e)) == "transform" && children(*as_struct_ptr(e)).size() && is<Ast_node_kind::structdef>(children(*as_struct_ptr(e))[0])){
         t =  read_value<rt::matrix_t>(as_struct_ref(children(*as_struct_ptr(e))[0])) ;
     }
    }
    
    rt::camera_t r{hsize, vsize, field_of_view};
    if(t) r.transform = *t;
    return r;
}
template<> rt::camera_t fetch<rt::camera_t>(ceps::ast::node_t n)
{
    using namespace ceps::ast;
    return fetch<rt::camera_t>(*as_struct_ptr(n));
}
template<> ceps::ast::node_t ast_rep<rt::camera_t>(rt::camera_t c){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("camera");
    add_field(result,"hsize", mk_int_node(c.hsize));
    add_field(result,"vsize", mk_int_node(c.vsize));
    add_field(result,"field_of_view", ast_rep<rt::precision_t>(c.field_of_view));
    add_field(result,"pixel_size", mk_double_node(c.pixel_size(),all_zero_unit()));
    add_field(result,"transform", ast_rep<rt::matrix_t>(c.transform));
    return result;
}

static node_t handle_camera(Struct* op){
    auto c{read_value<rt::camera_t>(*op)};    
    if (!c) return mk_struct("error");
    return ast_rep(*c);
}

static node_t handle_ray_for_pixel(Struct* op){
    if (!children(*op).size() || !is<Ast_node_kind::structdef>(children(*op)[0])) return mk_struct("error");

    auto c{read_value<rt::camera_t>( as_struct_ref(children(*op)[0]))} ;        
    if (!c) return mk_struct("error");
    int px{};
    int py{};
    if (children(*op).size() > 1 && is<Ast_node_kind::int_literal>(children(*op)[1]))
     px = value(as_int_ref(children(*op)[1]));
    if (children(*op).size() > 2 && is<Ast_node_kind::int_literal>(children(*op)[2]))
     py = value(as_int_ref(children(*op)[2]));
    
    return ast_rep( c->ray_for_pixel(px,py));
}

void test_interface::register_ops(rt::camera_t){
    ops["camera"] = handle_camera;
    ops["ray_for_pixel"] = handle_ray_for_pixel;
}    

///// rt::World <<<<<<
