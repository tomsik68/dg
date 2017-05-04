#ifndef _DG_UDG_ASSEMBLER_H_
#define _DG_UDG_ASSEMBLER_H_

#include "../ReachingDefinitions.h"
#include <set>
#include <vector>

namespace dg {
namespace analysis {
namespace rd {

/**
 * Assembles Use -> Def graph from existing Def -> Use graph
 */
class UdgAssembler
{
    /**
     * Returns true if @node accesses memory and needs to be analyzed
     * false otherwise
     */
    static inline bool isMemoryNode(const RDNode *const node) {
        std::set<RDNodeType> mem_types{RDNodeType::ALLOC, RDNodeType::DYN_ALLOC, RDNodeType::PHI, RDNodeType::STORE, RDNodeType::CALL_RETURN};
        auto it = mem_types.find(node->type);
        return (it != mem_types.end());
    }

    /**
     * Return true if update of @ds in @node is a strong update
     * false otherwise
     */
    inline bool isStrong(const RDNode *const node, const DefSite& ds) const {
        return node->getOverwrites().find(ds) != node->getOverwrites().end();
    }

    // find intersections between DefSites of @node and @curr
    // and add use->def edges as necessary
    void process(RDNode *node, RDNode *curr) {
        // at this point, @curr is reachable from @node.
        // for ALLOC node @node, add edge from @curr to @node
        // if @curr re-defines @node
        if (isMemoryNode(node)) {
            for(const DefSite& currDs : curr->getDefines()) {
                if (currDs.target == node) {
                    // TODO strong update overwrites predecessors
                    // weak update merges from predecessors
                    bool strong = isStrong(curr, currDs);
                    curr->addUse(node, strong);
                }
            }
            // for COPY, LOAD add an edge from @curr to @node
            // if @curr re-defines @node
            // FIXME edges from store nodes need to be labeled
            // by a variable(maybe use DefSite)
        }
    }

public:
    /**
     * Assembles Use -> Def graph from a Def -> Use graph
     * The resulting UD information is stored into @node.
     * @node: root of the tree where the UDG should be assembled
     * if node == nullptr, nothing happens.
     */
    void assemble(const std::vector<RDNode *>& nodes) {
        for(RDNode *node : nodes) {
            for(RDNode *curr : nodes) {
                process(node, curr);
            }
        }
    }
};

}
}
}


#endif
