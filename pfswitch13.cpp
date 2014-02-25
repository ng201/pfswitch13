#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_array.hpp>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <stdint.h>

#include "openflow-default.hh"
#include "assert.hh"
#include "component.hh"
#include "flow.hh"
#include "fnv_hash.hh"
#include "hash_set.hh"
#include "ofp-msg-event.hh"
#include "vlog.hh"
#include "flowmod.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include <stdio.h>

#include <cstdio>
#include <cmath>
#include "netinet++/ethernetaddr.hh"
#include "netinet++/ethernet.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"


#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "json-util.hh"

#include "switch13.h"
#include "switchs1.h"
#include "switchs2.h"
#include "switchs3.h"
#include "switchs4.h"
#include "switch13list.h"

#include "groupmod.h"

#include "rregionlist.h"

using namespace vigil;
using namespace vigil::container;
using namespace vigil::json;
using namespace std;

namespace{

Vlog_module log("PFSwitch13");

/// \class PFSwitch13
class PFSwitch13:public Component{

    private:

        std::string as;                    ///< Path id is...
        double delay;                      ///< Polling delay.
        timeval timeout;                   ///< Polling delat in timeval.

        pfswitch13::Switch13List sw;

        json_object *config;               ///< The whole cfg file in json.

        unsigned active_region;                        ///< The id of the active region.
        pfswitch13::RRegionList regions;               ///< The roouting regions and their splitting ratios
        boost::numeric::ublas::vector<double> demand;  ///< The current demand vector

        std::vector<bool> visible;                     ///< Active switches.
        std::vector<bool> invalid;                     ///< The invalid switches, i.e., switches beeing in wrong routing region.

    public:

        /// Ctor.
        PFSwitch13(const Context* c,const json_object*):Component(c){}

        /// Configure.
        void configure(const Configuration*);

        /// Install handles.
        void install();

        void swapRRegion(bool=false);

        /// New packet without rule.
        Disposition handle(const Event&);

        /// When router comes up, run this function.
        Disposition handle_dp_join(const Event& e);

        /// When router switches off.
        Disposition handle_dp_leave(const Event& e);

        /// When router comes up, run this function.
        void handle_timeout();

