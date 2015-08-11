#ifndef _LLVM_VALUE_FLOW_H_
#define _LLVM_VALUE_FLOW_H_

#include "analysis/DataFlowAnalysis.h"

namespace dg {

class LLVMNode;

namespace analysis {

class LLVMValueFlowAnalysis : public DataFlowAnalysis<LLVMNode>
{
public:
    LLVMValueFlowAnalysis(LLVMDependenceGraph *dg);

    /* virtual */
    bool runOnNode(LLVMNode *node);
};

} // namespace analysis
} // namespace dg

#endif //  _LLVM_VALUE_FLOW_H_
