#ifndef _LLVM_DG_RD_H_
#define _LLVM_DG_RD_H_

#include <unordered_map>
#include <memory>

#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

#include "llvm/MemAllocationFuncs.h"
#include "analysis/ReachingDefinitions/ReachingDefinitions.h"
#include "llvm/analysis/PointsTo/PointsTo.h"
#include "llvm/analysis/ReachingDefinitions/LLVMRDBuilder.h"
#include "llvm/analysis/ReachingDefinitions/dense/LLVMRDBuilderDense.h"

namespace dg {
namespace analysis {
namespace rd {

class LLVMReachingDefinitions
{
    std::unique_ptr<LLVMRDBuilder> builder;
    std::unique_ptr<ReachingDefinitionsAnalysis> RDA;
    RDNode *root;
    bool strong_update_unknown;
    uint32_t max_set_size;

public:
    LLVMReachingDefinitions(const llvm::Module *m,
                            dg::LLVMPointerAnalysis *pta,
                            bool strong_updt_unknown = false,
                            bool pure_funs = false,
                            uint32_t max_set_sz = ~((uint32_t) 0))
        : builder(std::unique_ptr<LLVMRDBuilder>(new LLVMRDBuilderDense(m, pta, pure_funs))),
          strong_update_unknown(strong_updt_unknown), max_set_size(max_set_sz) {}

    void run()
    {
        root = builder->build();
        RDA = std::unique_ptr<ReachingDefinitionsAnalysis>(
            new ReachingDefinitionsAnalysis(root, strong_update_unknown, max_set_size)
            );
        RDA->run();
    }

    RDNode *getNode(const llvm::Value *val)
    {
        return builder->getNode(val);
    }

    // let the user get the nodes map, so that we can
    // map the points-to informatio back to LLVM nodes
    const std::unordered_map<const llvm::Value *, RDNode *>&
                                getNodesMap() const
    { return builder->getNodesMap(); }

    const std::unordered_map<const llvm::Value *, RDNode *>&
                                getMapping() const
    { return builder->getMapping(); }

    RDNode *getMapping(const llvm::Value *val)
    {
        return builder->getMapping(val);
    }

    void getNodes(std::set<RDNode *>& cont)
    {
        assert(RDA);
        // FIXME: this is insane, we should have this method defined here
        // not in RDA
        RDA->getNodes(cont);
    }

    const RDMap& getReachingDefinitions(RDNode *n) const { return n->getReachingDefinitions(); }
    RDMap& getReachingDefinitions(RDNode *n) { return n->getReachingDefinitions(); }
    size_t getReachingDefinitions(RDNode *n, const Offset& off,
                                  const Offset& len, std::set<RDNode *>& ret)
    {
        return n->getReachingDefinitions(n, off, len, ret);
    }
};


} // namespace rd
} // namespace dg
} // namespace analysis

#endif
