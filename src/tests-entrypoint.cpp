
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <netinet/sctp.h> 
#include <cmath>
#include <vector>
#include <tuple>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"
#include "rt.hpp"

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t obj_type_as_str(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t op(ceps::ast::node_callparameters_t params);
}

//////////////////////////////////////////////
///////////////////Test Interface lives here//
//////////////////////////////////////////////


rt::matrix_t rt::mk_matrix(ceps::ast::Struct s) {
    auto& v{children(s)}; 
    using namespace ceps::ast;
    rt::matrix_t::dim_t dim[] = { {},{} };
    size_t dim_pos{};
    if (v.size() == 1 && is<Ast_node_kind::identifier>(v[0]) && ( name(as_id_ref(v[0])) == "id" || name(as_id_ref(v[0])) == "identity_matrix") ) return rt::id_4_4;
    for(auto iter = v.begin(); iter != v.end() && dim_pos < sizeof(dim)/sizeof(rt::matrix_t::dim_t); ++iter)
    {
     if (is<Ast_node_kind::int_literal>(*iter)) 
      dim[dim_pos++] = value(as_int_ref(*iter));
     else if (is<Ast_node_kind::structdef>(*iter)){
        auto& sub = *as_struct_ptr(*iter);
        auto& nm = name(sub);
        auto& ch = children(sub);
        if (nm == "dim"){
            if (ch.size() && is<Ast_node_kind::int_literal>(ch[0]))
              dim[0] = value(as_int_ref(ch[0]));
            if (ch.size() > 1 && is<Ast_node_kind::int_literal>(ch[1]))
              dim[1] = value(as_int_ref(ch[1]));
        }
     }    
    }
    matrix_t m{dim[0],dim[1]};
    for(auto iter = v.begin(); iter != v.end() ; ++iter)
    {
     if (is<Ast_node_kind::structdef>(*iter)){
        auto& sub = *as_struct_ptr(*iter);
        auto& nm = name(sub);
        auto& ch = children(sub);
        if (nm == "data"){
            auto it = m.begin();
            for( auto e : ch){
                if(it == m.end()) break;
                if (!is<Ast_node_kind::float_literal>(e)) continue;
                *it = value(as_double_ref(e));
                ++it;
            }
            break;
        }
     }
    }
    return m;   
}

ceps::ast::node_struct_t rt::mk_matrix(rt::matrix_t matrix){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    auto result = mk_struct("matrix");
    auto dim = mk_struct("dim");
    auto data = mk_struct("data");
    children(*result).push_back(dim);
    children(*result).push_back(data);
    for(auto v : matrix){
     children(*data).push_back(mk_double_node(v,all_zero_unit()));
    }
    children(*dim).push_back(mk_int_node(matrix.dim_y));
    children(*dim).push_back(mk_int_node(matrix.dim_x));
    return result;
}  


rt::tuple_t rt::mk_tuple(ceps::ast::Struct s){
    auto& v{children(s)}; using namespace ceps::ast;

    rt::tuple_t::val_t t[] = { rt::tuple_t::val_t{},rt::tuple_t::val_t{},rt::tuple_t::val_t{},rt::tuple_t::val_t{} };
    for(auto iter = v.begin(); iter != v.end(); ++iter)
     if (is<Ast_node_kind::float_literal>(*iter)){
        t[iter - v.begin()] = value(as_double_ref(*iter));
     } else if (is<Ast_node_kind::structdef>(*iter)){
        auto& sub = *as_struct_ptr(*iter);
        auto& nm = name(sub);
        auto& ch = children(sub);
        if (ch.size() == 0) continue;
        if (is<Ast_node_kind::float_literal>(ch[0])){
            if (nm == "x") t[0] = value(as_double_ref(ch[0]));
            else if (nm == "y") t[1] = value(as_double_ref(ch[0]));
            else if (nm == "z") t[2] = value(as_double_ref(ch[0]));
            else if (nm == "w") t[3] = value(as_double_ref(ch[0]));
        }
     }
      
//v.size() > 0 && is<Ast_node_kind::float_literal>(v[3]) ? value(as_double_ref(v[3])) :rt::tuple_t::val_t{}

    return rt::tuple_t{t[0],t[1],t[2],t[3]};
}

