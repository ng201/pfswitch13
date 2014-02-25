#include "rregionlist.h"

#include <boost/numeric/ublas/io.hpp>
#include <boost/tokenizer.hpp>

#include "json-util.hh"


namespace pfswitch13{

    bool RRegionList::parse(std::list<vigil::json_object*> *regions){
            using namespace vigil;
            using namespace vigil::json;

            std::list<json_object*>::iterator it=regions->begin();
            for(;it!=regions->end();++it){
                RRegion rp;

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
                        json_object *atmp=get_dict_value(*it,"name");
                        if(atmp->type!=json_object::JSONT_STRING){
                            clear();
                            return false;
                        }
                        std::string name=std::string(*(char**)atmp->object);

                        atmp=get_dict_value(*it,"ratio");
                        if(atmp->type!=json_object::JSONT_STRING){
                            clear();
                            return false;
                        }
                        std::string ratio=std::string(*(char**)atmp->object);

                        boost::char_separator<char> sep(";, ");
                        boost::tokenizer<boost::char_separator<char> > tokens(ratio,sep);
                        size_t tok_num=0;
                        for(boost::tokenizer<boost::char_separator<char> >::iterator tok_iter=tokens.begin();tok_iter!=tokens.end();++tok_iter,tok_num++);

                        unsigned *r=new unsigned[tok_num];
                        tok_num=0;
                        for(boost::tokenizer<boost::char_separator<char> >::iterator tok_iter=tokens.begin();tok_iter!=tokens.end();++tok_iter,tok_num++){
                            r[tok_num]=atoi(tok_iter->c_str());
                        }
                        rp.ratios[name]=r;
                    }

                }
                else{
                    clear();
                    return false;
                }

                push_back(rp);
            }

            return true;
        }

}; // namepsace pfswitch13
