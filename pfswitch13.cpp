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
#include <stdio.h>

#include <stdio.h>
#include "netinet++/ethernetaddr.hh"
#include "netinet++/ethernet.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

#include "json-util.hh"

#include "connection_config.h"
#include "path_config.h"
#include "region_config.h"

#include "groupmod.h"

using namespace vigil;
using namespace vigil::container;
using namespace vigil::json;
using namespace std;

namespace {

struct Mac_source
{
    /* Key. */
    datapathid datapath_id;     /* Switch. */
    ethernetaddr mac;           /* Source MAC. */

    /* Value. */
    mutable int port;           /* Port where packets from 'mac' were seen. */

    Mac_source() : port(-1) { }
    Mac_source(datapathid datapath_id_, ethernetaddr mac_)
        : datapath_id(datapath_id_), mac(mac_), port(-1)
        { }
};

bool operator==(const Mac_source& a, const Mac_source& b)
{
    return a.datapath_id == b.datapath_id && a.mac == b.mac;
}

bool operator!=(const Mac_source& a, const Mac_source& b) 
{
    return !(a == b);
}

struct Hash_mac_source
{
    std::size_t operator()(const Mac_source& val) const {
        uint32_t x;
        x = vigil::fnv_hash(&val.datapath_id, sizeof val.datapath_id);
        x = vigil::fnv_hash(val.mac.octet, sizeof val.mac.octet, x);
        return x;
    }
};

Vlog_module log("QPFSwitch");

class QPFSwitch:public Component{

    private:

        json_object *config;               ///< The whole cfg file in json.
        bool setup_flows;                  ///< ???
        double delay;                      ///< Polling delay.
        std::string as;                    ///< Path id is...
        ConnectionList connection_list;    ///< The list of defined connections.
        RegionList region_list;            ///< The list of defined routing regions.

        typedef hash_set<Mac_source, Hash_mac_source> Source_table;

        Source_table sources;

    public:

        /// Ctor.
        QPFSwitch(const Context* c,const json_object*):Component(c){}

        /// Configure.
        void configure(const Configuration*);

        /// Install handles.
        void install();

        /// New packet without rule.
        Disposition handle(const Event&);

        /// When router comes up, run this function.
        Disposition handle_dp_join(const Event& e);

};

void QPFSwitch::configure(const Configuration* conf){
    setup_flows=true; // default value

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

    config=load_document(input_file.c_str());
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
        json_object *d=get_dict_value(ctrl,"query");
        if(d){
            if(d->type==json_object::JSONT_FLOAT) delay=*((float*)d->object);
            else if(d->type==json_object::JSONT_INTEGER) delay=*((int*)d->object);
            VLOG_INFO(log,"Query delay set in controller config file. Delay is %f ms.",delay);
        }
        else{
            delay=100;
            VLOG_WARN(log,"Query delay not sent. Using %f ms.",delay);
        }
    }

    json_object *connections=get_dict_value(ctrl,"connections");
    if(connections && connections->type==json_object::JSONT_ARRAY){
        if(!connection_list.parse((std::list<json_object*>*)connections->object)) exit(-1);
    }
    else{
        VLOG_ERR(log,"Cannot find connections in the config file...");
        exit(-1);
    }

    json_object *regs=get_dict_value(ctrl,"regions");
    if(regs && regs->type==json_object::JSONT_ARRAY){
        if(!region_list.parse((std::list<json_object*>*)regs->object)) exit(-1);
    }
    else{
        VLOG_ERR(log,"No routing function given.");
        exit(-1);
    }

    VLOG_INFO(log,"Nunber of connections: %d.",connection_list.size());
    VLOG_INFO(log,"Nunber of routing regions: %d.",region_list.size());

}

void QPFSwitch::install(){
    register_handler(Datapath_join_event::static_get_name(),boost::bind(&QPFSwitch::handle_dp_join,this,_1));
    register_handler(Ofp_msg_event::get_name(OFPT_PACKET_IN),boost::bind(&QPFSwitch::handle,this,_1));
}

