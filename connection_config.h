#ifndef _CONNECTION_CONFIG_H_
#define _CONNECTION_CONFIG_H_

#include <vector>

#include "vlog.hh"

#include "json_object.hh"
#include "json-util.hh"

#include "qpf_types.h"
#include "path_config.h"

struct FlowPrimitive{

    QPF_IP_ADDRESS_TYPE src_ip;
    QPF_IP_ADDRESS_TYPE dst_ip;
    int src_dpid;
    int dst_dpid;
    int in_port;
    int out_port;

    PathList paths;

    FlowPrimitive(int src_ip,int dst_ip,
                  int src_dpid,int dst_dpid,
                  int in_port,int out_port):src_ip(src_ip),dst_ip(dst_ip),src_dpid(src_dpid),dst_dpid(dst_dpid),in_port(in_port),out_port(out_port){}

}; // structFlowPrimitive

class ConnectionList:public std::vector<FlowPrimitive>{

    public:

        bool parse(std::list<vigil::json_object*> *connections){
            using namespace vigil;
            using namespace vigil::json;

            std::list<json_object*>::iterator pit=connections->begin(); // connection iterator
            for(;pit!=connections->end();++pit){

                int src_ip=0;
                int dst_ip=0;
                int in_port=0;
                int out_port=0;
                int src_dpid=0;
                int dst_dpid=0;
                json_object *flow=get_dict_value(*pit,"flow"); // flow data
                if(!flow || flow->type!=json_object::JSONT_DICT){
                    clear();
                    return false;
                }

                json_object *ftmp=get_dict_value(flow,"src_ip");
                if(!ftmp && ftmp->type!=QPF_IP_ADDRESS_JSON){
                    clear();
                    return false;
                }
                src_ip=*(QPF_IP_ADDRESS_TYPE*)ftmp->object;
                ftmp=get_dict_value(flow,"dst_ip");
                if(!ftmp && ftmp->type!=QPF_IP_ADDRESS_JSON){
                    clear();
                    return false;
                }
                dst_ip=*(QPF_IP_ADDRESS_TYPE*)ftmp->object;
                ftmp=get_dict_value(flow,"in_port");
                if(!ftmp && ftmp->type!=json_object::JSONT_INTEGER){
                    clear();
                    return false;
                }
                in_port=*(int*)ftmp->object;
                ftmp=get_dict_value(flow,"out_port");
                if(!ftmp && ftmp->type!=json_object::JSONT_INTEGER){
                    clear();
                    return false;
                }
                out_port=*(int*)ftmp->object;
                ftmp=get_dict_value(flow,"src_dpid");
                if(!ftmp && ftmp->type!=json_object::JSONT_INTEGER){
                    clear();
                    return false;
                }
                src_dpid=*(int*)ftmp->object;
                ftmp=get_dict_value(flow,"dst_dpid");
                if(!ftmp && ftmp->type!=json_object::JSONT_INTEGER){
                    clear();
                    return false;
                }
                dst_dpid=*(int*)ftmp->object;

                FlowPrimitive fp(src_ip,dst_ip,src_dpid,dst_dpid,in_port,out_port);

                json_object *paths=get_dict_value(*pit,"paths"); // flow data
                if(paths && paths->type==json_object::JSONT_ARRAY){
                    if(!fp.paths.parse((std::list<vigil::json_object*> *)paths->object)){
                        clear();
                        return false;
                    }
                }
                else{
                    clear();
                    return false;
                }

                push_back(fp);

            }

            return true;

        }

        bool load(std::list<vigil::json_object*> *connections){
            return parse(connections);
        }

}; // class ConnectionList

#endif // _CONNECTION_CONFIG_H_
