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


///// rt::World >>>>>>

template<> bool check<rt::World>(ceps::ast::Struct & s)
{
    return name(s) == "world";
}

template<> bool check<rt::World>(ceps::ast::node_t n)
{
    if (!is<Ast_node_kind::structdef>(n)) return false;
    return check<rt::World>(*as_struct_ptr(n));
}

template<> rt::World fetch<rt::World>(ceps::ast::Struct& s)
{
    rt::World w{};
    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "objects" && children(*as_struct_ptr(e)).size() ){
        for (auto ee : children(*as_struct_ptr(e))){
            if (!is<Ast_node_kind::structdef>(ee)) continue;
            auto shape{read_value<rt::Shape*>(*as_struct_ptr(ee))};
            if(shape) w.objects.push_back(shared_ptr<rt::Shape>{*shape});
        }
     } else if (name(*as_struct_ptr(e)) == "lights" && children(*as_struct_ptr(e)).size() ){
        for (auto ee : children(*as_struct_ptr(e))){
            if (!is<Ast_node_kind::structdef>(ee)) continue;
            auto light{read_value<rt::point_light>( *as_struct_ptr(ee) )};
            if (light) w.lights.push_back(*light);
        }
     }
    }
    return w;
}

template<> rt::World fetch<rt::World>(ceps::ast::node_t n)
{
    return fetch<rt::World>(*as_struct_ptr(n));
}

template<> ceps::ast::node_t ast_rep<rt::World>(rt::World w){
    auto result = mk_struct("world");
    auto objs{add_field(result,"objects", nullptr)};
    auto lights{add_field(result,"lights", nullptr)};
    for(auto pl : w.lights){
        children(as_struct_ref(lights)).push_back(ast_rep<>(pl));
    }
    for(auto o : w.objects){
        children(as_struct_ref(objs)).push_back(ast_rep<>(o.get()));
    }
    return result;
}
 

rt::World default_world() {
    rt::World world;
    world.lights.push_back(rt::point_light{.position = {-10.0, 10.0, -10.0}, .intensity = {1,1,1}});
    std::shared_ptr<rt::Sphere> sphere1 {create<rt::Sphere>()};
    sphere1->material = {
        .color = {0.8,1.0,0.6},
        .diffuse = 0.7,
        .specular = 0.2
    };
    world.objects.push_back(sphere1);
    shared_ptr<rt::Sphere> sphere2 {create<rt::Sphere>()};
    sphere2->transformation = rt::scaling(0.5,0.5,0.5);
    world.objects.push_back(sphere2);
    return world; 
}

namespace test_interface{
    using namespace ceps::ast;
    using namespace std;
    using op_t = node_t (*) (node_struct_t);
    extern map<string, op_t> ops;   
    void register_ops(rt::World);
} 

static node_t handle_intersect_world(Struct* op){
    auto world{read_value<rt::World>(0,*op)};
    auto ray{read_value<rt::ray_t>(1,*op)};
    node_t result {};
    if (!world || !ray){
        result = mk_struct("error");
    }
    rt::intersections intersections_result{};
    for (auto shape : world->objects){
        auto r{shape->intersect(*ray)};
        intersections_result.append(r);
    }
    intersections_result.sort();
    return ast_rep(intersections_result);
}

static node_t handle_shade_hit(Struct* op){
    
    auto world{read_value<rt::World>(0,*op)};    
    auto comps{read_value<rt::prepare_computations_t>(1,*op)};
    if (!comps || !world) return mk_struct("error");
    auto c{shade_hit(*world,*comps)};    
    return ast_rep(c);
}

static node_t handle_color_at(Struct* op){
    auto world{read_value<rt::World>(0,*op)};    
    auto r{read_value<rt::ray_t>(1,*op)};
    if (!r || !world) return mk_struct("error");
    auto c{color_at(*world,*r)};    
    return ast_rep(c);
}

static node_t handle_is_shadowed(Struct* op){
    auto world{read_value<rt::World>(0,*op)};    
    auto p{read_value<rt::tuple_t>(1,*op)};
    if (!p || !world) return mk_struct("error");
    return ast_rep(rt::is_shadowed(*world, *p));
}

void test_interface::register_ops(rt::World){
    ops["intersect_world"] = handle_intersect_world;
    ops["shade_hit"] = handle_shade_hit;            
    ops["color_at"] = handle_color_at;
    ops["is_shadowed"] = handle_is_shadowed;            
}    

///// rt::World <<<<<<
