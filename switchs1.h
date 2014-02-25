#ifndef _SWITCHS1_H_
#define _SWITCHS1_H_

#include <string>

#include "netinet++/ipaddr.hh"

//#include "openflow-default.hh"
#include "flow.hh"
#include "ofp-msg-event.hh"
#include "flowmod.hh"
#include "datapath-join.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

#include "switch13.h"

namespace pfswitch13{

    using namespace vigil;

    class SwitchS1:public Switch13{

        protected:

            ipv4_addr src_ip;  ///< The source address of the flow.
            ipv4_addr dst_ip;  ///< The destination address of the flow.

        private:

            virtual void init(){
                self.Add_Field("in_port",data.in_port); // in_port
                self.Add_Field("eth_type",0x800);       // prereq. for IP src and dst
                self.Add_Field("ipv4_src",src_ip);      // src_ip
                self.Add_Field("ipv4_dst",dst_ip);      // dst_ip
            }

        public:

            SwitchS1(const vigil::datapathid dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            SwitchS1(const unsigned dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            SwitchS1(const uint64_t dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data):Switch13(dpid,name,data),
                                            src_ip(src_ip),dst_ip(dst_ip){}

            virtual void configure(unsigned _tnum=0,enum of_tmod_cmd cmd=OFTM_ADD){
                tnum=_tnum;

                //std::cerr<<self.to_string()<<"\n";
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
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,tnum,
                                           toFlowCmd(cmd), //OFPFC_ADD, // cf
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           OFP_DEFAULT_PRIORITY,  // tnum
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&self.match);
                mod->AddInstructions(inst);
                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                //delete inst; delete acts;
            }

            virtual void switchWeights(unsigned*,enum of_tmod_cmd=OFTM_MODIFY){
                // nothing to do
            }

            virtual void statReq()const{

                struct ofl_msg_multipart_request_flow req;
                req.header.header.type=OFPT_MULTIPART_REQUEST;
                req.header.type=OFPMP_FLOW;
                req.header.flags=0;

                req.table_id=tnum;
                req.out_port=OFPP_ANY;
                req.out_group=OFPG_ANY;
                req.cookie=0;
                req.cookie_mask=0;

                req.match=const_cast<ofl_match_header*>(&self.match.header);

                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&req,0,true);

            }

    }; // class SwitchS1

}; // namespace pfswitch13

#endif // _SWITCH1_H_
