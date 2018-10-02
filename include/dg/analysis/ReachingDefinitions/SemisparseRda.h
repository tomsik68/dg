#ifndef _DG_SEMISPARSERDA_H_
#define _DG_SEMISPARSERDA_H_

#include <vector>
#include <unordered_map>

#include "dg/analysis/ReachingDefinitions/ReachingDefinitions.h"

namespace dg {
namespace analysis {
namespace rd {

class SemisparseRda : public ReachingDefinitionsAnalysis
{
private:
    bool merge_maps(RDNode *source, RDNode *dest, DefSite& var) {
        bool changed = false;

        if (source->getType() != RDNodeType::PHI)
            changed |= dest->def_map.add(var, source);

        for (auto& pair : source->def_map) {
            const DefSite& ds = pair.first;
            auto& nodes = pair.second;

            if (ds.target == var.target) {
                for (RDNode *node : nodes) {
                    if (node->getType() != RDNodeType::PHI)
                        changed |= dest->def_map.add(ds, node);
                }
            }
        }
        return changed;
    }

    std::vector<std::unique_ptr<RDNode>> phi_nodes;

public:
    SemisparseRda(RDNode *root, ReachingDefinitionsAnalysisOptions opts)
        : ReachingDefinitionsAnalysis(root, opts.setSparse(true)) {}
    SemisparseRda(RDNode *root) : SemisparseRda(root, {}) {}

    void run() override;
};

}
}
}
#endif /* _DG_SEMISPARSERDA_H_ */
