#ifndef _SWITCH13_H_
#define _SWITCH13_H_

#include <string>

#include "netinet++/datapathid.hh"

#include "nox.hh"
#include "openflow-default.hh"
#include "flow.hh"
#include "fnv_hash.hh"
#include "ofp-msg-event.hh"
#include "flowmod.hh"
#include "datapath-join.hh"

#include "../../../oflib/ofl-actions.h"
#include "../../../oflib/ofl-messages.h"

namespace pfswitch13{

    enum of_tmod_cmd{OFTM_ADD,OFTM_MODIFY,OFTM_DELETE};

    typedef uint32_t ipv4_addr;

    struct SwitchData{

        public:

            unsigned pid;      ///< The paths identifier.
            std::string as;    ///< The path's identifier is treaten as... (VLAN ID, etc.)
            unsigned in_port;  ///< In port of the packets.
            unsigned out_port; ///< Out port of the packets.

        public:

            /// Ctor;
            SwitchData(unsigned pid,const std::string &as,
                       unsigned in_port,unsigned out_port):pid(pid),as(as),in_port(in_port),out_port(out_port){}
    };


    class Switch13{

        protected:

            vigil::datapathid dpid;    ///< The dpid of the switch.
            std::string name;          ///< The human readable name of the switch.
            SwitchData data;           ///< The base config data of the switch.

        public:

            Switch13(const vigil::datapathid dpid,const std::string &name,
                     const SwitchData &data):dpid(dpid),name(name),data(data){}

            Switch13(const unsigned dpid,const std::string &name,
                     const SwitchData &data):dpid(vigil::datapathid::from_host(dpid)),name(name),data(data){}

            Switch13(const uint64_t dpid,const std::string &name,
                     const SwitchData &data):dpid(vigil::datapathid::from_host(dpid)),name(name),data(data){}

            virtual ~Switch13(){}

            virtual void configure(unsigned=OFP_DEFAULT_PRIORITY,enum of_tmod_cmd=OFTM_ADD)=0;

            virtual void switchWeights(unsigned*,enum of_tmod_cmd=OFTM_MODIFY)=0;

            static ofp_flow_mod_command toFlowCmd(enum of_tmod_cmd cmd){
                switch(cmd){
                    case OFTM_ADD:
                                       return OFPFC_ADD;
                    case OFTM_MODIFY:
                                       return OFPFC_MODIFY;
                    default:
                                       return OFPFC_DELETE;
                }
                return OFPFC_ADD;
            }

            static ofp_group_mod_command toGroupCmd(enum of_tmod_cmd cmd){
                switch(cmd){
                    case OFTM_ADD:
                                       return OFPGC_ADD;
                    case OFTM_MODIFY:
                                       return OFPGC_MODIFY;
                    default:
                                       return OFPGC_DELETE;
                }
                return OFPGC_ADD;
            }


    }; // class Switch13

}; // namespace pfswitch13

#endif // _SWITCH13_H_
