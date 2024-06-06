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

///// rt::World >>>>>>

template<> bool check<rt::World>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    return name(s) == "world";
}

template<> bool check<rt::World>(ceps::ast::node_t n)
{
    using namespace ceps::ast;
    if (!is<Ast_node_kind::structdef>(n)) return false;
    return check<rt::World>(*as_struct_ptr(n));
}

template<> rt::World fetch<rt::World>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;
    rt::World w{};
    for (auto e : children(s)){
     if (!is<Ast_node_kind::structdef>(e)) continue;
     if (name(*as_struct_ptr(e)) == "objects" && children(*as_struct_ptr(e)).size() ){
        for (auto ee : children(*as_struct_ptr(e))){
            if (!is<Ast_node_kind::structdef>(ee)) continue;
            auto shape{read_value<rt::Shape*>(*as_struct_ptr(ee))};
        }
     } else if (name(*as_struct_ptr(e)) == "lights" && children(*as_struct_ptr(e)).size() ){
        for (auto ee : children(*as_struct_ptr(e))){
            if (!is<Ast_node_kind::structdef>(ee)) continue;
            auto light{read_value<rt::point_light>( *as_struct_ptr(ee) )};
        }
     }
    }
    return w;
}
template<> rt::World fetch<rt::World>(ceps::ast::node_t n)
{
    using namespace ceps::ast;
    return fetch<rt::World>(*as_struct_ptr(n));
}

template<> ceps::ast::node_t ast_rep<rt::World>(rt::World w){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
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
    std::shared_ptr<rt::Sphere> sphere2 {create<rt::Sphere>()};
    sphere2->transformation = rt::scaling(0.5,0.5,0.5);
    world.objects.push_back(sphere2);
    return world; 
}

///// rt::World <<<<<<