ceps::ast::node_struct_t rt::mk_tuple(rt::tuple_t tuple){
    using namespace ceps::ast;using namespace ceps::interpreter;
    auto result = mk_struct("tuple");
    auto x = mk_struct("x");auto y = mk_struct("y");auto z = mk_struct("z");auto w = mk_struct("w");
    children(*result).push_back(x);
    children(*result).push_back(y);
    children(*result).push_back(z);
    children(*result).push_back(w);
    
    children(*x).push_back(mk_double_node(get<0>(tuple),all_zero_unit()));
    children(*y).push_back(mk_double_node(get<1>(tuple),all_zero_unit()));
    children(*z).push_back(mk_double_node(get<2>(tuple),all_zero_unit()));
    children(*w).push_back(mk_double_node(get<3>(tuple),all_zero_unit()));
    return result;
}  

rt::color_t rt::mk_color(ceps::ast::Struct s){
    using namespace ceps::ast;
    auto& v{children(s)};
    color_prec_t t[] = { color_prec_t{},color_prec_t{},color_prec_t{}};
    for(auto iter = v.begin(); iter != v.end(); ++iter)
     if (is<Ast_node_kind::float_literal>(*iter)){
        t[iter - v.begin()] = value(as_double_ref(*iter));
     } else if (is<Ast_node_kind::structdef>(*iter)){
        auto& sub = *as_struct_ptr(*iter);
        auto& nm = name(sub);
        auto& ch = children(sub);
        if (ch.size() == 0) continue;
        if (is<Ast_node_kind::float_literal>(ch[0])){
            if (nm == "r" || nm == "red") t[0] = value(as_double_ref(ch[0]));
            else if (nm == "g" || nm == "green") t[1] = value(as_double_ref(ch[0]));
            else if (nm == "b" || nm == "blue") t[2] = value(as_double_ref(ch[0]));
        }
     }
    color_t r {t[0],t[1],t[2]};
    return r;
}
ceps::ast::node_struct_t rt::mk_color(rt::color_t color){
    using namespace ceps::ast;using namespace ceps::interpreter;
    auto result = mk_struct("color");
    auto r = mk_struct("red");auto g = mk_struct("green");auto b = mk_struct("blue");
    children(*result).push_back(r);
    children(*result).push_back(g);
    children(*result).push_back(b);
    
    children(*r).push_back(mk_double_node(color.r(),all_zero_unit()));
    children(*g).push_back(mk_double_node(color.g(),all_zero_unit()));
    children(*b).push_back(mk_double_node(color.b(),all_zero_unit()));
    return result;
}  

rt::canvas_t rt::mk_canvas(ceps::ast::Struct s){
    using namespace ceps::ast;
    using namespace std;
    auto& v{children(s)};
    unsigned short dim[] = { 0,0};
    node_struct_t data{};
    for(auto iter = v.begin(); iter != v.end(); ++iter)
     if (is<Ast_node_kind::int_literal>(*iter)){
        dim[iter - v.begin()] = static_cast<unsigned short>(value(as_int_ref(*iter)));
     } else if (is<Ast_node_kind::structdef>(*iter) && name(*as_struct_ptr(*iter)) == "data" ){
        data = as_struct_ptr(*iter);
     } else if (is<Ast_node_kind::structdef>(*iter)){
        auto& sub = *as_struct_ptr(*iter);
        auto& nm = name(sub);
        auto& ch = children(sub);
        if (ch.size() == 0) continue;
        if (is<Ast_node_kind::int_literal>(ch[0])){
            if (nm == "w" || nm == "width") dim[0] = static_cast<unsigned short>(value(as_int_ref(ch[0])));
            else if (nm == "h" || nm == "height") dim[1] = static_cast<unsigned short>(value(as_int_ref(ch[0])));
        }
     }
    canvas_t canvas{dim[0],dim[1]};
    if(data) {
        auto& v{children(data)};
        for (auto iter = v.begin(); iter != v.end(); ++iter){
            if (!is<Ast_node_kind::structdef>(*iter)) continue;
            canvas.write_pixel(iter - v.begin(), mk_color(*as_struct_ptr(*iter)) );
        }
    }
    return canvas;
}

