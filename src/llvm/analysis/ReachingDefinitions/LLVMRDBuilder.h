#ifndef _LLVM_DG_RD_BUILDER_H_
#define _LLVM_DG_RD_BUILDER_H_

#include <unordered_map>
#include <memory>

#include <llvm/Support/raw_os_ostream.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

#include "llvm/MemAllocationFuncs.h"
#include "analysis/ReachingDefinitions/ReachingDefinitions.h"
#include "llvm/analysis/PointsTo/PointsTo.h"

namespace dg {
namespace analysis {
namespace rd {

class LLVMRDBuilder
{
protected:
    const llvm::Module *M;
    const llvm::DataLayout *DL;
    bool assume_pure_functions;

    // points-to information
    dg::LLVMPointerAnalysis *PTA;

    // map of all nodes we created - use to look up operands
    std::unordered_map<const llvm::Value *, RDNode *> nodes_map;

    // mapping of llvm nodes to relevant reaching definitions nodes
    // (this is a super-set of nodes_map)
    // we could keep just one map of these two and don't duplicate
    // the information, but this way it is more bug-proof
    std::unordered_map<const llvm::Value *, RDNode *> mapping;

public:
    LLVMRDBuilder(const llvm::Module *m,
            dg::LLVMPointerAnalysis *p,
            bool pure_funs = false)
        : M(m), DL(new llvm::DataLayout(m)),
        assume_pure_functions(pure_funs), PTA(p) {}

    virtual ~LLVMRDBuilder() {
        delete DL;

        // delete nodes
        for (auto& it : nodes_map) {
            assert(it.first && "Have a nullptr node mapping");
            delete it.second;
        }
    }

    virtual RDNode *build() = 0;

    // let the user get the nodes map, so that we can
    // map the points-to informatio back to LLVM nodes
    const std::unordered_map<const llvm::Value *, RDNode *>&
        getNodesMap() const { return nodes_map; }
    const std::unordered_map<const llvm::Value *, RDNode *>&
        getMapping() const { return mapping; }

    RDNode *getMapping(const llvm::Value *val)
    {
        auto it = mapping.find(val);
        if (it == mapping.end())
            return nullptr;

        return it->second;
    }

    RDNode *getNode(const llvm::Value *val)
    {
        auto it = nodes_map.find(val);
        if (it == nodes_map.end())
            return nullptr;

        return it->second;
    }

    RDNode *getOperand(const llvm::Value *val);
    RDNode *createNode(const llvm::Instruction& Inst);
};

} /* rd */ 
} /* analysis */ 
} /* dg */ 

#endif /* end of include guard: _LLVM_DG_RD_BUILDER_H */
