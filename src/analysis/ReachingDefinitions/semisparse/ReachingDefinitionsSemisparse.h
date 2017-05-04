#ifndef _DG_REACHING_DEFINITIONS_SEMISPARSE_ANALYSIS_H_
#define _DG_REACHING_DEFINITIONS_SEMISPARSE_ANALYSIS_H_

#include "../ReachingDefinitions.h"
#include "UdgAssembler.h"
#include <algorithm>
#include <vector>

namespace dg {
namespace analysis {
namespace rd {

/**
 * Semi-sparse reaching definitions analysis
 * Uses data from previous points-to analysis stored in nodes.
 */
class ReachingDefinitionsSemisparse
{
    RDNode *root;
    unsigned int dfsnum;
    bool strong_update_unknown;
    uint32_t max_set_size;

    /**
     * Return true if this node manipulates memory somehow, false otherwise
     */
    static bool isMemAccessNode(const RDNode *node) {
        static std::set<RDNodeType> mem_access_types = { RDNodeType::ALLOC, RDNodeType::STORE, RDNodeType::DYN_ALLOC, RDNodeType::PHI };

        assert(node && "Do not have node");
        return mem_access_types.find(node->getType()) != mem_access_types.end();
    }

    /**
     * Return subset of @all_nodes which contains only nodes that satisfy ::isMemAccessNode
     */
    static std::vector<RDNode *> filterMemAccess(const std::vector<RDNode *>& all_nodes) {
        std::vector<RDNode *> result;
        std::copy_if(all_nodes.begin(), all_nodes.end(), std::back_inserter(result), isMemAccessNode);
        return result;
    }

public:
    ReachingDefinitionsSemisparse(RDNode *r,
                                bool field_insens = false,
                                uint32_t max_set_sz = ~((uint32_t)0))
    : root(r), dfsnum(0), strong_update_unknown(field_insens), max_set_size(max_set_sz)
    {
        assert(r && "Root cannot be null");
        // with max_set_size == 0 (everything is defined on unknown location)
        // we get unsound results with vararg functions and similar weird stuff
        assert(max_set_size > 0 && "The set size must be at least 1");
    }

    void getNodes(std::set<RDNode *>& cont)
    {
        assert(root && "Do not have root");

        ++dfsnum;

        ADT::QueueLIFO<RDNode *> lifo;
        lifo.push(root);
        root->dfsid = dfsnum;

        while (!lifo.empty()) {
            RDNode *cur = lifo.pop();
            cont.insert(cur);

            for (RDNode *succ : cur->successors) {
                if (succ->dfsid != dfsnum) {
                    succ->dfsid = dfsnum;
                    lifo.push(succ);
                }
            }
        }
    }


    // get nodes in BFS order and store them into
    // the container
    std::vector<RDNode *> getNodes(RDNode *start_node = nullptr,
                                   std::vector<RDNode *> *start_set = nullptr,
                                   unsigned expected_num = 0)
    {
        assert(root && "Do not have root");
        assert(!(start_set && start_node)
               && "Need either starting set or starting node, not both");

        ++dfsnum;
        ADT::QueueFIFO<RDNode *> fifo;

        if (start_set) {
            for (RDNode *s : *start_set) {
                fifo.push(s);
                s->dfsid = dfsnum;
            }
        } else {
            if (!start_node)
                start_node = root;

            fifo.push(start_node);
            start_node->dfsid = dfsnum;
        }

        std::vector<RDNode *> cont;
        if (expected_num != 0)
            cont.reserve(expected_num);

        while (!fifo.empty()) {
            RDNode *cur = fifo.pop();
            cont.push_back(cur);

            for (RDNode *succ : cur->successors) {
                if (succ->dfsid != dfsnum) {
                    succ->dfsid = dfsnum;
                    fifo.push(succ);
                }
            }
        }

        return cont;
    }


    RDNode *getRoot() const { return root; }
    void setRoot(RDNode *r) { root = r; }

    bool processNode(RDNode *n);


    void run()
    {
        assert(root && "Do not have root");

        std::vector<RDNode *> to_process = getNodes(root);
        std::vector<RDNode *> changed;

        // assemble Use-Def graph to be used with the analysis
        UdgAssembler ua;
        ua.assemble(to_process);

        // do fixpoint
        do {
            unsigned last_processed_num = to_process.size();
            changed.clear();

            for (RDNode *cur : to_process) {
                if (processNode(cur))
                    changed.push_back(cur);
            }

            if (!changed.empty()) {
                to_process.clear();
                to_process = getNodes(nullptr /* starting node */,
                                      &changed /* starting set */,
                                      last_processed_num /* expected num */);

                // since changed was not empty,
                // the to_process must not be empty too
                assert(!to_process.empty());
            }
        } while (!changed.empty());
    }

};


} // namespace rd
} // namespace analysis
} // namespace dg
#endif