ceps::ast::node_struct_t rt::mk_canvas(rt::canvas_t canvas){
    using namespace ceps::ast;using namespace ceps::interpreter;
    auto result = mk_struct("canvas");
    auto w = mk_struct("width");auto h = mk_struct("height");
    children(*result).push_back(w);
    children(*result).push_back(h);
    children(*w).push_back(mk_int_node(canvas.width));
    children(*h).push_back(mk_int_node(canvas.height));

    auto d = mk_struct("data");
    children(*result).push_back(d);
    for(auto c : canvas){
        children(*d).push_back(mk_color(c));
    }
    return result;
}


rt::tuple_t tuple_from_ceps(ceps::ast::Struct& ceps_struct){
    if (name(ceps_struct) == "tuple"){
        return rt::mk_tuple(ceps_struct);
    }
    if (name(ceps_struct) == "point"){
        auto tuple{rt::mk_tuple(ceps_struct)};
        get<3>(tuple) = 1.0;
        return tuple;
    }
    if (name(ceps_struct) == "vector"){
        auto tuple{rt::mk_tuple(ceps_struct)};
        get<3>(tuple) = 0.0;
        return tuple;
    }
    return {};
}

// Sane part starts here
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////

template <typename T> std::string type_name();
template <> std::string type_name<rt::intersections>(){
    return "intersections";
}
template <> std::string type_name<rt::intersection>(){
    return "intersection";
}


template<typename T>
 class Serializer;

template<>
 class Serializer<rt::UnknownShape> : public rt::Serializer{
     ceps::ast::node_t serialize(rt::Shape&) override{
        auto t {ceps::ast::mk_struct("shape")};
        return t;
     }    
 };

template<>
 class Serializer<rt::Sphere> : public rt::Serializer{
     ceps::ast::node_t serialize(rt::Shape&) override{
        auto t {ceps::ast::mk_struct("sphere")};
        return t;
     }    
 };

template<typename T> bool check(ceps::ast::Struct&);
template<typename T> bool check(ceps::ast::node_t);
template<> bool check<double>(ceps::ast::node_t n){ return n && ceps::ast::is<ceps::ast::Ast_node_kind::float_literal>(n); }

template<typename T> T fetch(ceps::ast::node_t);
template<> double fetch<double>(ceps::ast::node_t n){ return value(as_double_ref(n));}

template<> bool check<rt::tuple_t>(ceps::ast::node_t n){ 
    using namespace ceps::ast; 
 return n && 
  is<Ast_node_kind::structdef>(n) && 
  children(*as_struct_ptr(n)).size() > 3; }

template<> bool check<rt::ray_t>(ceps::ast::node_t n){ 
 using namespace ceps::ast; 
 return n && 
  is<Ast_node_kind::structdef>(n) && 
  children(*as_struct_ptr(n)).size() > 1 &&
  is<Ast_node_kind::structdef>(children(*as_struct_ptr(n))[0]) &&
  is<Ast_node_kind::structdef>(children(*as_struct_ptr(n))[1]) ; }

template<> bool check<rt::sphere_t>(ceps::ast::node_t n){ 
 using namespace ceps::ast; 
 return n && 
  is<Ast_node_kind::structdef>(n);
}
template<typename T> T* create();
template<> rt::UnknownShape* create(){
    static Serializer<rt::UnknownShape>s{};
    return new rt::UnknownShape{s};
}
template<> rt::Sphere* create(){
    static Serializer<rt::Sphere>s{};
    return new rt::Sphere{s};
}

template<typename T> T fetch(ceps::ast::Struct&);

template<typename T> 
std::optional<T> read_value(ceps::ast::Struct& s);
template<typename T> 
std::optional<T> read_value(size_t idx, ceps::ast::Struct& s);
template<> 
std::optional<rt::Shape*> read_value(size_t idx, ceps::ast::Struct& s){
    using namespace ceps::ast;

    if (is<Ast_node_kind::structdef>(children(s)[idx])
        && ceps::ast::name(*as_struct_ptr(children(s)[idx])) == "sphere"){
        return create<rt::Sphere>();    
    }
    return {};    
}


