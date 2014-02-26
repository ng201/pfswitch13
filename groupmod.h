#ifndef _GROUPMOD_H_
#define _GROUPMOD_H_

#include "../oflib/ofl-messages.h"
#include "../oflib/ofl-structs.h"
#include "actions.hh"

class GroupMod{

    private:

    public:

        struct ofl_msg_group_mod gr_msg;

        GroupMod(uint16_t group_id,
                 enum ofp_group_mod_command cmd,
                 enum ofp_group_type type){
            gr_msg.header.type=OFPT_GROUP_MOD;
            gr_msg.command=cmd;     //OFPGC_* ADD, MODIFY, DELETE
            gr_msg.type=type;       //OFPGT_* ALL, SELECT, INDIRECT, FF
            gr_msg.group_id=group_id;
            gr_msg.buckets_num=0;

            gr_msg.buckets=(struct ofl_bucket**) xmalloc(sizeof(struct ofl_bucket*));
        }

        GroupMod& addBucket(uint16_t weight,
                            uint32_t watch_port,uint32_t watch_group,
                            vigil::Actions *action){
            /*
            ofl_bucket **buckets;

            struct ofl_bucket {
                uint16_t   weight;      //  Relative weight of bucket. Only defined for select groups.
                uint32_t   watch_port;  //  Port whose state affects whether this bucket is live. Only required for fast ver groups.
                uint32_t   watch_group; // Group whose state affects whether this                               bucket is live. Only required for fast                              failover groups. 
                size_t actions_num;
                struct ofl_action_header **actions;
            };
            */
            //action->acts;
            gr_msg.buckets=(struct ofl_bucket**) xrealloc(gr_msg.buckets,sizeof(struct ofl_bucket*)*(gr_msg.buckets_num+1));

            struct ofl_bucket *bucket=(struct ofl_bucket*) xmalloc(sizeof(struct ofl_bucket));
            bucket->weight=weight;
            bucket->watch_port=watch_port;
            bucket->watch_group=watch_group;
            bucket->actions_num=action->act_num;
            bucket->actions=action->acts;

            gr_msg.buckets[gr_msg.buckets_num]=bucket;
            gr_msg.buckets_num++;

            return *this;

        }

}; // class GroupMod

#endif // _GROUPMOD_H_
