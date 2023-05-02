
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

namespace rt{
    using precision_t = double;
    using tuple_base_t = std::tuple<precision_t,precision_t,precision_t,precision_t>;
    struct tuple_t: public tuple_base_t{
        using val_t = precision_t;
        tuple_t() = default;
        tuple_t(precision_t a,precision_t b,precision_t c,precision_t d):tuple_base_t{a,b,c,d}{}
    };

    inline tuple_t operator + (tuple_t l, tuple_t r) { return {get<0>(l)+get<0>(r),get<1>(l)+get<1>(r),get<2>(l)+get<2>(r),get<3>(l)+get<3>(r)  };}
    inline tuple_t operator * (precision_t scalar, tuple_t t) { 
        return {scalar*get<0>(t), scalar*get<1>(t),scalar*get<2>(t),scalar*get<3>(t)  };}
    inline tuple_t operator - (tuple_t l, tuple_t r) { 
        return {get<0>(l)+ -1.0*get<0>(r),get<1>(l)+-1.0*get<1>(r),get<2>(l)+-1.0*get<2>(r),get<3>(l)+-1.0*get<3>(r)  };}

    tuple_t  mk_tuple(ceps::ast::Struct);
    ceps::ast::node_struct_t mk_tuple(tuple_t);  
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
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = l + r;
            return rt::mk_tuple(result);
        }
    } else if (name(ceps_struct) == "minus"){
        if (children(ceps_struct).size() > 1){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto r = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[1]));
            auto result = l - r;
            return rt::mk_tuple(result);
        }
    } else if (name(ceps_struct) == "negate" || name(ceps_struct) == "neg"){
        if (children(ceps_struct).size() > 0){
            auto l = tuple_from_ceps(*as_struct_ptr(children(ceps_struct)[0]));
            auto result = -1.0 * l;
            return rt::mk_tuple(result);
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
				
                