#include <set>

#include "../RDMap.h"
#include "ReachingDefinitionsSemisparse.h"

namespace dg {
namespace analysis {
namespace rd {

bool ReachingDefinitionsSemisparse::processNode(RDNode *node)
{
    bool changed = false;

    // merge maps from predecessors
    for (const auto& pair : node->use_def) {
        RDNode *pred = pair.first;
        bool strong = pair.second;
        changed |= node->def_map.merge(&(pred->def_map),
                                       &node->overwrites, /* strong update */
                                       strong_update_unknown,
                                       max_set_size /* max size of set of reaching definition
                                                       of one definition site */,
                                       true /* merge unknown */);
    }

    return changed;
}

} // namespace rd
} // namespace analysis
} // namespace dg