template<> rt::tuple_t fetch<rt::tuple_t>(ceps::ast::node_t n){ 
    using namespace ceps::ast;
    using namespace rt;
    auto& v {children(*as_struct_ptr(n))};

    return tuple_t{
        fetch<tuple_t::val_t>( children(*as_struct_ptr(v[0]))[0]),
        fetch<tuple_t::val_t>( children(*as_struct_ptr(v[1]))[0]),
        fetch<tuple_t::val_t>( children(*as_struct_ptr(v[2]))[0]),
        fetch<tuple_t::val_t>( children(*as_struct_ptr(v[3]))[0])
    };
}

template<> rt::ray_t fetch<rt::ray_t>(ceps::ast::node_t n){ 
    using namespace ceps::ast;
    using namespace rt;
    using namespace std;
    auto& v {children(*as_struct_ptr(n))};

    return { fetch<tuple_t>(children(*as_struct_ptr(v[0]))[0]), fetch<tuple_t>(children(*as_struct_ptr(v[1]))[0])};
}

template<> rt::sphere_t fetch<rt::sphere_t>(ceps::ast::node_t n){ 
    using namespace ceps::ast;
    using namespace rt;
    using namespace std;
    auto& v {children(*as_struct_ptr(n))};

    return {};
}


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

template<> ceps::ast::node_t ast_rep<double>(std::string field_name, double value){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    auto f = mk_struct(field_name);
    children(*f).push_back(mk_double_node(value,all_zero_unit()));
    return f;    
}

template<> ceps::ast::node_t ast_rep<rt::ray_t>(rt::ray_t ray){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("ray");
    auto o = mk_struct("origin");
    auto d = mk_struct("direction");
    children(*result).push_back(o);
    children(*result).push_back(d);
    children(*o).push_back(mk_tuple(ray.origin));
    children(*d).push_back(mk_tuple(ray.direction));
    return result;
}

ceps::ast::node_t add_field(ceps::ast::Struct* s, ceps::ast::node_t n){
    children(*s).push_back(n);
    return n;
}

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



template<> ceps::ast::node_t ast_rep<rt::tuple_t>(rt::tuple_t t){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("tuple");
    auto x = mk_struct("x");
    auto y = mk_struct("y");
    auto z = mk_struct("z");
    auto w = mk_struct("w");
    
    children(*result).push_back(x);
    children(*result).push_back(y);
    children(*result).push_back(z);
    children(*result).push_back(w);

    children(*x).push_back(mk_double_node(std::get<0>(t),all_zero_unit()));
    children(*y).push_back(mk_double_node(std::get<1>(t),all_zero_unit()));
    children(*z).push_back(mk_double_node(std::get<2>(t),all_zero_unit()));
    children(*w).push_back(mk_double_node(std::get<3>(t),all_zero_unit()));

    return result;
}

template<> ceps::ast::node_t ast_rep<rt::sphere_t>(rt::sphere_t t){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("sphere");
    return result;
}

template<> ceps::ast::node_t ast_rep<rt::intersect_result_t>(rt::intersect_result_t t){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct("intersect_result");
    if (t.size()){
        children(*result).push_back(mk_double_node(t[0],all_zero_unit()));
        if (t.size() > 1) children(*result).push_back(mk_double_node(t[1],all_zero_unit()));
    }
    return result;
}

///// rt::intersection >>>>>>
template<> bool check<rt::intersection>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    if (children(s).size() < 2)  return false;
    if (!is<Ast_node_kind::structdef>(children(s)[0])&&
         !is<Ast_node_kind::float_literal>(children(s)[0]))  return false;
    if (is<Ast_node_kind::structdef>(children(s)[0]) &&
       (name(*as_struct_ptr(children(s)[0])) != "t" ||
        children(*as_struct_ptr(children(s)[0])).size() != 1 ||
        !is<Ast_node_kind::float_literal>(children(*as_struct_ptr(children(s)[0]))[0] ))  )
     return false;
    return true;
}

template<> rt::intersection fetch<rt::intersection>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;
    auto t{read_value<double>(0,s)};
    auto shape{read_value<rt::Shape*>(1,s)};
    if (!t)
     t = read_value<double>(0,*as_struct_ptr(children(s)[0]));
    if (!shape)
     shape = read_value<rt::Shape*>(0,*as_struct_ptr(children(s)[1]));
    
    if (!t || !shape) return {};
    return {*t,*shape};
}

