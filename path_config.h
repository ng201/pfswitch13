#ifndef _PATH_CONFIG_H_
#define _PATH_CONFIG_H_

#include <vector>

#include "vlog.hh"

#include "json_object.hh"
#include "json-util.hh"

//extern vigil::Vlog_module log;

struct PathNode{

    int pid;
    int dpid;
    int in_port;
    int out_port;

    PathNode(int pid,int dpid,int in_port,int out_port):pid(pid),dpid(dpid),in_port(in_port),out_port(out_port){}

}; // struct PathNode

class PathPrimitive:public std::vector<PathNode>{

    public:

        bool parse(vigil::json_object *path){
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
                std::cout<<"pid\n";
                //VLOG_ERR(log,"Path error: cannot read pid.");
                clear();
                return false;
            }

            ptmp=get_dict_value(path,"actions");
            if(!ptmp || ptmp->type!=json_object::JSONT_ARRAY){
                //VLOG_ERR(log,"Path error: cannot construct action fo path with pid=%d.",pid);
                clear();
                return false;
            }

            std::cout<<"action\n";
            std::list<json_object*> *actions=(std::list<json_object*>*)ptmp->object;
            std::list<json_object*>::iterator ait=actions->begin(); // actions iterator
            for(;ait!=actions->end();++ait){
                json_object *atmp=get_dict_value(*ait,"dpid");
                if(atmp && atmp->type==json_object::JSONT_INTEGER){
                    dpid=*((int*)atmp->object);
                }
                else{
                   std::cout<<"dpid\n";
                    //VLOG_ERR(log,"Path error: cannot read dpid for path with pid=%d.",pid);
                    clear();
                    return false;
                }

                atmp=get_dict_value(*ait,"in_port");
                if(atmp && atmp->type==json_object::JSONT_INTEGER){
                    in_port=*((int*)atmp->object);
                }
                else{
                   std::cout<<"inport\n";
                    //VLOG_ERR(log,"Path error: cannot read in_port for path with pid=%d.",pid);
                    clear();
                    return false;
                }

                atmp=get_dict_value(*ait,"out_port");
                if(atmp && atmp->type==json_object::JSONT_INTEGER){
                    out_port=*((int*)atmp->object);
                }
                else{
                   std::cout<<"outport\n";
                    //VLOG_ERR(log,"Path error: cannot read out_port for path with pid=%d.",pid);
                    clear();
                    return false;
                }

                push_back(PathNode(pid,dpid,in_port,out_port));

            }

            return true;
        }

        bool load(vigil::json_object *path){
            return parse(path);
        }

}; // class Path

class PathList:public std::vector<PathPrimitive>{

    public:

        bool parse(std::list<vigil::json_object*> *paths){
            using namespace vigil;
            using namespace vigil::json;

            std::list<json_object*>::iterator pit=paths->begin(); // path iterator
            for(;pit!=paths->end();++pit){
                PathPrimitive p;
                if((*pit)->type!=json_object::JSONT_DICT){
                    std::cout<<"NOT DICT\n";
                    clear();
                    return false;
                }
                if(!p.parse((vigil::json_object*)(*pit))){
                    std::cout<<"p.parse\n";
                    clear();
                    return false;
                }
                push_back(p);
            }

            return true;

        }

        bool load(std::list<vigil::json_object*> *paths){
            return parse(paths);
        }

}; // class PathList

#endif // _PATH_CONFIG_H_
