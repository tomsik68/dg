#include <llvm/IR/Value.h>

#include "LLVMDependenceGraph.h"
#include "ValueFlow.h"

namespace dg {
namespace analysis {

LLVMValueFlowAnalysis::LLVMValueFlowAnalysis(LLVMDependenceGraph *dg)
    : DataFlowAnalysis<LLVMNode>(dg->getEntryBB(), DATAFLOW_INTERPROCEDURAL)
{
}

/* virtual */
bool LLVMValueFlowAnalysis::runOnNode(LLVMNode *node)
{
    const llvm::Value *
}

} // namespace analysis
} // namespace dg
