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
#pragma once

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"


ceps::ast::node_t add_field(ceps::ast::Struct* s, ceps::ast::node_t n);

ceps::ast::node_t add_field(ceps::ast::Struct* s,std::string name, ceps::ast::node_t n);

template<typename T> T fetch(ceps::ast::Struct&);
template<typename T> T fetch(ceps::ast::node_t);

template<typename T> 
std::optional<T> read_value(ceps::ast::Struct& s);
template<typename T> 
std::optional<T> read_value(size_t idx, ceps::ast::Struct& s);
template<typename T>
 class Serializer;
template<typename T> bool check(ceps::ast::Struct&);
template<typename T> bool check(ceps::ast::node_t);

template<typename T> 
std::optional<T> read_value(size_t idx, ceps::ast::Struct& s){
    auto & v{children(s)};
    if(v.size() <= idx || !check<T>(v[idx])) return {};
    return fetch<T>(v[idx]);
}

template<typename T> 
std::optional<T> read_value(ceps::ast::Struct& s){
    if(!check<T>(s)) return {};
    return fetch<T>(s);
}

template<typename T> ceps::ast::node_t ast_rep (T entity);
template<typename T> ceps::ast::node_t ast_rep (std::string field_name, T value);
template<typename T> ceps::ast::node_t ast_rep (T* entity);
template<typename T> T* create();
