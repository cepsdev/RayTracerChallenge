#pragma once

#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include <vector>
#include <tuple>
#include "ceps_ast.hh"

template<typename T> struct smallness;

namespace rt{
    using precision_t = double;
    using color_prec_t = double;

    bool small(precision_t v);
    
    using tuple_base_t = std::tuple<precision_t,precision_t,precision_t,precision_t>;
    struct tuple_t: public tuple_base_t{
        using val_t = precision_t;
        tuple_t() = default;
        tuple_t(precision_t a,precision_t b,precision_t c,precision_t d):tuple_base_t{a,b,c,d}{}
    };
    struct vector_t;struct point_t;
    
    struct point_t: public tuple_t{
        point_t() = default;
        point_t(precision_t a,precision_t b,precision_t c):tuple_t{a,b,c,1.0}{}
        friend point_t operator + (point_t , vector_t );
    };

    struct vector_t: public tuple_t{
        vector_t() = default;
        vector_t(precision_t a,precision_t b,precision_t c):tuple_t{a,b,c,0.0}{}
        friend point_t operator + (point_t , vector_t );
        friend vector_t operator + (vector_t , vector_t);
    };

    tuple_t operator + (tuple_t l, tuple_t r);
    point_t operator + (point_t l, vector_t r);
    vector_t operator + (vector_t l, vector_t r);
    tuple_t operator * (precision_t scalar, tuple_t const & t);
    tuple_t operator - (tuple_t const & l, tuple_t const & r);
    precision_t norm_2(tuple_t t);

    precision_t dot(tuple_t l, tuple_t r);
    tuple_t cross (tuple_t v1, tuple_t v2);

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
    precision_t norm_2(color_t t);

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

    std::ostream& operator << (std::ostream& out, ppm const & p);

    struct matrix_t{
            using prec_t = precision_t;
            using dim_t = unsigned int;
            using idx_t = dim_t;


            dim_t dim_y{};
            dim_t dim_x{};
            std::vector<prec_t> data;
            matrix_t() = default;
            matrix_t(unsigned int dim_y, unsigned int dim_x):
            dim_y{dim_y}, dim_x{dim_x} {data.resize(dim_y*dim_x);};
            matrix_t(unsigned int dim_y, unsigned int dim_x, std::vector<prec_t> const & data):
            dim_y{dim_y}, dim_x{dim_x}, data{data} {};
            matrix_t(matrix_t const & rhs): dim_y{rhs.dim_y}, dim_x{rhs.dim_x}, data{rhs.data} {}

            using iterator = std::vector<prec_t>::iterator;
            using const_iterator = std::vector<prec_t>::const_iterator;
            iterator begin(){return data.begin();}
            iterator end(){return data.end();}
            const_iterator begin() const {return data.begin();}
            const_iterator end() const {return data.end();}
            prec_t& at(dim_t y, dim_t x) {return data[x + y*dim_x];}
            prec_t  at(dim_t y, dim_t x) const {return data[x + y*dim_x];}
    };

    static matrix_t id_4_4{4,4,{1.0,0.0,0.0,0.0,
                                0.0,1.0,0.0,0.0,
                                0.0,0.0,1.0,0.0,
                                0.0,0.0,0.0,1.0 }};

    matrix_t mk_matrix(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_matrix(matrix_t);
    matrix_t operator - (matrix_t const & m1, matrix_t const & m2);
    matrix_t operator * (matrix_t const & m1, matrix_t const & m2);
    inline precision_t norm_2(matrix_t m){
        precision_t acc{};
        for(auto c : m) acc += c*c;
        return std::sqrt(acc); 
    }
    inline precision_t norm_max(matrix_t m){
        precision_t r{};
        for(auto c : m) if (std::abs(c) > r) r = std::abs(c);
        return r; 
    }
    tuple_t operator * (matrix_t const & m, tuple_t t);
    matrix_t transpose(matrix_t const &);
    matrix_t::prec_t det(matrix_t const &);
    matrix_t sub_matrix(matrix_t const &,  int,  int);
    matrix_t::prec_t minor(matrix_t const &,  int,  int);
    matrix_t::prec_t cofactor(matrix_t const &,  int,  int);
    bool invertible(matrix_t const &);
    matrix_t inverse(matrix_t const &);
    matrix_t translation(tuple_t::val_t, tuple_t::val_t,tuple_t::val_t );
    matrix_t scaling(tuple_t::val_t, tuple_t::val_t,tuple_t::val_t );
    matrix_t rotation_x(tuple_t::val_t);
    matrix_t rotation_y(tuple_t::val_t);
    matrix_t rotation_z(tuple_t::val_t);
    matrix_t shearing(tuple_t::val_t, tuple_t::val_t,tuple_t::val_t,tuple_t::val_t, tuple_t::val_t,tuple_t::val_t );

    struct ray_t{
        tuple_t origin;
        tuple_t direction;
        ray_t() = default;
        ray_t(tuple_t origin, tuple_t direction ):origin{origin}, direction{direction}{}
    };

    tuple_t postion(ray_t, tuple_t::val_t);
    struct sphere_t{
        tuple_t::val_t radius{1.0};
        tuple_t center{0.0,0.0,0.0,1.0 };
        sphere_t() = default;
    };

    struct intersect_result_t{
        std::optional<std::tuple<tuple_t::val_t,tuple_t::val_t>> is{};
        intersect_result_t() = default;
        intersect_result_t(tuple_t::val_t v1,tuple_t::val_t v2): is{std::tuple<tuple_t::val_t,tuple_t::val_t>{ v1,v2} } {} 
        size_t size() {if (!is) return 0; return 2;}
        tuple_t::val_t operator [](size_t idx) {
            if (!is) return {};
            if (idx == 0) return std::get<0>(*is);
            else return std::get<1>(*is);
        }
    };
    template<typename T> intersect_result_t intersect(T obj, ray_t);

    class Shape;
    struct intersections;
    
    class Serializer{
        public:
            virtual ceps::ast::node_t serialize(Shape&) = 0;
    };

    class Shape{
        Serializer& serializer;
        public:
         Shape(Serializer& serializer):serializer{serializer}{}
         virtual intersections intersect(ray_t) = 0;
         Serializer& get_serializer() {return serializer;}
    };

    struct intersection{
        tuple_t::val_t t;
        Shape* obj;
    };

    struct intersections{
        std::vector<intersection> is;
        intersections() = default;
        void add(intersection i) {is.push_back(i);}
        std::vector<intersection>::iterator begin() {return is.begin();}
        std::vector<intersection>::const_iterator begin() const {return is.begin();}
        std::vector<intersection>::iterator end() {return is.end();}
        std::vector<intersection>::const_iterator end() const {return is.end();}
        std::optional<intersection> hit() const;
    };

    class UnknownShape : public Shape{
        public:
         UnknownShape(Serializer& serializer):Shape{serializer}{}
         intersections intersect(ray_t) override{
            return {};
         }
    };

    class Sphere : public Shape, public sphere_t{
        public:
         Sphere(Serializer& serializer):Shape{serializer}{}
         intersections intersect(ray_t) override;
    };
}
