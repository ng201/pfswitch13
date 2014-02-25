#ifndef _SWITCH13_H_
#define _SWITCH13_H_

#include <string>
#include <cstdlib>

#include "netinet++/datapathid.hh"

#include "nox.hh"
#include "openflow-default.hh"
#include "flow.hh"
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


    /// \class Switch13
    /// Base class for all the switch types used in path-flow model.
    class Switch13{

        protected:

            vigil::datapathid dpid;    ///< The dpid of the switch.
            std::string name;          ///< The human readable name of the switch.
            SwitchData data;           ///< The base config data of the switch.
            unsigned tnum;             ///< Tablenum.

            vigil::Flow self;          ///< The flows matching this switch.
            void *uid;                 ///< User defined stuff.

        public:

            /// CTor.
            Switch13(const vigil::datapathid dpid,const std::string &name,
                     const SwitchData &data):dpid(dpid),name(name),data(data),tnum(0),uid(0){}

            /// CTor.
            Switch13(const unsigned dpid,const std::string &name,
                     const SwitchData &data):dpid(vigil::datapathid::from_host(dpid)),name(name),data(data),tnum(0),uid(0){}

            /// CTor.
            Switch13(const uint64_t dpid,const std::string &name,
                     const SwitchData &data):dpid(vigil::datapathid::from_host(dpid)),name(name),data(data),tnum(0),uid(0){}

            /// Virtual dTor, due to the virtual stuff.
            virtual ~Switch13(){
                free(uid);
            }

            /// Init the switch. This method will generate the match structure for the switch.
            virtual void init(){
                self.Add_Field("in_port",data.in_port);     // in_port
                self.Add_Field(data.as.c_str(),data.pid);   // vlan_vid
            }

            /// Attach an user defined piece of data to the switch.
            Switch13& attach(const void *_uid,size_t size){
                free(uid);
                uid=malloc(size);
                memcpy(uid,_uid,size);
                return *this;
            }

            /// Retrun true, if the datapath id of the switch is equal
            /// to the given datapath id.
            bool operator==(const vigil::datapathid _dpid)const{
                return dpid==_dpid;
            }

            /// Get the DPID of the switch. On the same dpid there are sitting
            /// multiple switches acting differenty/having different roles.
            vigil::datapathid getDPID()const{
                return dpid;
            }

            /// Get the user defined name of the switch.
            std::string getName()const{
                return name;
            }

            /// This method returns the user defined piece of data stored
            /// along with the switch.
            void* getUID()const{
                return uid;
            }

            /// Returns true if the match structure of the switch is
            /// the same as the given match structure.
            /// \todo string formats are compared; find a better way!
            bool same(struct ofl_match *match)const{
                return (self.to_string()==vigil::Flow(match).to_string());
            }

            virtual bool responsible(unsigned pid)const{
                return data.pid==pid;
            }

            /// Configrure the switch (i.e., its rule) on the physical switch.
            virtual void configure(unsigned=0,enum of_tmod_cmd=OFTM_ADD)=0;

            /// Change bucket weights.
            virtual void switchWeights(unsigned*,enum of_tmod_cmd=OFTM_MODIFY)=0;

            /// Send stat request message.
            virtual void statReq()const{
            /*
                struct ofl_msg_header {
                    enum ofp_type   type;   // One of the OFPT_ constants.
                };

                struct ofl_msg_multipart_request_header {
                    struct ofl_msg_header   header;     // OFPT_MULTIPART_REQUEST

                    enum ofp_multipart_types   type;    // One of the OFPMP_* constants.
                    uint16_t               flags;        // OFPSF_REQ_* flags (none yet defined). 
                };

                struct ofl_msg_multipart_request_flow{
                    struct ofl_msg_multipart_request_header   header; // OFPMP_FLOW/AGGREGATE 

                    uint8_t                  table_id;     // ID of table to read
                    uint32_t                 out_port;     // Require matching entries to include this as an output port. OFPP_ANY
                    uint32_t                 out_group;    // Require matching entries to include this as an output group. OFPG_ANY
                    uint64_t                 cookie;       // Require matching entries to contain this cookie value
                    uint64_t                 cookie_mask;  // Mask used to restrict the cookie bits that must match. A value of 0 indicates no restriction.
                    struct ofl_match_header  *match;       // Fields to match.
                };

                struct ofl_match_header {
                    uint16_t   type;             // One of OFPMT_*
                    uint16_t   length;           // Match length
                };
            */

            }

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
