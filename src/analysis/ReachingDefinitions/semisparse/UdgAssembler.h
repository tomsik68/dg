#ifndef _DG_UDG_ASSEMBLER_H_
#define _DG_UDG_ASSEMBLER_H_

#include "../ReachingDefinitions.h"
#include <iostream>
#include <vector>

namespace dg {
namespace analysis {
namespace rd {

/**
 * Assembles Use -> Def graph from existing Def -> Use graph
 */
class UdgAssembler
{
    void dumpDs(const DefSite& ds) {
        std::cout << "DefSite( " << ds.target << ", " << ds.len.offset << ", " << ds.offset.offset << ")" << std::endl;
    /**
     * Returns true if @node accesses memory and needs to be analyzed
     * false otherwise
     */
    static inline bool isMemoryNode(const RDNode *const node) {
        std::set<RDNodeType> mem_types{RDNodeType::ALLOC, RDNodeType::DYN_ALLOC, RDNodeType::PHI, RDNodeType::STORE, RDNodeType::CALL_RETURN};
        auto it = mem_types.find(node->type);
        return (it != mem_types.end());
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
                    std::cout << "found overlap with alloca" << std::endl;
                    curr->addUse(node);
                }
            }
            for(const DefSite& currDs : curr->getOverwrites()) {
                if (currDs.target == node) {
                    std::cout << "found overlap with alloca" << std::endl;
                    curr->addUse(node);
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
