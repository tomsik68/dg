#ifndef _DG_UDG_ASSEMBLER_H_
#define _DG_UDG_ASSEMBLER_H_

#include "../ReachingDefinitions.h"
#include <iostream>

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
    void construct(RDNode *node, RDNode *curr) {
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

    // BFS search nodes reachable from @node, find definition intersection with @node, add use->def edge
    void process(RDNode *node) {
        assert(node && "Node cannot be null");

        std::cout << "RDNode type: " << node->type << std::endl;
        std::set<RDNode *> visited;
        ADT::QueueFIFO<RDNode *> fifo;
        // do NOT push node to fifo.
        // as all @node's DefSite-s overlap with themselves, that would create a Use -> Def edge to itself
        // for every node with at least one DefSite. that is something we do not need
        for(RDNode *succ : node->successors) {
            fifo.push(succ);
            visited.insert(succ);
        }
        while(!fifo.empty()) {
            RDNode *curr = fifo.pop();
            construct(node, curr);
            // continue BFS
            for(RDNode *succ : curr->successors) {
                if (visited.insert(succ).second) {
                    fifo.push(succ);
                }
            }
        }
    }
public:
    //UdgAssembler(int dfsnum) : dfsnum(dfsnum) {}

    /**
     * Assembles Use -> Def graph from a Def -> Use graph
     * The resulting UD information is stored into @node.
     * @node: root of the tree where the UDG should be assembled
     * if node == nullptr, nothing happens.
     */
    void assemble(RDNode *root) {
        assert(root && "Root cannot be null");

        std::set<RDNode *> visited;

        ADT::QueueFIFO<RDNode *> fifo;
        fifo.push(root);
        visited.insert(root);

        while(!fifo.empty()) {
            RDNode *cur = fifo.pop();
            // TODO use extremely sophisticated way of handling dfsnum
            process(cur);

            for (RDNode *succ : cur->successors) {
                if (visited.insert(succ).second) {
                    fifo.push(succ);
                }
            }
        }

    }
};

}
}
}


#endif
