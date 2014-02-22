#ifndef _SWITCHS2_H_
#define _SWITCHS2_H_

#include <string>

#include "openflow-default.hh"
#include "flow.hh"
#include "ofp-msg-event.hh"
#include "flowmod.hh"
#include "datapath-join.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

#include "switch13.h"

namespace pfswitch13{

    using namespace vigil;
    using namespace vigil::container;

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

            virtual void configure(unsigned tnum=OFP_DEFAULT_PRIORITY,enum of_tmod_cmd cmd=OFTM_ADD){
                Flow f;
                f.Add_Field("in_port",data.in_port);
                f.Add_Field(data.as.c_str(),data.pid);
                Actions *acts=new Actions();
                acts->CreateOutput(data.out_port);
                Instruction *inst=new Instruction();
                inst->CreateApply(acts);
                FlowMod *mod = new FlowMod(0x00ULL,0x00ULL,0,
                                           toFlowCmd(cmd), //OFPFC_ADD, // cf if-fel eldonteni
                                           OFP_FLOW_PERMANENT,
                                           OFP_FLOW_PERMANENT,
                                           tnum,//OFP_DEFAULT_PRIORITY, //tnum
                                           0,
                                           OFPP_ANY,OFPG_ANY,ofd_flow_mod_flags());
                mod->AddMatch(&f.match);
                mod->AddInstructions(inst);
                nox::send_openflow_msg(dpid,(struct ofl_msg_header *)&mod->fm_msg,0,true);

                delete inst; delete acts;
            }

            virtual void switchWeights(unsigned*,enum of_tmod_cmd=OFTM_MODIFY){
                // nothing to do
            }



    }; // class SwitchS2

}; // namespace pfswitch13

#endif // _SWITCHS2_H_
