#ifndef _SWITCHS2_H_
#define _SWITCHS2_H_

#include <string>

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

    class SwitchS2:public Switch13{

        private:

            unsigned pid;
            unsigned out_port;

        public:

            SwitchS2(const vigil::datapathid dpid,const std::string &name,
                     const SwitchData &data):Switch13(dpid,name,data){}

            SwitchS2(const unsigned dpid,const std::string &name,
                     const SwitchData &data):Switch13(dpid,name,data){}

            SwitchS2(const uint64_t dpid,const std::string &name,
                     const SwitchData &data):Switch13(dpid,name,data){}

            virtual void configure(unsigned _tnum=0,enum of_tmod_cmd cmd=OFTM_ADD){
                tnum=_tnum;

                //std::cerr<<self.to_string()<<"\n";

                Actions *acts=new Actions();
                acts->CreateOutput(data.out_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,tnum,
                                           toFlowCmd(cmd), //OFPFC_ADD, // cf if-fel eldonteni
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           OFP_DEFAULT_PRIORITY, //tnum
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


    }; // class SwitchS2

}; // namespace pfswitch13

#endif // _SWITCHS2_H_
