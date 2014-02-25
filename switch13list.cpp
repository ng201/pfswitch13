#include "switch13list.h"

namespace pfswitch13{

    /// Parse the json object given.
    bool Switch13List::parse(std::list<vigil::json_object*> *connections,const std::string &as){
        using namespace vigil;
        using namespace vigil::json;

        //std::vector<Switch13*> temp; // temp storage for the paths
        Switch13List temp;

        std::list<json_object*>::iterator pit=connections->begin(); // connection iterator
        for(;pit!=connections->end();++pit){

            ipv4_addr src_ip;
            ipv4_addr dst_ip;
            std::string name;

            json_object *flow=get_dict_value(*pit,"flow"); // flow data
            if(!flow || flow->type!=json_object::JSONT_DICT){
                std::cerr<<"json error: flow\n";
                clear();
                return false;
            }

            json_object *ftmp=get_dict_value(flow,"src_ip");
            if(!ftmp && ftmp->type!=json_object::JSONT_STRING){
                std::cerr<<"json error: src_ip\n";
                clear();
                return false;
            }
            src_ip=ipaddr(*(char**)ftmp->object); //ipaddr can be converted to ipv4_addr
            ftmp=get_dict_value(flow,"dst_ip");
            if(!ftmp && ftmp->type!=json_object::JSONT_STRING){
                std::cerr<<"json error: dst_ip\n";
                clear();
                return false;
            }
            dst_ip=ipaddr(*(char**)ftmp->object); //ipaddr can be converted to ipv4_addr

            ftmp=get_dict_value(flow,"name");
            if(!ftmp && ftmp->type!=json_object::JSONT_STRING){
                std::cerr<<"json error: name\n";
                clear();
                return false;
            }
            name=std::string(*(char**)ftmp->object); //convert name


            json_object *paths=get_dict_value(*pit,"paths"); // flow data
            if(paths && paths->type==json_object::JSONT_ARRAY){
                if(!temp.parse_paths(name,src_ip,dst_ip,as,
                                     (std::list<vigil::json_object*> *)paths->object)){
                    temp.clear();
                    clear();
                    return false;
                }
            }
            else{
                std::cerr<<"json error: paths\n";
                clear();
                return false;
            }
            this->consume(temp);
        }

        return true;
    }


    bool Switch13List::parse_paths(const std::string &name,ipv4_addr src_ip,ipv4_addr dst_ip,const std::string &as,
                                  std::list<vigil::json_object*> *paths){

        using namespace vigil;
        using namespace vigil::json;

        size_t elements=paths->size();
        if(!elements) return false; // empty paths!
        PathHdr *hdr=new PathHdr[elements+1];
        PathHdr *hdr_tmp=hdr;
        unsigned in_port,dpid;
        unsigned curr_path=0;

        std::list<json_object*>::iterator pit=paths->begin(); // path iterator
        for(;pit!=paths->end();++pit,curr_path++){

            if((*pit)->type!=json_object::JSONT_DICT){
                return false;
            }
            if(!parse_path(name,src_ip,dst_ip,as,
                           (vigil::json_object*)(*pit),hdr_tmp++,&dpid,&in_port)){
                return false;
            }
            else if(curr_path==elements-1){
                if(elements>1){
                    std::cerr<<"creating switch S4\n";
                    push_back(new SwitchS4((unsigned)dpid,name,src_ip,dst_ip,SwitchData(0,as,in_port,0),&hdr));
                }
                else{
                    std::cerr<<"creating switch S1\n";
                    push_back(new SwitchS1((unsigned)dpid,name,src_ip,dst_ip,SwitchData(hdr->pid,as,in_port,hdr->out_port)));
                }
            }
        }
        return true;
    }


    bool Switch13List::parse_path(const std::string &name,ipv4_addr src_ip,ipv4_addr dst_ip,const std::string &as,
                                  vigil::json_object *path,
                                  PathHdr *hdr,unsigned *idpid,unsigned *iport){
        using namespace vigil;
        using namespace vigil::json;

        int pid=0;
        int dpid=0;
        int in_port=0;
        int out_port=0;

        json_object *ptmp=get_dict_value(path,"pid"); // path tmp
        if(ptmp && ptmp->type==json_object::JSONT_INTEGER){
            pid=*((int*)ptmp->object);
        }
        else{
            std::cerr<<"json error: pid\n";
            return false;
        }

        ptmp=get_dict_value(path,"actions");
        if(!ptmp || ptmp->type!=json_object::JSONT_ARRAY){
            std::cerr<<"json error: actions\n";
            return false;
        }

        std::cerr<<"reading actions\n";

        std::list<json_object*> *actions=(std::list<json_object*>*)ptmp->object;
        std::list<json_object*>::iterator ait=actions->begin(); // actions iterator
        size_t position=0,max_pos=actions->size();
        for(;ait!=actions->end();++ait,position++){
            json_object *atmp=get_dict_value(*ait,"dpid");
            if(atmp && atmp->type==json_object::JSONT_INTEGER){
                dpid=*((int*)atmp->object);
            }
            else{
               std::cerr<<"json error: dpid\n";
               return false;
            }

            atmp=get_dict_value(*ait,"in_port");
            if(atmp && atmp->type==json_object::JSONT_INTEGER){
                in_port=*((int*)atmp->object);
            }
            else{
                std::cerr<<"json error: in_port\n";
                return false;
            }

            atmp=get_dict_value(*ait,"out_port");
            if(atmp && atmp->type==json_object::JSONT_INTEGER){
                out_port=*((int*)atmp->object);
            }
            else{
                std::cerr<<"json error: out_port\n";
                return false;
            }

            if(position==0){
                *idpid=dpid;
                *iport=in_port;
                hdr->pid=pid;
                hdr->out_port=out_port;
            }
            else if(position==max_pos-1){ // last
                push_back(new SwitchS3((unsigned)dpid,name,SwitchData(pid,as,in_port,out_port)));
                std::cerr<<"creating switch S3\n";
            }
            else{
                push_back(new SwitchS2((unsigned)dpid,name,SwitchData(pid,as,in_port,out_port)));
                std::cerr<<"creating switch S2\n";
            }
        }

        return true;
    }



};