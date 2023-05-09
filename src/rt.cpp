
#include "ceps_ast.hh"
#include "rt.hpp"


template<> struct smallness<double> { static constexpr double value = 0.00001;};

namespace rt{
    bool small(precision_t v) {
        return std::abs(v) < smallness<precision_t>::value;
    }

    tuple_t operator + (tuple_t l, tuple_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r),get<3>(l)+get<3>(r)  };
    }
    point_t operator + (point_t l, vector_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r)  };
    }
    vector_t operator + (vector_t l, vector_t r) { 
        return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r)  };
    }

    tuple_t operator * (precision_t scalar, tuple_t const & t) { 
        return {scalar*get<0>(t), scalar*get<1>(t),scalar*get<2>(t),scalar*get<3>(t)  };
    }
        
    tuple_t operator - (tuple_t const & l, tuple_t const & r) { 
        return {get<0>(l)+ -1.0*get<0>(r),get<1>(l)+-1.0*get<1>(r),get<2>(l)+-1.0*get<2>(r),get<3>(l)+-1.0*get<3>(r)  };
    }

    precision_t norm_2(tuple_t t){
        return std::sqrt(get<0>(t)*get<0>(t) + get<1>(t)*get<1>(t)+get<2>(t)*get<2>(t)+get<3>(t)*get<3>(t)); 
    }

    precision_t dot(tuple_t l, tuple_t r){
            return get<0>(l)*get<0>(r) + get<1>(l)*get<1>(r)+get<2>(l)*get<2>(r)+get<3>(l)*get<3>(r);
    }

    tuple_t cross (tuple_t v1, tuple_t v2) { 
        return { get<1>(v1)*get<2>(v2) - get<1>(v2) * get<2>(v1) , 
                 get<2>(v1)*get<0>(v2) - get<2>(v2)*get<0>(v1),
                 get<0>(v1)*get<1>(v2) - get<0>(v2)*get<1>(v1), 0.0 };
    }


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
    tuple_t operator * (matrix_t const & m, tuple_t t){
        using namespace std;
        tuple_t r{};
        get<0>(r) = m.at(0,0) * get<0>(t) +  m.at(0,1) * get<1>(t) +  m.at(0,2) * get<2>(t) +  m.at(0,3) * get<3>(t);
        get<1>(r) = m.at(1,0) * get<0>(t) +  m.at(1,1) * get<1>(t) +  m.at(1,2) * get<2>(t) +  m.at(1,3) * get<3>(t);
        get<2>(r) = m.at(2,0) * get<0>(t) +  m.at(2,1) * get<1>(t) +  m.at(2,2) * get<2>(t) +  m.at(2,3) * get<3>(t);;
        get<3>(r) = m.at(3,0) * get<0>(t) +  m.at(3,1) * get<1>(t) +  m.at(3,2) * get<2>(t) +  m.at(3,3) * get<3>(t);;
        return r;
    }

}