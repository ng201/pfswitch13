#ifndef _RREGIONLIST_H_
#define _RREGIONLIST_H_

#include <sstream>
#include <vector>
#include <map>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include "json_object.hh"

namespace pfswitch13{

    struct RRegion{

        public:

            int rid; ///< The id of the region
            boost::numeric::ublas::matrix<double> lhs;
            boost::numeric::ublas::vector<double> rhs;

            // akciok listaja
            std::map<std::string,unsigned*> ratios;

        public:

            ~RRegion(){
                for(std::map<std::string,unsigned*>::iterator it=ratios.begin();it!=ratios.end();++it)
                    delete it->second;
            }

            bool is_active(boost::numeric::ublas::vector<double> &v)const{
                boost::numeric::ublas::vector<double> tmp;
                tmp=(boost::numeric::ublas::prod(lhs,v)-rhs);
                for(boost::numeric::ublas::vector<double>::iterator it=tmp.begin();it!=tmp.end();++it)
                    if(*it>0) return false;
                return true;
            }

    }; // class RRegion


class RRegionList:public std::vector<RRegion>{

    public:

        bool parse(std::list<vigil::json_object*>*);

        bool load(std::list<vigil::json_object*> *regions){
            return parse(regions);
        }

};

}; // namespace pfswitch13

#endif // _RREGIONLIST_H_
