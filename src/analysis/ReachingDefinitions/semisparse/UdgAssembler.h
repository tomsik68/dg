#ifndef _DG_UDG_ASSEMBLER_H_
#define _DG_UDG_ASSEMBLER_H_

#include "../ReachingDefinitions.h"
#include "../../BFS.h"

namespace dg {
namespace analysis {
namespace rd {

/**
 * Assembles Use -> Def graph from existing Def -> Use graph
 */
class UdgAssembler
{

    unsigned int dfsnum;

    void process(RDNode *node) {
        assert(node && "Node cannot be null");

        // for every node defined by this node
        for (const DefSite& ds : node->getDefines()) {
            // tell the DUG child node it is defined by this node
            assert( ds.target && "DefSite must have a target" );
            ds.target->addUse(DefSite(node, ds.offset, ds.len));
        }

    }
public:
    UdgAssembler(int dfsnum) : dfsnum(dfsnum) {}

    /**
     * Assembles Use -> Def graph from a Def -> Use graph
     * The resulting UD information is stored into @node.
     * @node: root of the tree where the UDG should be assembled
     * if node == nullptr, nothing happens.
     */
    void assemble(RDNode *root) {
        assert(root && "Root cannot be null");

        ++dfsnum;

        ADT::QueueFIFO<RDNode *> fifo;
        fifo.push(root);
        root->dfsid = dfsnum;

        while(!fifo.empty()) {
            RDNode *cur = fifo.pop();
            process(cur);

            for (RDNode *succ : cur->successors) {
                if (succ->dfsid != dfsnum) {
                    succ->dfsid = dfsnum;
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
