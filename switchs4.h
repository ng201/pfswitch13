#ifndef _SWITCHS4_H_
#define _SWITCHS4_H_

#include <string>

//#include "openflow-default.hh"
#include "flow.hh"
#include "ofp-msg-event.hh"
#include "flowmod.hh"
#include "datapath-join.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

#include "switch13.h"
#include "groupmod.h"

namespace pfswitch13{

    using namespace vigil;


    /// PathHdr - null teminated, i.e., the last elements pid have to be zero!
    struct PathHdr{

        public:

            unsigned pid;
            unsigned out_port;

        public:

            PathHdr():pid(0),out_port(0){}
            PathHdr(unsigned pid,unsigned out_port):pid(pid),out_port(out_port){}
    };


    class SwitchS4:public SwitchS1{

        protected:

            PathHdr *ps; ///< Path header is moved!

        public:

            SwitchS4(const vigil::datapathid dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data,
                     PathHdr **_ps):SwitchS1(dpid,name,src_ip,dst_ip,data){
                ps=*_ps;
                *_ps=0;
            }

            SwitchS4(const unsigned dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data,
                     PathHdr **_ps):SwitchS1(dpid,name,src_ip,dst_ip,data){
                ps=*_ps;
                *_ps=0;
            }

            SwitchS4(const uint64_t dpid,const std::string &name,
                     ipv4_addr src_ip,ipv4_addr dst_ip,
                     const SwitchData &data,
                     PathHdr **_ps):SwitchS1(dpid,name,src_ip,dst_ip,data){
                ps=*_ps;
                *_ps=0;
            }

            virtual ~SwitchS4(){
                delete ps;
            }
            virtual void configure(unsigned tnum=OFP_DEFAULT_PRIORITY,enum of_tmod_cmd cmd=OFTM_ADD){

                if(!ps || ps->pid==0) return;

                Flow f;
                f.Add_Field("in_port",data.in_port); // src_ip, ...
                f.Add_Field("ipv4_src",src_ip); // src_ip, ...
                f.Add_Field("ipv4_dst",dst_ip); // src_ip, ...

                Actions *acts=new Actions();
                acts->CreateGroupAction(data.in_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           toFlowCmd(cmd), //OFPFC_ADD, // cf
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           tnum,//OFP_DEFAULT_PRIORITY, // tnum
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                delete inst; delete acts;


                // multiple flows - group table
                GroupMod *gmod=new GroupMod(data.in_port,toGroupCmd(cmd) /*OFPGC_ADD*/,OFPGT_SELECT);  //ADD, MODIFY, DELETE
                std::vector<Actions*> gr_acts;
                PathHdr *p=ps;
                while(p->pid){
                    Actions *acts1=new Actions();
                    gr_acts.push_back(acts1);
                    if(data.as=="vlan_id"){
                        // VLAN
                        acts1->CreatePushAction(OFPAT_PUSH_VLAN,0x8100);
                        acts1->CreateSetField("vlan_id",&(p->pid));
                        acts1->CreateOutput(p->out_port);
                    }
                    else{
                        // MPLS
                        acts1->CreatePushAction(OFPAT_PUSH_MPLS,0x8100);
                        acts1->CreateSetField("mpls_label",&(p->pid));
                        acts1->CreateOutput(p->out_port);
                    }

                    // all buckets have equal weights - startup; 
                    // call switchWeights to alter weights
                    gmod->addBucket(1,0,0,acts1);
                    p++;
                }

                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&gmod->gr_msg,0,true);

                // delete actions
                for(std::vector<Actions*>::iterator it=gr_acts.begin();it!=gr_acts.end();++it){
                    delete *it;
                }

            }


            virtual void switchWeights(unsigned *ratios,enum of_tmod_cmd cmd=OFTM_MODIFY){
                GroupMod *gmod=new GroupMod(data.in_port,toGroupCmd(cmd) /*OFPGC_MODIFY*/,OFPGT_SELECT);  //ADD, MODIFY, DELETE
                std::vector<Actions*> gr_acts; // to store actions
                PathHdr *p=ps;                 // temp pointer to be used and aletered
                while(p->pid){
                    Actions *acts1=new Actions();
                    gr_acts.push_back(acts1);
                    if(data.as=="vlan_id"){
                        // VLAN
                        acts1->CreatePushAction(OFPAT_PUSH_VLAN,0x8100);
                        acts1->CreateSetField("vlan_id",&(p->pid));
                        acts1->CreateOutput(p->out_port);
                    }
                    else{
                        // MPLS
                        acts1->CreatePushAction(OFPAT_PUSH_MPLS,0x8100);
                        acts1->CreateSetField("mpls_label",&(p->pid));
                        acts1->CreateOutput(p->out_port);
                    }

                    gmod->addBucket(*ratios++,0,0,acts1);
                    p++;
                }

                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&gmod->gr_msg,0,true);

                // delete actions
                for(std::vector<Actions*>::iterator it=gr_acts.begin();it!=gr_acts.end();++it){
                    delete *it;
                }

            }



    }; // class SwitchS4

}; // namespace pfswitch13

#endif // _SWITCHS4_H_