template<> ceps::ast::node_t ast_rep<rt::intersection>(rt::intersection inter){
    using namespace ceps::ast;
    using namespace ceps::interpreter;    
    auto result = mk_struct(type_name<rt::intersection>());
    add_field(result,ast_rep<double>("t",inter.t));
    add_field(result,ast_rep<rt::Shape*>("object",inter.obj));
    return result;
}

template<> bool check<rt::intersection>(ceps::ast::node_t n){
    using namespace ceps::ast;
    if (!is<Ast_node_kind::structdef>(n)) return false;
    if ("intersection" != name(*as_struct_ptr(n))) return false;
    return check<rt::intersection>(*as_struct_ptr(n));
}

///// rt::intersection <<<<<<

///// rt::intersections >>>>>>


template<> bool check<rt::intersections>(ceps::ast::Struct & s)
{
    using namespace ceps::ast;
    for (auto e : children(s) ) 
     if (!check<rt::intersection>(e)) return false;
    return true;
}

template<> rt::intersections fetch<rt::intersections>(ceps::ast::Struct& s)
{
    using namespace ceps::ast;
    rt::intersections r{};
    for (auto e : children(s))
     r.add(fetch<rt::intersection>(*as_struct_ptr(e)));
    return r;
}

template<> ceps::ast::node_t ast_rep<rt::intersections>(rt::intersections t){
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    
    auto result = mk_struct(type_name<rt::intersections>());
    for(auto e : t)
     add_field(result, ast_rep<rt::intersection>(e));
    return result;
}
 
///// rt::intersections <<<<<<

namespace rt2ceps{
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    using namespace std;    

    template <typename T> node_t rt_obj(Struct& s){
            auto t{read_value<T>(s)};
            if (t) return ast_rep(*t);
            return nullptr;
    }

}

///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;
    static std::map<std::string, node_t (*) (Struct& )> n2f{ 
        {type_name<rt::intersection>(), rt2ceps::rt_obj<rt::intersection>},
        {type_name<rt::intersections>(), rt2ceps::rt_obj<rt::intersections>}
    };



    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    auto& nm {name(ceps_struct)};

    if (nm == "tuple" || nm == "point" || nm == "vector"){
        return rt::mk_tuple(tuple_from_ceps(ceps_struct));
    } else if (nm == "color"){
        return rt::mk_color(rt::mk_color(ceps_struct));
    } else if (nm == "canvas"){
        return rt::mk_canvas(rt::mk_canvas(ceps_struct));
    } else if (nm == "matrix"){
        return rt::mk_matrix(rt::mk_matrix(ceps_struct));
    } else if (nm == "translation" || nm == "scaling"){
        auto x{read_value<double>(0,ceps_struct)};
        auto y{read_value<double>(1,ceps_struct)};
        auto z{read_value<double>(2,ceps_struct)};
        if (x && y && z) return rt::mk_matrix( nm == "translation" ? 
         rt::translation(*x,*y,*z) : 
         rt::scaling(*x,*y,*z));
    } else if (nm == "rotation_x" || nm == "rotation_y" || nm == "rotation_z"){
        auto rad{read_value<double>(0,ceps_struct)};
        
        if(rad) return rt::mk_matrix(  nm=="rotation_x" ? rt::rotation_x(*rad) :
                        (nm=="rotation_y" ? rt::rotation_y(*rad) : rt::rotation_z(*rad) ));
    } else if (nm == "shearing"){        
        auto x_by_y{read_value<double>(0,ceps_struct)};
        auto x_by_z{read_value<double>(1,ceps_struct)};
        auto y_by_x{read_value<double>(2,ceps_struct)};
        auto y_by_z{read_value<double>(3,ceps_struct)};
        auto z_by_x{read_value<double>(4,ceps_struct)};
        auto z_by_y{read_value<double>(5,ceps_struct)};
        if (x_by_y && x_by_z && y_by_x && y_by_z && z_by_x && z_by_y)
         return rt::mk_matrix(rt::shearing(*x_by_y, *x_by_z, *y_by_x, *y_by_z, *z_by_x, *z_by_y));
    } else if (nm == "ray"){
        auto origin{read_value<rt::tuple_t>(0,ceps_struct)};
        auto direction{read_value<rt::tuple_t>(1,ceps_struct)};
        if (origin && direction)
         return ast_rep(rt::ray_t{*origin,*direction});
    } else if (nm == "sphere"){
         return ast_rep(rt::sphere_t{});
    }   
    
    auto it{n2f.find(nm)};
    if (it != n2f.end() ) return it->second(ceps_struct);
    
    auto result = mk_struct("error");
    children(*result).push_back(mk_int_node(0));
    return result;
}

