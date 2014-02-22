#ifndef _SWITCHS1_H_
#define _SWITCHS1_H_

#include <string>

#include "netinet++/ipaddr.hh"

#include "openflow-default.hh"
#include "flow.hh"
#include "ofp-msg-event.hh"
#include "flowmod.hh"
#include "datapath-join.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

#include "Switch13.h"

namespace pfswitch13{

    class SwitchS1:public Switch13{

        protected:

            ipv4_addr src_ip;  ///< The source address of the flow.
            ipv4_addr dst_ip;  ///< The destination address of the flow.

        public:

            SwitchS(const vigil::datapathid dpid,const std::string &name,
                    ipv4_addr src_ip,ipv4_addr dst_ip,
                    const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            SwitchS(const unsgined dpid,const std::string &name,
                    ipv4_addr src_ip,ipv4_addr dst_ip,
                    const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            SwitchS(const uint64_t dpid,const std::string &name,
                    ipv4_addr src_ip,ipv4_addr dst_ip,
                    const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            virtual void configure(unsigned tnum=OFP_DEFAULT_PRIORITY,enum of_tmod_cmd cmd=OFTM_ADD){
                Flow f;
                f.Add_Field("in_port",data.in_port); // src_ip, ...
                f.Add_Field("ipv4_src",src_ip); // src_ip, ...
                f.Add_Field("ipv4_dst",dst_ip); // src_ip, ...
                Actions *acts=new Actions();
                if(data.as=="vlan_id"){
                    // VLAN
                    acts->CreatePushAction(OFPAT_PUSH_VLAN,0x8100);
                    acts->CreateSetField("vlan_id",&(data.pid));
                }
                else{
                    // MPLS
                    acts->CreatePushAction(OFPAT_PUSH_MPLS,0x8100);
                    acts->CreateSetField("mpls_label",&(data.pid));
                }
                acts->CreateOutput(data.out_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           toFlowCmd(cmd), //OFPFC_ADD, // cf
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           tnum,//OFP_DEFAULT_PRIORITY,  // tnum
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                delete inst; delete acts;
            }

            virtual void switchWeights(unsigned*,enum of_tmod_cmd=OFTM_MODIFY){
                // nothing to do
            }



    }; // class SwitchS1

}; // namespace pfswitch13

#endif // _SWITCH1_H_
