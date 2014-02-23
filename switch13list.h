#ifndef _SWITCH13LIST_H_
#define _SWITCH13LIST_H_

#include <vector>

#include "netinet++/ipaddr.hh"

#include "json-util.hh"

#include "switch13.h"
#include "switchs1.h"
#include "switchs2.h"
#include "switchs3.h"
#include "switchs4.h"

namespace pfswitch13{

    using namespace vigil;

    class Switch13List:public std::vector<Switch13*>{

        private:

            /// Makes the parameter part of the current list.
            Switch13List& consume(Switch13List &l){
                for(std::vector<Switch13*>::iterator it=l.begin();it!=l.end();++it){
                    this->push_back(*it);
                }
                l.clear();
                return *this;
            }


            bool parse_path(ipv4_addr,ipv4_addr,const std::string&,
                            vigil::json_object*,
                            PathHdr*,unsigned*,unsigned*);

            bool parse_paths(ipv4_addr,ipv4_addr,const std::string&,
                             std::list<vigil::json_object*>*);

        public:


            /// DTor. To delete the stored switches.
            ~Switch13List(){
                for(std::vector<Switch13*>::iterator it=this->begin();it!=this->end();++it){
                    delete *it;
                }
            }

            Switch13List& clear(){
                for(std::vector<Switch13*>::iterator it=this->begin();it!=this->end();++it){
                    delete *it;
                }
                std::vector<Switch13*>::clear();
                return *this;
            }

            /// Parse the json object given.
            bool parse(std::list<vigil::json_object*>*,const std::string&);

            /// Parse the json object given.
            bool load(std::list<vigil::json_object*> *connections,const std::string &as){
                return parse(connections,as);
            }

    }; // class Switch13List

};

#endif // _SWITCH13LIST_H_
