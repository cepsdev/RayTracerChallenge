
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

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "INSERT_NAME_HERE v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t obj_type_as_str(ceps::ast::node_callparameters_t params);
    ceps::ast::node_t op(ceps::ast::node_callparameters_t params);
}

#include <tuple>

//////////////////////////////////
///////////////////RT lives here//
//////////////////////////////////

template<typename T> struct smallness;
template<> struct smallness<double> { static constexpr double value = 0.00001;};


namespace rt{
    using precision_t = double;
    using color_prec_t = double;

    bool small(precision_t v) {
        return std::abs(v) < smallness<precision_t>::value;
    }
    
    using tuple_base_t = std::tuple<precision_t,precision_t,precision_t,precision_t>;
    struct tuple_t: public tuple_base_t{
        using val_t = precision_t;
        tuple_t() = default;
        tuple_t(precision_t a,precision_t b,precision_t c,precision_t d):tuple_base_t{a,b,c,d}{}
    };
    struct vector_t;struct point_t;
    
    struct point_t: private tuple_t{
        point_t() = default;
        point_t(precision_t a,precision_t b,precision_t c):tuple_t{a,b,c,1.0}{}
        friend point_t operator + (point_t , vector_t );
    };

    struct vector_t: private tuple_t{
        vector_t() = default;
        vector_t(precision_t a,precision_t b,precision_t c):tuple_t{a,b,c,0.0}{}
        friend point_t operator + (point_t , vector_t );
        friend vector_t operator + (vector_t , vector_t);
    };