Disposition QPFSwitch::handle_dp_join(const Event& e){

    const Datapath_join_event& dpj=assert_cast<const Datapath_join_event&>(e);

    vigil::datapathid dpid=dpj.dpid;
    for(ConnectionList::iterator cit=connection_list.begin();cit!=connection_list.end();++cit){

        if(vigil::datapathid::from_host(cit->src_dpid)==dpid){
            if(cit->paths.size()==1){
                // single flow - flow table
                PathNode pn=cit->paths[0].front();

                Flow f;
                f.Add_Field("in_port",pn.in_port); // src_ip, ...
                Actions *acts=new Actions();
                acts->CreatePushAction(OFPAT_PUSH_VLAN,0x8100);
                acts->CreateSetField("vlan_id",&(pn.pid));
                acts->CreateOutput(pn.out_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           OFPFC_ADD,
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           0,//OFP_DEFAULT_PRIORITY,
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                VLOG_DBG(log,"Installing flow table to the switch on dpid= 0x%"PRIx64" to handle and pop VLAN vids\n", dpj.dpid.as_host());

            }
            else{
                PathNode pn=cit->paths[0].front();

                Flow f;
                f.Add_Field("in_port",pn.in_port); // src_ip, ...
                Actions *acts=new Actions();
                acts->CreateGroupAction(pn.in_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           OFPFC_ADD,
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           0,//OFP_DEFAULT_PRIORITY,
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                VLOG_DBG(log,"Installing GOTO Group to the switch on dpid= 0x%"PRIx64"\n", dpj.dpid.as_host());

                // multiple flows - group table
                GroupMod *gmod=new GroupMod(pn.in_port,OFPGC_MODIFY,OFPGT_SELECT);  //ADD, MODIFY, DELETE
                std::vector<Actions*> gr_acts;
                std::vector<uint16_t> vlan_vids(100);
                for(PathList::iterator it=cit->paths.begin();it!=cit->paths.end();++it){
                    PathNode pn=it->front();
                    if(vigil::datapathid::from_host(pn.dpid)!=dpid) continue; // this is a rule for other dpid

                    Actions *acts1=new Actions();
                    gr_acts.push_back(acts1);
                    vlan_vids.push_back(pn.pid);

                    acts1->CreatePushAction(OFPAT_PUSH_VLAN,0x8100);
                    acts1->CreateSetField("vlan_id",&vlan_vids.back());
                    acts1->CreateOutput(pn.out_port);

                    gmod->addBucket((pn.pid==12?33:66),0,0,acts1);
                }

                send_openflow_msg(dpid,(struct ofl_msg_header *)&gmod->gr_msg,0,true);
                VLOG_DBG(log,"Installing group table to the switch on dpid= 0x%"PRIx64"\n", dpj.dpid.as_host());

            }
        }
        else if(vigil::datapathid::from_host(cit->dst_dpid)==dpid){
            for(PathList::iterator it=cit->paths.begin();it!=cit->paths.end();++it){
                PathNode pn=it->back();
                if(vigil::datapathid::from_host(pn.dpid)!=dpid) continue; // this is a rule for other dpid

                Flow f;
                f.Add_Field("in_port",pn.in_port);
                f.Add_Field(as.c_str(),pn.pid);
                Actions *acts=new Actions();
                acts->CreatePopVlan();
                acts->CreateOutput(pn.out_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           OFPFC_ADD,
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           0,//OFP_DEFAULT_PRIORITY,
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                VLOG_DBG(log,"Installing flow table to the switch on dpid= 0x%"PRIx64" to handle and pop VLAN vids\n", dpj.dpid.as_host());
            }
        }
        else{
            for(PathList::iterator it=cit->paths.begin();it!=cit->paths.end();++it){
                for(PathPrimitive::iterator it2=it->begin();it2!=it->end();++it2){
                    PathNode pn=*it2;
                    if(vigil::datapathid::from_host(pn.dpid)!=dpid) continue; // this is a rule for other dpid

                    Flow f;
                    f.Add_Field("in_port",pn.in_port);
                    f.Add_Field(as.c_str(),pn.pid);
                    Actions *acts=new Actions();
                    acts->CreateOutput(pn.out_port);
                    Instruction *inst=new Instruction();
                    inst->CreateApply(acts);
                    FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                               OFPFC_ADD,
                                               OFP_FLOW_PERMANENT,
                                               OFP_FLOW_PERMANENT,
                                               0,//OFP_DEFAULT_PRIORITY,
                                               0,
                                               OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                    mod->AddMatch(&f.match);
                    mod->AddInstructions(inst);
                    send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                    VLOG_DBG(log,"Installing flow table to the switch on dpid= 0x%"PRIx64" to handle VLAN vids\n", dpj.dpid.as_host());
                }
            }
        }

    }

    return CONTINUE;



    /* The behavior on a flow miss is to drop packets
       so we need to install a default flow */
//    VLOG_DBG(log,"Installing default flows to the switch on dpid= 0x%"PRIx64"\n", dpj.dpid.as_host());
//
//    return CONTINUE;

}

Disposition QPFSwitch::handle(const Event& e){
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
    flow->get_Field<uint32_t>("in_port", &in_port);        
	
   
    /* Learn the source. */
    uint8_t eth_src[6];
    flow->get_Field("eth_src", eth_src);
    ethernetaddr dl_src(eth_src);
    if (!dl_src.is_multicast()) {
        Mac_source src(pi.dpid, dl_src);
        Source_table::iterator i = sources.insert(src).first;

        if (i->port != in_port) {
            i->port = in_port;
            VLOG_DBG(log, "learned that "EA_FMT" is on datapath %s port %d",
                     EA_ARGS(&dl_src), pi.dpid.string().c_str(),
                     (int) in_port);
        }
    } else {
        VLOG_DBG(log, "multicast packet source "EA_FMT, EA_ARGS(&dl_src));
    }

    /* Figure out the destination. */
    int out_port = -1;        /* Flood by default. */
    uint8_t eth_dst[6];
    flow->get_Field("eth_dst", eth_dst);
    ethernetaddr dl_dst(eth_dst);
    if (!dl_dst.is_multicast()) {
        Mac_source dst(pi.dpid, dl_dst);
	Source_table::iterator i(sources.find(dst));
        if (i != sources.end()) {
            out_port = i->port;
        }
    }
		
    /* Set up a flow if the output port is known. */
    if (setup_flows && out_port != -1) {

        
	Flow  f;
	f.Add_Field("in_port", in_port);
	f.Add_Field("eth_src", eth_src);
	f.Add_Field("eth_dst",eth_dst);
	Actions *acts = new Actions();
    acts->CreateOutput(out_port);
    Instruction *inst =  new Instruction();
    inst->CreateApply(acts);
    FlowMod *mod = new FlowMod(0x00ULL,0x00ULL, 0,OFPFC_ADD, 1, OFP_FLOW_PERMANENT, OFP_DEFAULT_PRIORITY,in->buffer_id, 
                                    OFPP_ANY, OFPG_ANY, ofd_flow_mod_flags());
    mod->AddMatch(&f.match);
	mod->AddInstructions(inst);
    send_openflow_msg(pi.dpid, (struct ofl_msg_header *)&mod->fm_msg, 0/*xid*/, true/*block*/);
    }
    /* Send out packet if necessary. */
    if (!setup_flows || out_port == -1 || in->buffer_id == UINT32_MAX) {
        if (in->buffer_id == UINT32_MAX) {
            if (in->total_len != in->data_length) {
                /* Control path didn't buffer the packet and didn't send us
                 * the whole thing--what gives? */
                VLOG_DBG(log, "total_len=%"PRIu16" data_len=%zu\n",
                        in->total_len, in->data_length);
                return CONTINUE;
            }
            send_openflow_pkt(pi.dpid, Nonowning_buffer(in->data, in->data_length), in_port, out_port == -1 ? OFPP_FLOOD : out_port, true/*block*/);
        } else {
            send_openflow_pkt(pi.dpid, in->buffer_id, in_port, out_port == -1 ? OFPP_FLOOD : out_port, true/*block*/);
        }
    }
    return CONTINUE;
}

REGISTER_COMPONENT(container::Simple_component_factory<QPFSwitch>, QPFSwitch);

} // unnamed namespacennamed namespace
