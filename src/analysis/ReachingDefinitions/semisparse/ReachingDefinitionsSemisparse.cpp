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
    for (const DefSite& ds : node->use_def)
        changed |= node->def_map.merge(&(ds.target->def_map),
                                       &node->ud_overwrites, /* strong update */
                                       strong_update_unknown,
                                       max_set_size /* max size of set of reaching definition
                                                       of one definition site */,
                                       false /* merge unknown */);

    return changed;
}

} // namespace rd
} // namespace analysis
} // namespace dg