    inline tuple_t operator + (tuple_t l, tuple_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r),get<3>(l)+get<3>(r)  };
    }
    inline point_t operator + (point_t l, vector_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r)  };
    }
    inline vector_t operator + (vector_t l, vector_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r)  };
    }

    inline tuple_t operator * (precision_t scalar, tuple_t const & t) { 
        return {scalar*get<0>(t), scalar*get<1>(t),scalar*get<2>(t),scalar*get<3>(t)  };}
    
    inline tuple_t operator - (tuple_t const & l, tuple_t const & r) { 
        return {get<0>(l)+ -1.0*get<0>(r),get<1>(l)+-1.0*get<1>(r),get<2>(l)+-1.0*get<2>(r),get<3>(l)+-1.0*get<3>(r)  };}
    inline precision_t norm_2(tuple_t t){
        return std::sqrt(get<0>(t)*get<0>(t) + get<1>(t)*get<1>(t)+get<2>(t)*get<2>(t)+get<3>(t)*get<3>(t)); }
    inline precision_t dot(tuple_t l, tuple_t r){
        return get<0>(l)*get<0>(r) + get<1>(l)*get<1>(r)+get<2>(l)*get<2>(r)+get<3>(l)*get<3>(r);
    }
    inline tuple_t cross (tuple_t v1, tuple_t v2) { 
        return { get<1>(v1)*get<2>(v2) - get<1>(v2) * get<2>(v1) , 
                 get<2>(v1)*get<0>(v2) - get<2>(v2)*get<0>(v1),
                 get<0>(v1)*get<1>(v2) - get<0>(v2)*get<1>(v1), 0.0 };}

    tuple_t  mk_tuple(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_tuple(tuple_t);

    inline color_prec_t clamp(color_prec_t v){
        return v > 1.0 ? 1.0 : (v < 0.0 ? 0.0 : v); 
    }

    struct color_t: private std::tuple<color_prec_t,color_prec_t,color_prec_t>{
        color_t() = default;
        color_t(color_prec_t r,color_prec_t g,color_prec_t b):tuple{r,g,b}{}
        color_prec_t r() const { return std::get<0>(*this);}
        color_prec_t g() const { return std::get<1>(*this);}
        color_prec_t b() const { return std::get<2>(*this);}

        friend color_t operator + (color_t , color_t );
        friend color_t operator - (color_t , color_t);
        friend color_t operator * (color_prec_t , color_t );
        friend color_t operator * (color_t , color_t);
        friend color_t clamp(color_t);
    };
    inline color_t clamp(color_t c){
        return { clamp(c.r()), clamp(c.g()), clamp(c.b()) };
    }

    inline color_t operator + (color_t l, color_t r) { 
        return {l.r()+r.r(),l.g()+r.g(),l.b()+r.b() };
    }

    inline color_t operator - (color_t l, color_t r) { 
        return {l.r()-r.r(),l.g()-r.g(),l.b()-r.b() };
    }
    
    inline color_t operator * (color_t l, color_t r) { 
        return {l.r()*r.r(),l.g()*r.g(),l.b()*r.b() };
    }
    inline color_t operator * (color_prec_t scalar, color_t l) { 
        return {scalar*l.r(),scalar*l.g(),scalar*l.b() };
    }

    color_t mk_color(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_color(color_t);

    class canvas_t{
        std::vector<color_t> data;
        public:
        canvas_t() = default;
        canvas_t(unsigned short width, unsigned short height):width{width}, height{height}{
            data.resize(width * height);
        }
        using iterator = std::vector<color_t>::iterator;
        using const_iterator = std::vector<color_t>::const_iterator;
        
        const_iterator begin() const {return data.begin();}
        const_iterator end() const {return data.end();}
        iterator begin() {return data.begin();}
        iterator end() {return data.end();}
        unsigned short width{};
        unsigned short height{};
        void write_pixel(unsigned short x, unsigned short y, color_t color){
            data[y*width+x] = color;
        }
        void write_pixel( unsigned int pos, color_t color){
            data[pos] = color;
        }
        color_t pixel_at(short x, short y) const {return data[y*width+x];}
    };
    canvas_t mk_canvas(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_canvas(canvas_t);

    struct ppm{
        canvas_t canvas;
        static int constexpr clamp_high_default = 255;
        static int constexpr max_line = 70;

        int clamp_high{clamp_high_default};
        ppm() = default;
        ppm(canvas_t canvas, int clamp_high = clamp_high_default) : canvas{canvas}{
            for(auto& c: this->canvas) c = clamp(c);
        }
        friend std::ostream& operator << (std::ostream& out, ppm const &);
    };

    std::ostream& operator << (std::ostream& out, ppm const & p){
        out << "P3\n";
        out << p.canvas.width << ' ';
        out << p.canvas.height << '\n';
        out << p.clamp_high << '\n';
        int current_line_chars_written = 0;
        int pixels_written = 0;

        auto write_color_tuple = [&](int v){
            std::stringstream s;
            s << v;
            auto num_of_chars_to_write = s.str().length();
            int blanks_before =  current_line_chars_written == 0 ? 0 : 1;
            if (num_of_chars_to_write + blanks_before + current_line_chars_written > ppm::max_line) {
                out << '\n'; current_line_chars_written = 0;blanks_before = 0;
            }
            out << (blanks_before == 0 ? "" : " ") << s.str();
            current_line_chars_written += blanks_before + num_of_chars_to_write;
        };

        for (auto c : p.canvas){
            write_color_tuple(std::round(c.r() * p.clamp_high));
            write_color_tuple(std::round(c.g() * p.clamp_high));
            write_color_tuple(std::round(c.b() * p.clamp_high));

            ++pixels_written;
            if (pixels_written == p.canvas.width){
                if (current_line_chars_written) out << "\n";
                pixels_written = 0;current_line_chars_written = 0;
            } else if (current_line_chars_written >= ppm::max_line){
                out << "\n";current_line_chars_written = 0;
            }
        }
        if (current_line_chars_written) out << '\n';
        return out;
    }

    struct matrix_t{
            using prec_t = precision_t;
            using dim_t = unsigned int;

            dim_t dim_y{};
            dim_t dim_x{};
            std::vector<prec_t> data;
            matrix_t() = default;
            matrix_t(unsigned int dim_y, unsigned int dim_x):
            dim_y{dim_y}, dim_x{dim_x} {data.resize(dim_y*dim_x);};
            using iterator = std::vector<prec_t>::iterator;
            using const_iterator = std::vector<prec_t>::const_iterator;
            iterator begin(){return data.begin();}
            iterator end(){return data.end();}
            const_iterator begin() const {return data.begin();}
            const_iterator end() const {return data.end();}
            prec_t& at(dim_t y, dim_t x) {return data[x + y*dim_x];}
            prec_t  at(dim_t y, dim_t x) const {return data[x + y*dim_x];}
    };
    matrix_t mk_matrix(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_matrix(matrix_t);
    matrix_t operator - (matrix_t const & m1, matrix_t const & m2){
        matrix_t r{std::min(m1.dim_y,m2.dim_y),std::min(m1.dim_x,m2.dim_x)};
        auto it1 = m1.begin();
        auto it2 = m2.begin();
        for(auto it = r.begin(); it != r.end(); ++it,++it1,++it2)
          *it = *it1 - *it2;

        return r;
    }
    matrix_t operator * (matrix_t const & m1, matrix_t const & m2){
        matrix_t r{m1.dim_y,m2.dim_x};
        for(matrix_t::dim_t i{}; i < r.dim_y; ++i)
         for(matrix_t::dim_t j{}; j < r.dim_x; ++j)
          {
            matrix_t::prec_t v{};
            for (matrix_t::dim_t k {}; k < m2.dim_x; ++k)
             v += m1.at(i,k) * m2.at(k,j);
            r.at(i,j) = v; 
          }

        return r;
    }

    inline precision_t norm_2(matrix_t m){
        precision_t acc{};
        for(auto c : m) acc += c*c;
        return std::sqrt(acc); 
    }

}

//////////////////////////////////////////////
///////////////////Test Interface lives here//
//////////////////////////////////////////////


rt::matrix_t rt::mk_matrix(ceps::ast::Struct s) {
    auto& v{children(s)}; 
    using namespace ceps::ast;
    rt::matrix_t::dim_t dim[] = { {},{} };
    size_t dim_pos{};
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
            auto col = mk_color(*as_struct_ptr(*iter));
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

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    /*cout << "cepsplugin::plugin_entrypoint:\n";
    for(auto e : children(ceps_struct)){
        cout <<"\t"<< * e << "\n";
    }
    cout <<"\n\n";*/

    if (name(ceps_struct) == "tuple" || name(ceps_struct) == "point" || name(ceps_struct) == "vector"){
        return rt::mk_tuple(tuple_from_ceps(ceps_struct));
    } else if (name(ceps_struct) == "color"){
        return rt::mk_color(rt::mk_color(ceps_struct));
    } else if (name(ceps_struct) == "canvas"){
        return rt::mk_canvas(rt::mk_canvas(ceps_struct));
    } else if (name(ceps_struct) == "matrix"){
        return rt::mk_matrix(rt::mk_matrix(ceps_struct));
    }

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
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[0]) 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[1])
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

            auto n2 = rt::norm_2(l - r);
            return mk_int_node(1 ? rt::small(n2): 0);
        } else if (children(ceps_struct).size() > 1 
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[0])
            && is<Ast_node_kind::float_literal>(children(ceps_struct)[1])){
            auto l = value(as_double_ref(children(ceps_struct)[0]));
            auto r = value(as_double_ref(children(ceps_struct)[0]));
            auto n2 = std::abs(l - r);
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
				
                