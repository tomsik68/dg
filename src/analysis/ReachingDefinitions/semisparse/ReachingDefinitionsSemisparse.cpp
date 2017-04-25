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
    for (RDNode *pred : node->use_def)
        changed |= node->def_map.merge(&(pred->def_map),
                                       &node->overwrites, /* strong update */
                                       strong_update_unknown,
                                       max_set_size /* max size of set of reaching definition
                                                       of one definition site */,
                                       true /* merge unknown */);

    return changed;
}

} // namespace rd
} // namespace analysis
} // namespace dg