ceps::ast::node_t cepsplugin::op(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto data = get_first_child(params);   
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    if (name(ceps_struct) == "plus"){
        if (children(ceps_struct).size() > 1){
            if (
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "color") && 
                (name(*as_struct_ptr(children(ceps_struct)[1])) == "color") ){
                auto l = rt::mk_color(*as_struct_ptr(children(ceps_struct)[0]));
                auto r = rt::mk_color(*as_struct_ptr(children(ceps_struct)[1]));
                auto result = l + r;
                return rt::mk_color(result);
            } else{
                auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
                auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
                auto result = l + r;
                return rt::mk_tuple(result);
            }
        }
    } else if (name(ceps_struct) == "minus"){
        if (children(ceps_struct).size() > 1){
            if (
                is<Ast_node_kind::structdef>(children(ceps_struct)[0]) &&
                is<Ast_node_kind::structdef>(children(ceps_struct)[1]) &&
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "color") && 
                (name(*as_struct_ptr(children(ceps_struct)[1])) == "color") ){
                auto l = rt::mk_color(*as_struct_ptr(children(ceps_struct)[0]));
                auto r = rt::mk_color(*as_struct_ptr(children(ceps_struct)[1]));
                auto result = l - r;
                return rt::mk_color(result);
            } else  if (
                is<Ast_node_kind::structdef>(children(ceps_struct)[0]) &&
                is<Ast_node_kind::structdef>(children(ceps_struct)[1]) &&
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "matrix") && 
                (name(*as_struct_ptr(children(ceps_struct)[1])) == "matrix") ) {
                auto l = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
                auto r = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[1]));
                auto result = l - r;
                return rt::mk_matrix(result);
                
            }else{
                auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
                auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
                auto result = l - r;
                return rt::mk_tuple(result);
            }
        }
    } else if (name(ceps_struct) == "negate" || name(ceps_struct) == "neg"){
        if (children(ceps_struct).size() > 0){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto result = -1.0 * l;
            return rt::mk_tuple(result);
        }
    }else if (name(ceps_struct) == "magnitude"){
        if (children(ceps_struct).size() > 0){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto result = rt::norm_2(l);
            return mk_double_node(result,all_zero_unit());
        }
    } else  if (name(ceps_struct) == "multiply"){
        if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[1]))=="color" ){
            auto l = value(as_double_ref(children(ceps_struct)[0]));
            auto r = rt::mk_color(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = l * r;
            return rt::mk_color(result);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="color"
            && name(*as_struct_ptr(children(ceps_struct)[1]))=="color" 
            ){
            auto l = rt::mk_color(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = rt::mk_color(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = l * r;
            return rt::mk_color(result);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[1]))=="tuple"){
            auto l = value(as_double_ref(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = l * r;
            return rt::mk_tuple(result);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"
            && name(*as_struct_ptr(children(ceps_struct)[1]))=="matrix"){
            auto l = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[1]));
            if (l.dim_x != r.dim_y)     {
                auto result = mk_struct("dimensions_dont_match");
                return result;
            }
            auto result = l * r;
            return rt::mk_matrix(result);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"
            && name(*as_struct_ptr(children(ceps_struct)[1]))=="tuple"){
            auto l = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = rt::mk_tuple(*as_struct_ptr(children(ceps_struct)[1]));
            if (l.dim_y != 4)     {
                auto result = mk_struct("dimensions_dont_match");
                return result;
            }
            auto result = l * r;
            return rt::mk_tuple(result);
        }
    } else  if (name(ceps_struct) == "divide"){
        if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[1]) 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0])){
            auto l = value(as_double_ref(children(ceps_struct)[1]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto result = 1/l * r;
            return rt::mk_tuple(result);
        }
    }else  if (name(ceps_struct) == "normalize"){
        if (children(ceps_struct).size() > 0 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0])){
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto n2 = rt::norm_2(r);
            return rt::mk_tuple(1/n2 * r);
        }
    } else  if (name(ceps_struct) == "approx_equal"){
        if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0])
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[0])) == "tuple"
            && name(*as_struct_ptr(children(ceps_struct)[1])) == "tuple"){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));

            auto n2 = rt::norm_2(l - r);
            return mk_int_node(1 ? rt::small(n2): 0);
        } else  if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0])
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])            
            && name(*as_struct_ptr(children(ceps_struct)[0])) == "matrix"
            && name(*as_struct_ptr(children(ceps_struct)[1])) == "matrix"){
            auto l = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[1]));

            auto n2 = rt::norm_max(l - r);
            return mk_int_node(1 ? rt::small(n2): 0);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[0])
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[1])){
            auto l = value(as_double_ref(children(ceps_struct)[0]));
            auto r = value(as_double_ref(children(ceps_struct)[1]));
            auto n2 = std::abs(l - r);
            return mk_int_node(1 ? rt::small(n2): 0);
        } else  if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0])
            && is<Ast_node_kind::structdef>(children(ceps_struct)[1])
            && name(*as_struct_ptr(children(ceps_struct)[0])) == "color"
            && name(*as_struct_ptr(children(ceps_struct)[1])) == "color"){
            auto l = rt::mk_color(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = rt::mk_color(*as_struct_ptr(children(ceps_struct)[1]));

            auto n2 = rt::norm_2(l - r);
            return mk_int_node(1 ? rt::small(n2): 0);
        } 
    } else if (name(ceps_struct) == "dot"){
        if (children(ceps_struct).size() > 1){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = rt::dot(l,r);
            return mk_double_node(result,all_zero_unit());
        }
    } else if (name(ceps_struct) == "cross"){
        if (children(ceps_struct).size() > 1){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = rt::cross(l,r);
            return rt::mk_tuple(result);
        }
    } else if (name(ceps_struct) == "write_pixel") {
        if (children(ceps_struct).size() > 3){
            if (
                is<Ast_node_kind::structdef>(children(ceps_struct)[0]) &&
                is<Ast_node_kind::structdef>(children(ceps_struct)[3]) &&
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "canvas") &&
                is<Ast_node_kind::int_literal>(children(ceps_struct)[1]) &&
                is<Ast_node_kind::int_literal>(children(ceps_struct)[2]) &&
                (name(*as_struct_ptr(children(ceps_struct)[3])) == "color") ){
                auto col = rt::mk_color(*as_struct_ptr(children(ceps_struct)[3]));
                short x = value(as_int_ref(children(ceps_struct)[1]));
                short y = value(as_int_ref(children(ceps_struct)[2]));
                auto canvas = rt::mk_canvas(*as_struct_ptr(children(ceps_struct)[0]));                
                canvas.write_pixel(x,y,col);
                return rt::mk_canvas(canvas);
            }
        }
    } else if (name(ceps_struct) == "pixel_at") {
        if (children(ceps_struct).size() > 2){
            if (
                is<Ast_node_kind::structdef>(children(ceps_struct)[0]) &&
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "canvas") &&
                is<Ast_node_kind::int_literal>(children(ceps_struct)[1]) &&
                is<Ast_node_kind::int_literal>(children(ceps_struct)[2])  ){
                short x = value(as_int_ref(children(ceps_struct)[1]));
                short y = value(as_int_ref(children(ceps_struct)[2]));
                auto canvas = rt::mk_canvas(*as_struct_ptr(children(ceps_struct)[0]));                
                return rt::mk_color(canvas.pixel_at(x,y));
            }
        }
    } else if (name(ceps_struct) == "canvas_to_ppm") {
        if (children(ceps_struct).size() > 0){
            if (
                is<Ast_node_kind::structdef>(children(ceps_struct)[0]) &&
                (name(*as_struct_ptr(children(ceps_struct)[0])) ==  "canvas")){
                auto canvas = rt::mk_canvas(*as_struct_ptr(children(ceps_struct)[0]));
                rt::ppm p{canvas};
                std::stringstream s;
                s << p;                
                return mk_string(s.str());
            }
        }
    } else if (name(ceps_struct) == "write_file") {
        if (children(ceps_struct).size() > 0){
            if (is<Ast_node_kind::string_literal>(children(ceps_struct)[0]) &&
                is<Ast_node_kind::string_literal>(children(ceps_struct)[1])){
                auto ppm = value(as_string_ref(children(ceps_struct)[0]));
                auto ppm_file = value(as_string_ref(children(ceps_struct)[1]));
                std::ofstream f{ppm_file};
                f << ppm;                
                return mk_int_node((bool)f); 
            }
        }
    } else  if (name(ceps_struct) == "transpose"){
        if (children(ceps_struct).size() > 0 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            return rt::mk_matrix(rt::transpose(m));
        } 
    } else  if (name(ceps_struct) == "det"){
        if (children(ceps_struct).size() > 0 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            return mk_double_node(rt::det(m),all_zero_unit());
        } 
    } else  if (name(ceps_struct) == "sub_matrix"){
        if (children(ceps_struct).size() > 2 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[1])
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[2])
            ){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto row = value(as_int_ref(children(ceps_struct)[1]));
            auto col = value(as_int_ref(children(ceps_struct)[2]));

            return rt::mk_matrix(rt::sub_matrix(m,row,col));
        } 
    } else  if (name(ceps_struct) == "minor"){
        if (children(ceps_struct).size() > 2 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[1])
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[2])
            ){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto row = value(as_int_ref(children(ceps_struct)[1]));
            auto col = value(as_int_ref(children(ceps_struct)[2]));

            return mk_double_node(rt::minor(m,row,col),all_zero_unit());
        } 
    } else  if (name(ceps_struct) == "cofactor"){
        if (children(ceps_struct).size() > 2 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[1])
            && is<Ast_node_kind::int_literal>(children(ceps_struct)[2])
            ){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            auto row = value(as_int_ref(children(ceps_struct)[1]));
            auto col = value(as_int_ref(children(ceps_struct)[2]));

            return mk_double_node(rt::cofactor(m,row,col),all_zero_unit());
        } 
    } else  if (name(ceps_struct) == "invertible"){
        if (children(ceps_struct).size() > 0 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            return mk_int_node(rt::invertible(m));
        } 
    } else  if (name(ceps_struct) == "inverse"){
        if (children(ceps_struct).size() > 0 
            && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) 
            && name(*as_struct_ptr(children(ceps_struct)[0]))=="matrix"){
            auto m = rt::mk_matrix(*as_struct_ptr(children(ceps_struct)[0]));
            return rt::mk_matrix(rt::inverse(m));
        } 
    } else  if (name(ceps_struct) == "position"){
        auto ray{read_value<rt::ray_t>(0,ceps_struct)};
        auto t{read_value<double>(1,ceps_struct)};
        if(ray && t) 
         return ast_rep(rt::postion(*ray,*t));
    } else  if (name(ceps_struct) == "intersect"){
        auto shape{read_value<rt::Shape*>(0,ceps_struct)};
        auto ray{read_value<rt::ray_t>(1,ceps_struct)};
        if(ray && shape) 
         return ast_rep( (*shape)->intersect(*ray));
    } 
    auto result = mk_struct("error");
    children(*result).push_back(mk_int_node(0));
    return result;
}

ceps::ast::node_t cepsplugin::obj_type_as_str(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    if (name(ceps_struct) == "tuple"){
        auto t {rt::mk_tuple(ceps_struct)};
        if (get<3>(t) == 0.0) return mk_string("vector"); 
        if (get<3>(t) == 1.0) return mk_string("point"); 
    } 
    return mk_string(name(ceps_struct));
}


extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("rt_obj", cepsplugin::plugin_entrypoint);
  cepsplugin::plugin_master->reg_ceps_phase0plugin("rt_obj_type_as_str", cepsplugin::obj_type_as_str);
  cepsplugin::plugin_master->reg_ceps_phase0plugin("rt_op", cepsplugin::op);
}					
				
                