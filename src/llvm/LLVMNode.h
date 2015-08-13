/// XXX add licence

#ifdef HAVE_LLVM

#ifndef _LLVM_NODE_H_
#define _LLVM_NODE_H_

#ifndef ENABLE_CFG
#error "Need CFG enabled"
#endif

#include <utility>
#include <map>
#include <set>

namespace dg {

class LLVMDependenceGraph;
class LLVMNode;

typedef dg::BBlock<LLVMNode> LLVMBBlock;
typedef dg::DGParameter<LLVMNode> LLVMDGParameter;
typedef dg::DGParameters<const llvm::Value *, LLVMNode> LLVMDGParameters;

#define UNKNOWN_OFFSET ~((uint64_t) 0)

// just a wrapper around uint64_t to
// handle UNKNOWN_OFFSET somehow easily
// maybe later we'll make it a range
struct Offset
{
    Offset(uint64_t o = UNKNOWN_OFFSET) : offset(o) {}
    Offset& operator+(const Offset& o)
    {
        if (offset == UNKNOWN_OFFSET)
            return *this;

        if (o.offset == UNKNOWN_OFFSET)
            offset = UNKNOWN_OFFSET;
        else
            offset += o.offset;

        return *this;
    }

    bool operator<(const Offset& o) const
    {
        return offset < o.offset;
    }

    uint64_t operator*() const { return offset; }
    const uint64_t *operator->() const { return &offset; }

    uint64_t offset;
};

struct MemoryObj;
struct Pointer
{
    Pointer(MemoryObj *m, Offset off = 0) : obj(m), offset(off) {}

    MemoryObj *obj;
    Offset offset;

    bool operator<(const Pointer& oth) const
    {
        return obj == oth.obj ? offset < oth.offset : obj < oth.obj;
    }
};

typedef std::set<Pointer> PointsToSetT;
typedef std::set<LLVMNode *> ValuesSetT;
typedef std::map<Offset, PointsToSetT> PointsToMapT;
typedef std::map<Offset, ValuesSetT> ValuesMapT;

struct MemoryObj
{
    MemoryObj(LLVMNode *n) : node(n) {}
    LLVMNode *node;

    PointsToMapT pointsTo;
    ValuesMapT values;
#if 0
    // actually we should take offsets into
    // account, since we can memset only part of the memory
    ValuesSetT memsetTo;
#endif
};

/// ------------------------------------------------------------------
//  -- LLVMNode
/// ------------------------------------------------------------------
class LLVMNode : public dg::Node<LLVMDependenceGraph,
                                 const llvm::Value *, LLVMNode>
{

public:
    LLVMNode(const llvm::Value *val)
        :dg::Node<LLVMDependenceGraph, const llvm::Value *, LLVMNode>(val),
         operands(nullptr), operands_num(0), memoryobj(nullptr), has_unknown_value(false)
    {}

    ~LLVMNode()
    {
        delete memoryobj;
        delete[] operands;
    }

    const llvm::Value *getValue() const
    {
        return getKey();
    }

    // create new subgraph with actual parameters that are given
    // by call-site and add parameter edges between actual and
    // formal parameters. The argument is the graph of called function.
    // Must be called only when node is call-site.
    // XXX create new class for parameters
    void addActualParameters(LLVMDependenceGraph *);

    LLVMNode **getOperands()
    {
        if (!operands)
            findOperands();

        return operands;
    }

    size_t getOperandsNum()
    {
        if (!operands)
            findOperands();

        return operands_num;
    }

    LLVMNode *getOperand(unsigned int idx)
    {
        if (!operands)
            findOperands();

        assert(operands_num > idx && "idx is too high");
        return operands[idx];
    }

    LLVMNode *setOperand(LLVMNode *op, unsigned int idx)
    {
        assert(operands && "Operands has not been found yet");
        assert(operands_num > idx && "idx is too high");

        LLVMNode *old = operands[idx];
        operands[idx] = op;

        return old;
    }

    ValuesSetT& getValues() { return values; }
    const ValuesSetT& getValues() const { return values; }
    PointsToSetT& getPointsTo() { return pointsTo; }
    const PointsToSetT& getPointsTo() const { return pointsTo; }
    MemoryObj *&getMemoryObj() { return memoryobj; }
    MemoryObj *getMemoryObj() const { return memoryobj; }

    bool addValue(LLVMNode *n)
    {
        return values.insert(n).second;
    }

    bool addPointsTo(const Pointer& vr)
    {
        return pointsTo.insert(vr).second;
    }

    bool addPointsTo(MemoryObj *m, Offset off = 0)
    {
        return pointsTo.insert(Pointer(m, off)).second;
    }

    bool hasUnknownValue() const { return has_unknown_value; }
    bool setUnknownValue()
    {
        if (has_unknown_value)
            return false;

        has_unknown_value = true;

        assert(!memoryobj && "Node with unknown value must not have memoryobj");
        values.clear();
        pointsTo.clear();

        return true;
    }

private:
    LLVMNode **findOperands();
    // here we can store operands of instructions so that
    // finding them will be asymptotically constant
    LLVMNode **operands;
    size_t operands_num;

    MemoryObj *memoryobj;
    PointsToSetT pointsTo;
    ValuesSetT values;

    bool has_unknown_value;
};

} // namespace dg

#endif // _LLVM_NODE_H_

#endif /* HAVE_LLVM */
