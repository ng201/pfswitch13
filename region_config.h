#ifndef _REGION_CONFIG_H_
#define _REGION_CONFIG_H_

#include <sstream>
#include <vector>
#include <map>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "vlog.hh"

#include "json_object.hh"
#include "json-util.hh"

//extern vigil::Vlog_module log;

struct RegionPrimitive{

    public:

        int rid; ///< The id of the region
        boost::numeric::ublas::matrix<double> lhs;
        boost::numeric::ublas::vector<double> rhs;

        // akciok listaja
        std::map<unsigned,double> ratios;

    public:

        bool valid(boost::numeric::ublas::vector<double> &v){
            //if(boost::numeric::ublas::prod(lhs,v)<=rhs) return true;
            return false;
        }
}; // class RegionPrmitive


class RegionList:public std::vector<RegionPrimitive>{

    public:

        bool parse(std::list<vigil::json_object*> *regions){
            using namespace vigil;
            using namespace vigil::json;

            std::list<json_object*>::iterator it=regions->begin();
            for(;it!=regions->end();++it){
                RegionPrimitive rp;

                vigil::json_object* tmp=get_dict_value(*it,"rid");
                if(tmp->type==json_object::JSONT_INTEGER){
                    rp.rid=*(int*)tmp->object;
                }
                else{
                    //VLOG_ERR(log,"Region error: cannot read rid.");
                    clear();
                    return false;
                }

                tmp=get_dict_value(*it,"lhs");
                if(tmp->type==json_object::JSONT_STRING){
                    std::istringstream iss(*(char**)tmp->object);
                    iss>>rp.lhs;
                }
                else{
                    //VLOG_ERR(log,"Region error: cannot read lhs for region with rid=%d.",rp.rid);
                    clear();
                    return false;
                }

                tmp=get_dict_value(*it,"rhs");
                if(tmp->type==json_object::JSONT_STRING){
                    std::istringstream iss(*(char**)tmp->object);
                    iss>>rp.rhs;
                }
                else{
                    //VLOG_ERR(log,"Region error: cannot read rhs for region with rid=%d.",rp.rid);
                    clear();
                    return false;
                }

                tmp=get_dict_value(*it,"actions");
                if(tmp->type==json_object::JSONT_ARRAY){

                    std::list<json_object*> *act=(std::list<json_object*> *)tmp->object;
                    for(std::list<json_object*>::iterator it=act->begin();it!=act->end();++it){
                        json_object *atmp=get_dict_value(*it,"pid");
                        if(atmp->type!=json_object::JSONT_INTEGER){
                            clear();
                            return false;
                        }
                        int pid=*(int*)atmp->object;
                        float ratio;
                        atmp=get_dict_value(*it,"ratio");
                        if(atmp->type==json_object::JSONT_FLOAT){
                            ratio=*(float*)atmp->object;
                        }
                        else if(atmp->type==json_object::JSONT_INTEGER){
                            ratio=*(int*)atmp->object;
                        }
                        else{
                            clear();
                            return false;
                        }

                        rp.ratios[pid]=ratio;

                    }

                }
                else{
                    //VLOG_ERR(log,"Region error: cannot read rhs for region with rid=%d.",rp.rid);
                    clear();
                    return false;
                }


                push_back(rp);
            }

            return true;
        }

        bool load(std::list<vigil::json_object*> *regions){
            return parse(regions);
        }

};

#endif // _REGION_CONFIG_H_
