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

template<> ceps::ast::node_t ast_rep<rt::Shape>(rt::Shape* shape){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    auto& ser{shape->get_serializer()};
    return ser.serialize(*shape);
}

template<> ceps::ast::node_t ast_rep<rt::Shape*>(std::string field_name, rt::Shape* shape){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    auto f = mk_struct(field_name);
    children(*f).push_back(ast_rep<rt::Shape>(shape));
    return f;    
}

template<> bool check<rt::Shape*>(ceps::ast::Struct& s);
template<> bool check<rt::Shape*>(ceps::ast::node_t  n){
    using namespace ceps::ast;
    if (!is<Ast_node_kind::structdef>(n)) return false;
    return check<rt::Shape*>(*as_struct_ptr(n));
    return true;
}


template<> bool check<rt::Shape*>(ceps::ast::Struct& s){
    using namespace ceps::ast;
    if (name(s) != "sphere") return false;
    return true;
}

template<> rt::Shape* fetch<rt::Shape*>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;

    rt::Shape* result{};
    if (name(s) == "sphere"){ 
     result = create<rt::Sphere>();
     if (children(s).size() == 0) return result;
     for (auto e : children(s)){
        if (is<Ast_node_kind::structdef>(e) && name(*as_struct_ptr(e)) == "transform" ){
            auto m {read_value<rt::matrix_t>( *as_struct_ptr(children(*as_struct_ptr(e))[0])  )};
            if (m) result->transformation = *m;
        } else if (is<Ast_node_kind::structdef>(e) && name(*as_struct_ptr(e)) == "material" ){
            auto m {read_value<rt::material_t>( *as_struct_ptr(e)  )};
            if (m) result->material = *m;
        }
     }      
    }    
    return result;
}

template<> rt::Shape* fetch<rt::Shape*>(ceps::ast::node_t  n)
{
    return fetch<rt::Shape*>(*ceps::ast::as_struct_ptr(n));
}