        /// When router comes up, run this function.
        Disposition handle_stat(const Event& e);


};

void PFSwitch13::configure(const Configuration* conf){

    //Get commandline arguments
    std::string input_file;
    const hash_map<string, string> argmap=conf->get_arguments_list();
    hash_map<string, string>::const_iterator i;

    i=argmap.find("cfg");
    if(i!=argmap.end()) input_file=i->second;
    else exit(-1);
    bool delay_set=false;
    i=argmap.find("delay");
    if(i!=argmap.end()){ 
        delay=atof(i->second.c_str()); 
        delay_set=true; 
        VLOG_INFO(log,"Query delay set on command line. Delay is %f ms.",delay);
    }

    json_object *config=load_document(input_file.c_str());
    if(!config){
        VLOG_ERR(log,"Config file error.");
        exit(-1);
    }
    json_object *ctrl=get_dict_value(config,"controller");
    if(!ctrl || ctrl->type==json_object::JSONT_NULL){
        VLOG_ERR(log,"Config file error. Cannot understand controller\'s description.");
        exit(-1);
    }

    if(!get_dict_value(ctrl,"as") || get_dict_value(ctrl,"as")->type!=json_object::JSONT_STRING){
        VLOG_ERR(log,"Config file error. As wasn\'t specified.");
        exit(-1);
    }
    as=*(char**)get_dict_value(ctrl,"as")->object;

    if(!get_dict_value(ctrl,"model") || get_dict_value(ctrl,"model")->type!=json_object::JSONT_STRING){
        VLOG_ERR(log,"Config file error. Model wasn\'t specified.");
        exit(-1);
    }
    if(strcmp(*(char**)get_dict_value(ctrl,"model")->object,"pathflow")){
        VLOG_ERR(log,"Config file error. Controller speak \'pathflow\' config speaks \'arcflow\'.",(char*)get_dict_value(ctrl,"model")->object);
        exit(-1);
    }
    if(!delay_set){
        json_object *d=get_dict_value(ctrl,"delay");
        if(d){
            if(d->type==json_object::JSONT_FLOAT) delay=*((float*)d->object);
            else if(d->type==json_object::JSONT_INTEGER) delay=*((int*)d->object);
            VLOG_INFO(log,"Query delay set in controller config file. Delay is %f s.",delay);
        }
        else{
            delay=100;
            VLOG_WARN(log,"Query delay not sent. Using %f s.",delay);
        }
    }

    timeout.tv_sec=(unsigned)floor(delay);
    timeout.tv_usec=(unsigned)(1000*(delay-floor(delay)));


    json_object *connections=get_dict_value(ctrl,"connections");
    if(connections && connections->type==json_object::JSONT_ARRAY){
        if(!sw.parse((std::list<json_object*>*)connections->object,as)) exit(-1);
    }
    else{
        VLOG_ERR(log,"Cannot find connections in the config file...");
        exit(-1);
    }
    VLOG_INFO(log,"Will install %d switches...",sw.size());
    demand.resize(sw.size());

    size_t serial=0;
    for(pfswitch13::Switch13List::iterator it=sw.begin();it!=sw.end();++it,serial++){
        (*it)->init();
        (*it)->attach((void*)&serial,sizeof(size_t)); // store index in/together with the switch
    }

    json_object *regs=get_dict_value(ctrl,"regions");
    if(regs && regs->type==json_object::JSONT_ARRAY){
        if(!regions.parse((std::list<json_object*>*)regs->object)) exit(-1);
    }
    else{
        VLOG_ERR(log,"No routing function given.");
        exit(-1);
    }
    VLOG_INFO(log,"Nunber of routing regions: %d.",regions.size());

    invalid.resize(sw.size(),true);  // each one is invalid
    visible.resize(sw.size(),false); // there aren't connected switches

}

void PFSwitch13::install(){
    register_handler(Datapath_join_event::static_get_name(),boost::bind(&PFSwitch13::handle_dp_join,this,_1));
    register_handler(Datapath_leave_event::static_get_name(),boost::bind(&PFSwitch13::handle_dp_leave,this,_1));

    register_handler(Ofp_msg_event::get_name(OFPT_PACKET_IN),boost::bind(&PFSwitch13::handle,this,_1));

    register_handler(Ofp_msg_event::get_stats_name(OFPMP_FLOW),boost::bind(&PFSwitch13::handle_stat,this,_1));
    post(boost::bind(&PFSwitch13::handle_timeout,this),timeout);
}

void PFSwitch13::swapRRegion(bool anyway){

    for(pfswitch13::RRegionList::const_iterator it=regions.begin();it!=regions.end();++it){
        if(it->is_active(demand)){
            VLOG_INFO(log,"The rid of the currently active regions is %d.",regions.size());

            if(anyway || it->rid!=active_region){
                for(pfswitch13::Switch13List::iterator it3=sw.begin();it3!=sw.end();++it3){
                    if(visible[*(size_t*)((*it3)->getUID())] &&
                       dynamic_cast<pfswitch13::SwitchS4*>(*it3)){
                        std::map<std::string,unsigned*> r=it->ratios;
                        for(std::map<std::string,unsigned*>::iterator it2=r.begin();it2!=r.end();++it2){
                            if(it2->first==(*it3)->getName()){
                                (*it3)->switchWeights(it2->second);
                                invalid[*(size_t*)((*it3)->getUID())]=false;
                            }
                        }
                    }
                }
            }
            else{
                for(pfswitch13::Switch13List::iterator it3=sw.begin();it3!=sw.end();++it3){
                    if(visible[*(size_t*)((*it3)->getUID())] &&
                       dynamic_cast<pfswitch13::SwitchS4*>(*it3) &&
                       invalid[*(size_t*)((*it3)->getUID())]){
                        std::map<std::string,unsigned*> r=it->ratios;
                        for(std::map<std::string,unsigned*>::iterator it2=r.begin();it2!=r.end();++it2){
                            if(it2->first==(*it3)->getName()){
                                (*it3)->switchWeights(it2->second);
                                invalid[*(size_t*)((*it3)->getUID())]=false;
                            }
                        }
                    }
                }

            }

            active_region=it->rid;
            break;
        }
    }

}

Disposition PFSwitch13::handle_dp_leave(const Event& e){
    const Datapath_leave_event& dpl=assert_cast<const Datapath_leave_event&>(e);

    vigil::datapathid dpid=dpl.datapath_id;

    for(pfswitch13::Switch13List::iterator it=sw.begin();it!=sw.end();++it){
        if(**it==dpid){
            demand(*(size_t*)((*it)->getUID()))=0;
            visible[*(size_t*)((*it)->getUID())]=false;
            //invalid[*(size_t*)((*it)->getUID())]=true;
        }
    }

    VLOG_DBG(log,"Datapath leave from switch on dpid=0x%"PRIx64".\n",dpl.datapath_id.as_host());

    return CONTINUE;
}

Disposition PFSwitch13::handle_dp_join(const Event& e){

    const Datapath_join_event& dpj=assert_cast<const Datapath_join_event&>(e);

    vigil::datapathid dpid=dpj.dpid;

    VLOG_DBG(log,"Datapath join from switch on dpid=0x%"PRIx64".\n",dpj.dpid.as_host());

    for(pfswitch13::Switch13List::iterator it=sw.begin();it!=sw.end();++it){
        if(**it==dpid){
            VLOG_DBG(log,"Installing table(s) to the switch on dpid=0x%"PRIx64".\n",dpj.dpid.as_host());
            (*it)->configure();
            demand(*(size_t*)((*it)->getUID()))=0;
            visible[*(size_t*)((*it)->getUID())]=true;
            invalid[*(size_t*)((*it)->getUID())]=true;
            VLOG_INFO(log,"Table entries installed successfully.");
        }
    }

    swapRRegion();

    return CONTINUE;
}

void PFSwitch13::handle_timeout(){
    post(boost::bind(&PFSwitch13::handle_timeout,this),timeout);

    for(pfswitch13::Switch13List::iterator it=sw.begin();it!=sw.end();++it){
        (*it)->statReq();
    }
    VLOG_INFO(log,"Stat requests sent successfully.");
}

Disposition PFSwitch13::handle_stat(const Event& e){

    const Ofp_msg_event& pi = assert_cast<const Ofp_msg_event&>(e);

    datapathid dpid=pi.dpid; // the datapath id
    VLOG_INFO(log,"readind flow table stats on dpid=0x%"PRIx64".\n",pi.dpid.as_host());

    struct ofl_msg_multipart_reply_flow *in=(struct ofl_msg_multipart_reply_flow*)**pi.msg;

    if(in->stats_num!=1){
        VLOG_DBG(log,"More than one entry for switch on dpid=0x%"PRIx64".\n",pi.dpid.as_host());
        return CONTINUE;
    }
    Flow t=Flow((ofl_match*)in->stats[0]->match);
    VLOG_INFO(log,"oxm-match: %s",t.to_string().c_str());

    for(pfswitch13::Switch13List::iterator it=sw.begin();it!=sw.end();++it){
        if(**it==dpid){
            if((*it)->same((ofl_match*)in->stats[0]->match)){
                uint64_t byte_count=in->stats[0]->byte_count;

                demand(*(size_t*)((*it)->getUID()))=byte_count/delay-demand(*(size_t*)((*it)->getUID()));

                std::cerr<<"Demand: "<<demand(*(size_t*)((*it)->getUID()))<<"\n";

            }
        }
    }

    swapRRegion();

    /*
     struct ofl_msg_multipart_reply_header{
         struct ofl_msg_header      header;  // OFPT_MULTIPART_REPLY
         enum ofp_multipart_types   type;    // One of the OFPMP_* constants.
        uint16_t                    flags;
     };

     struct ofl_msg_multipart_reply_flow{
         struct ofl_msg_multipart_reply_header   header; // OFPMP_FLOW
         size_t                                  stats_num;
         struct ofl_flow_stats                   **stats;
     };

     struct ofl_flow_stats{
         uint8_t                         table_id;      // ID of table flow came from.
         uint32_t                        duration_sec;  // Time flow has been alive in secs.
         uint32_t                        duration_nsec; // Time flow has been alive in nsecs beyond duration_sec.
         uint16_t                        priority;      // Priority of the entry. Only meaningful when this is not an exact-match entry.
         uint16_t                        idle_timeout;  // Number of seconds idle before expiration.
         uint16_t                        hard_timeout;  // Number of seconds before expiration.
         uint64_t                        cookie;        // Opaque controller-issued identifier.
         uint64_t                        packet_count;  // Number of packets in flow.
         uint64_t                        byte_count;    // Number of bytes in flow.
         struct ofl_match_header        *match;         // Description of fields.
         size_t                          instructions_num;
         struct ofl_instruction_header **instructions;  // Instruction set.
     };
    */

    return CONTINUE;
}

Disposition PFSwitch13::handle(const Event& e){
    const Ofp_msg_event& pi = assert_cast<const Ofp_msg_event&>(e);

    struct ofl_msg_packet_in *in = (struct ofl_msg_packet_in *)**pi.msg;
    Flow *flow = new Flow((struct ofl_match*) in->match);

    /* drop all LLDP packets */
    uint16_t dl_type;
    flow->get_Field<uint16_t>("eth_type",&dl_type);
    if (dl_type == ethernet::LLDP){
        return CONTINUE;
    }

    uint32_t in_port;
    flow->get_Field<uint32_t>("in_port",&in_port);

    /* Figure out the destination. */
    int out_port = -1;        /* Flood by default. */
    uint8_t eth_dst[6];

    /* Send out packet if necessary. */
    if (in->buffer_id == UINT32_MAX) {
        if (in->total_len != in->data_length) {
            /* Control path didn't buffer the packet and didn't send us
             * the whole thing--what gives? */
            VLOG_DBG(log, "total_len=%"PRIu16" data_len=%zu\n",
                    in->total_len, in->data_length);
            return CONTINUE;
        }
        send_openflow_pkt(pi.dpid, Nonowning_buffer(in->data, in->data_length), in_port, out_port == -1 ? OFPP_FLOOD : out_port, true/*block*/);
    }
    else {
        send_openflow_pkt(pi.dpid, in->buffer_id, in_port, out_port == -1 ? OFPP_FLOOD : out_port, true/*block*/);
    }
    return CONTINUE;
}

REGISTER_COMPONENT(container::Simple_component_factory<PFSwitch13>, PFSwitch13);

} // unnamed namespace
