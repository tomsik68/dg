#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>

#include "LLVMDependenceGraph.h"
#include "ValueFlow.h"

using namespace llvm;

namespace dg {
namespace analysis {

LLVMValueFlowAnalysis::LLVMValueFlowAnalysis(LLVMDependenceGraph *dg)
    : DataFlowAnalysis<LLVMNode>(dg->getEntryBB(), DATAFLOW_INTERPROCEDURAL)
{
}

static LLVMNode *getUndefNode(Type *Ty)
{
    static std::map<Type *, LLVMNode *> undefNodes;
    LLVMNode *&n = undefNodes[Ty];
    if (!n)
        n = new LLVMNode(UndefValue::get(Ty));

    return n;
}

static bool handleAllocaInst(const AllocaInst *Inst, LLVMNode *node)
{
    MemoryObj *& mo = node->getMemoryObj();
    if (!mo) {
        mo = new MemoryObj(node);
        node->addPointsTo(mo);
        return true;
    }

    return false;
}

static bool handleStoreInst(const StoreInst *Inst, LLVMNode *node)
{
    bool changed = false;
    const Value *valOp = Inst->getValueOperand();
    const Value *ptrOp = Inst->getPointerOperand();

    LLVMNode *ptrNode = node->getOperand(0);
    LLVMNode *valNode = node->getOperand(1);
    assert(ptrNode && "No node for pointer argument");

    bool isptr = valOp->getType()->isPointerTy();

    if (!valNode) {
        if (isa<Argument>(valOp)) {
            LLVMDependenceGraph *dg = node->getDG();
            LLVMDGParameters *params = dg->getParameters();
            LLVMDGParameter *p = params->find(valOp);
            // we're storing value of parameter somewhere,
            // so it is input parameter
            if (p)
                valNode = p->in;
        }

        if (!valNode) {
            // XXX for now
            valNode = new LLVMNode(valOp);
            node->getDG()->addNode(valNode);
            valNode->addValue(valNode);
        }

        node->setOperand(valNode, 1);
    }

    // iterate over points to locations of pointer node
    for (auto ptr : ptrNode->getPointsTo()) {
        if(isptr) {
            // if we store pointer to unknown location,
            // then this pointer can point anywhere
            if (valNode->hasUnknownValue())
                return ptrNode->setUnknownValue();

            // if we're storing a pointer, make obj[offset]
            // points to the same locations as the valNode
            for (auto valptr : valNode->getPointsTo())
                changed |= ptr.obj->pointsTo[ptr.offset].insert(valptr).second;
        } else {
            // if it is just a value, make obj[offset] has the valNode value
            const ValuesSetT& vals = valNode->getValues();
            if (valNode->hasUnknownValue())
                changed |= ptr.obj->values[ptr.offset].insert(getUndefNode(valOp->getType())).second;
            else
                for (auto v : vals)
                    changed |= ptr.obj->values[ptr.offset].insert(v).second;
        }
    }

    return changed;
}

static bool handleLoadInst(const LoadInst *Inst, LLVMNode *node)
{
    bool changed = false;
    LLVMNode *ptrNode = node->getOperand(0);
    assert(ptrNode && "No pointer operand for LoadInst");

    bool isptr = Inst->getType()->isPointerTy();

    // dereferencing unknown pointer results in unknown value
    if (ptrNode->hasUnknownValue())
        return node->setUnknownValue();

    // get values that are referenced by pointer and
    // store them as values of this load (or as pointsTo
    // if the load it pointer)
    for (auto ptr : ptrNode->getPointsTo()) {
        if (isptr) {
            // load of pointer makes this node point
            // to the same values as ptrNode
            for (auto memptr : ptr.obj->pointsTo[ptr.offset])
                changed |= node->addPointsTo(memptr);

            if (ptr.obj->pointsTo.count(UNKNOWN_OFFSET) != 0)
                for (auto memptr : ptr.obj->pointsTo[UNKNOWN_OFFSET])
                    changed |= node->addPointsTo(memptr);
        } else {
            const ValuesSetT& memvals = ptr.obj->values[ptr.offset];
            if (memvals.empty()) // unkown value
                errs() << "WARN: LoadInst unknown value operand: " << *Inst << "\n";
                //changed |= node->addValue(getUndefNode(Inst->getType()));
            else {
                for (auto memval : ptr.obj->values[ptr.offset])
                    changed |= node->addValue(memval);

                // if there are values on UNKNOWN_OFFSET, we must add them too
                if (ptr.obj->values.count(UNKNOWN_OFFSET) != 0)
                    for (auto memval : ptr.obj->values[UNKNOWN_OFFSET])
                        changed |= node->addValue(memval);
                    
            }
        }
    }

    return changed;
}

static const Module *
valueGetModule(const Value *V)
{
    static const Module *module;

    // we always work in one module,
    // so we can store it and reuse it
    if (module)
        return module;

    const Instruction *I = dyn_cast<Instruction>(V);
    assert(I && "works only for Instruction derivatives");

    const Function *M = I->getParent() ? I->getParent()->getParent() : nullptr;
    assert(M && "No function?");

    module =  M->getParent();
    return module;
}

static bool handleGepInst(const GetElementPtrInst *Inst, LLVMNode *node)
{
    bool changed = false;
    LLVMNode *ptrNode = node->getOperand(0);
    const DataLayout& DL = valueGetModule(node->getKey())->getDataLayout();
    APInt offset(64, 0);

    if (ptrNode->hasUnknownValue())
        return node->setUnknownValue();

    if (Inst->accumulateConstantOffset(DL, offset))
        if (offset.isIntN(64)) {
            for (auto ptr : ptrNode->getPointsTo()) {
                changed |= node->addPointsTo(ptr.obj,
                                             ptr.offset + offset.getZExtValue());
                                             // accumulate the offset of pointers
            }

            return changed;
        } else
            errs() << "WARN: GEP offset greater that 64-bit\n";

    for (auto ptr : ptrNode->getPointsTo())
        // UKNOWN_OFFSET + something is still unknown
        changed |= node->addPointsTo(ptr.obj, UNKNOWN_OFFSET);

    return changed;
}

static bool handleCallInst(const CallInst *Inst, LLVMNode *node)
{
    bool changed = false;
    bool isptr;
    LLVMNode **operands = node->getOperands();

    auto subgraphs = node->getSubgraphs();
    // function is undefined?
    if (subgraphs.empty()) {
        Type *Ty = Inst->getType();
        if (!Ty->isVoidTy())
            return node->setUnknownValue();

        return false;
    }

    for (auto sub : subgraphs) {
        LLVMDGParameters *formal = sub->getParameters();
        assert(formal && "No formal params in subgraph");
        const Function *subfunc = dyn_cast<Function>(sub->getEntry()->getKey());
        assert(subfunc && "Entry is not a llvm::Function");

        int i = 1;
        for (auto I = subfunc->arg_begin(), E = subfunc->arg_end(); I != E; ++I) {
            LLVMNode *op = operands[i++];
            const Value *val = op->getKey();
            isptr = val->getType()->isPointerTy();

            if (isptr) {
                LLVMDGParameter *p = formal->find(&*I);
                if (!p) {
                    errs() << "No such formal param: " << *val << "\n";
                    return false;
                }

                for (auto ptr : op->getPointsTo())
                    changed |= p->in->addPointsTo(ptr);
            } else {
                errs() << "NOT IMPLEMENTED YET\n";
            }
        }
    }

    // what about llvm intrinsic functions like llvm.memset?
    // we could handle those

    return changed;
}

static bool handleMemSetInst(const MemSetInst *Inst, LLVMNode *node)
{
    bool changed = false;
    LLVMNode *dest = node->getOperand(1);
    LLVMNode *val = node->getOperand(2);
    const Value *len;
    uint64_t size;
 
    len = Inst->getArgOperand(2);

    if (const Constant *c = dyn_cast<Constant>(len)) {
        const APInt& apint = c->getUniqueInteger();

        if (apint.isIntN(64))
            size = apint.getZExtValue();
        else
            return false;
    } else
        return false;

    if (!val) {
        val = new LLVMNode(Inst->getArgOperand(1));
        node->setOperand(val, 2);
    }

    // FIXME
    // suppose whole node is memset to the value
    // and do it some efficiently!
    Type *Ty = dest->getKey()->stripPointerCasts()->getType()->getContainedType(0);
    if (Ty->isSingleValueType()) {
        for (auto ptr : dest->getPointsTo())
            changed |= ptr.obj->values[0].insert(val).second;

        return changed;
    }

    uint64_t jump;
    if (true || Ty->isArrayTy()) {
         jump = Ty->getContainedType(0)->getPrimitiveSizeInBits() / 4;
         errs() << "MEMSET [array] of size " << size << " (jump "
                << jump << ") of value" << *Ty << "\n";

         for (auto ptr : dest->getPointsTo())
            for (uint64_t off = 0; off < size; off += jump)
                changed |= ptr.obj->values[off].insert(val).second;
    } // else if struct type

    return changed;
}



static bool handleBitCastInst(const BitCastInst *Inst, LLVMNode *node)
{
    bool changed = false;
    LLVMNode *op = node->getOperand(0);
    if (!op) {
        errs() << "WARN: Cast without operand " << *Inst << "\n";
        return false;
    }

    if (op->hasUnknownValue())
        node->setUnknownValue();

    if (Inst->isLosslessCast()) {
        if (Inst->getType()->isPointerTy()) {
            for (auto ptr : op->getPointsTo())
                changed |= node->addPointsTo(ptr);
        } else {
            errs() << "WARN: Not implemented non-pointer casts " << *Inst << "\n";
        }
    } else
        errs() << "WARN: Not a loss less cast unhandled" << *Inst << "\n";

    return changed;
}

static bool handleReturnInst(const ReturnInst *Inst, LLVMNode *node)
{
    bool changed = false;
    LLVMNode *val = node->getOperand(0);

    // val may be a constant, which is not in the graph
    if (!val) {
        val = new LLVMNode(Inst->getReturnValue());
        val->addValue(val);
        node->setOperand(val, 0);
    }

    if (val->hasUnknownValue())
        node->setUnknownValue();

    // XXX pointers?
    for (auto v : val->getValues())
        changed |= node->addValue(v);

    return changed;
}

bool LLVMValueFlowAnalysis::runOnNode(LLVMNode *node)
{
    bool changed = false;
    const Value *val = node->getKey();

    if (const AllocaInst *Inst = dyn_cast<AllocaInst>(val)) {
        changed |= handleAllocaInst(Inst, node);
    } else if (const StoreInst *Inst = dyn_cast<StoreInst>(val)) {
        changed |= handleStoreInst(Inst, node);
    } else if (const LoadInst *Inst = dyn_cast<LoadInst>(val)) {
        changed |= handleLoadInst(Inst, node);
    } else if (const GetElementPtrInst *Inst = dyn_cast<GetElementPtrInst>(val)) {
        changed |= handleGepInst(Inst, node);
    } else if (const MemSetInst *Inst = dyn_cast<MemSetInst>(val)) {
        // this must be handled before callinst because memset
        // is callinst too
        changed |= handleMemSetInst(Inst, node);
    } else if (const CallInst *Inst = dyn_cast<CallInst>(val)) {
        changed |= handleCallInst(Inst, node);
    } else if (const ReturnInst *Inst = dyn_cast<ReturnInst>(val)) {
        changed |= handleReturnInst(Inst, node);
    } else if (const BitCastInst *Inst = dyn_cast<BitCastInst>(val)) {
        changed |= handleBitCastInst(Inst, node);
    } else {
        // nothing else we can do
        node->setUnknownValue();
        errs() << "WARN: Unhandled instruction: " << *val << "\n";
    }

    return changed;
}

} // namespace analysis
} // namespace dg
