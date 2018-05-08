#include "taco/index_notation/index_notation.h"

#include <iostream>

#include "error/error_checks.h"
#include "taco/error/error_messages.h"
#include "taco/type.h"
#include "taco/format.h"

#include "taco/index_notation/schedule.h"
#include "taco/index_notation/index_notation_nodes.h"
#include "taco/index_notation/index_notation_rewriter.h"
#include "taco/index_notation/index_notation_printer.h"

#include "taco/util/name_generator.h"
#include "taco/util/scopedmap.h"

using namespace std;

namespace taco {

// class IndexExpr
IndexExpr::IndexExpr(TensorVar var) : IndexExpr(new AccessNode(var,{})) {
}

IndexExpr::IndexExpr(char val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(int8_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(int16_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(int32_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(int64_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(uint8_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(uint16_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(uint32_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(uint64_t val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(float val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(double val) : IndexExpr(new LiteralNode(val)) {
}

IndexExpr::IndexExpr(std::complex<float> val) :IndexExpr(new LiteralNode(val)){
}

IndexExpr::IndexExpr(std::complex<double> val) :IndexExpr(new LiteralNode(val)){
}

void IndexExpr::splitOperator(IndexVar old, IndexVar left, IndexVar right) {
  const_cast<IndexExprNode*>(this->ptr)->splitOperator(old, left, right);
}
  
DataType IndexExpr::getDataType() const {
  return const_cast<IndexExprNode*>(this->ptr)->getDataType();
}

void IndexExpr::accept(IndexExprVisitorStrict *v) const {
  ptr->accept(v);
}

std::ostream& operator<<(std::ostream& os, const IndexExpr& expr) {
  if (!expr.defined()) return os << "IndexExpr()";
  IndexNotationPrinter printer(os);
  printer.print(expr);
  return os;
}

struct Equals : public IndexNotationVisitorStrict {
  bool eq = false;
  IndexExpr bExpr;
  IndexStmt bStmt;

  bool check(IndexExpr a, IndexExpr b) {
    this->bExpr = b;
    a.accept(this);
    return eq;
  }

  bool check(IndexStmt a, IndexStmt b) {
    this->bStmt = b;
    a.accept(this);
    return eq;
  }

  using IndexNotationVisitorStrict::visit;

  void visit(const AccessNode* anode) {
    if (!isa<AccessNode>(bExpr.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<AccessNode>(bExpr.ptr);
    if (anode->tensorVar != bnode->tensorVar) {
      eq = false;
      return;
    }
    if (anode->indexVars.size() != anode->indexVars.size()) {
      eq = false;
      return;
    }
    for (size_t i = 0; i < anode->indexVars.size(); i++) {
      if (anode->indexVars[i] != bnode->indexVars[i]) {
        eq = false;
        return;
      }
    }
    eq = true;
  }

  void visit(const LiteralNode* anode) {
    if (!isa<LiteralNode>(bExpr.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<LiteralNode>(bExpr.ptr);
    if (anode->getDataType() != bnode->getDataType()) {
      eq = false;
      return;
    }
    if (memcmp(anode->val,bnode->val,anode->getDataType().getNumBytes()) != 0) {
        std::cout << "here" << std::endl;
      eq = false;
      return;
    }
    eq = true;
  }

  template <class T>
  bool unaryEquals(const T* anode, IndexExpr b) {
    if (!isa<T>(b.ptr)) {
      return false;
    }
    auto bnode = to<T>(b.ptr);
    if (!equals(anode->a, bnode->a)) {
      return false;
    }
    return true;
  }

  void visit(const NegNode* anode) {
    eq = unaryEquals(anode, bExpr);
  }

  void visit(const SqrtNode* anode) {
    eq = unaryEquals(anode, bExpr);
  }

  template <class T>
  bool binaryEquals(const T* anode, IndexExpr b) {
    if (!isa<T>(b.ptr)) {
      return false;
    }
    auto bnode = to<T>(b.ptr);
    if (!equals(anode->a, bnode->a) || !equals(anode->b, bnode->b)) {
      return false;
    }
    return true;
  }

  void visit(const AddNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const SubNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const MulNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const DivNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const ReductionNode* anode) {
    if (!isa<ReductionNode>(bExpr.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<ReductionNode>(bExpr.ptr);
    if (!(equals(anode->op, bnode->op) && equals(anode->a, bnode->a))) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const AssignmentNode* anode) {
    if (!isa<AssignmentNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<AssignmentNode>(bStmt.ptr);
    if (!equals(anode->lhs, bnode->lhs) || !equals(anode->rhs, bnode->rhs) ||
        !equals(anode->op, bnode->op)) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const ForallNode* anode) {
    if (!isa<ForallNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<ForallNode>(bStmt.ptr);
    if (anode->indexVar != bnode->indexVar ||
        !equals(anode->stmt, bnode->stmt)) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const WhereNode* anode) {
    if (!isa<WhereNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<WhereNode>(bStmt.ptr);
    if (!equals(anode->consumer, bnode->consumer) ||
        !equals(anode->producer, bnode->producer)) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const MultiNode* anode) {
    if (!isa<MultiNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<MultiNode>(bStmt.ptr);
    if (!equals(anode->stmt1, bnode->stmt1) ||
        !equals(anode->stmt2, bnode->stmt2)) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const SequenceNode* anode) {
    if (!isa<SequenceNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<SequenceNode>(bStmt.ptr);
    if (!equals(anode->definition, bnode->definition) ||
        !equals(anode->mutation, bnode->mutation)) {
      eq = false;
      return;
    }
    eq = true;
  }
};

bool equals(IndexExpr a, IndexExpr b) {
  if (!a.defined() && !b.defined()) {
    return true;
  }
  if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
    return false;
  }
  return Equals().check(a,b);
}

IndexExpr operator-(const IndexExpr& expr) {
  return new NegNode(expr.ptr);
}

IndexExpr operator+(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new AddNode(lhs, rhs);
}

IndexExpr operator-(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new SubNode(lhs, rhs);
}

IndexExpr operator*(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new MulNode(lhs, rhs);
}

IndexExpr operator/(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new DivNode(lhs, rhs);
}

vector<IndexVar> getIndexVars(const IndexExpr& expr) {
  vector<IndexVar> indexVars;
  set<IndexVar> seen;
  match(expr,
    function<void(const AccessNode*)>([&](const AccessNode* op) {
      for (auto& var : op->indexVars) {
        if (!util::contains(seen, var)) {
          seen.insert(var);
          indexVars.push_back(var);
        }
      }
    })
  );
  return indexVars;
}

struct Simplify : public ExprRewriterStrict {
public:
  Simplify(const set<Access>& zeroed) : zeroed(zeroed) {}

private:
  using ExprRewriterStrict::visit;

  set<Access> zeroed;
  void visit(const AccessNode* op) {
    if (util::contains(zeroed, op)) {
      expr = IndexExpr();
    }
    else {
      expr = op;
    }
  }

  void visit(const LiteralNode* op) {
    expr = op;
  }

  template <class T>
  IndexExpr visitUnaryOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    if (!a.defined()) {
      return IndexExpr();
    }
    else if (a == op->a) {
      return op;
    }
    else {
      return new T(a);
    }
  }

  void visit(const NegNode* op) {
    expr = visitUnaryOp(op);
  }

  void visit(const SqrtNode* op) {
    expr = visitUnaryOp(op);
  }

  template <class T>
  IndexExpr visitDisjunctionOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    IndexExpr b = rewrite(op->b);
    if (!a.defined() && !b.defined()) {
      return IndexExpr();
    }
    else if (!a.defined()) {
      return b;
    }
    else if (!b.defined()) {
      return a;
    }
    else if (a == op->a && b == op->b) {
      return op;
    }
    else {
      return new T(a, b);
    }
  }

  template <class T>
  IndexExpr visitConjunctionOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    IndexExpr b = rewrite(op->b);
    if (!a.defined() || !b.defined()) {
      return IndexExpr();
    }
    else if (a == op->a && b == op->b) {
      return op;
    }
    else {
      return new T(a, b);
    }
  }

  void visit(const AddNode* op) {
    expr = visitDisjunctionOp(op);
  }

  void visit(const SubNode* op) {
    expr = visitDisjunctionOp(op);
  }

  void visit(const MulNode* op) {
    expr = visitConjunctionOp(op);
  }

  void visit(const DivNode* op) {
    expr = visitConjunctionOp(op);
  }

  void visit(const ReductionNode* op) {
    IndexExpr a = rewrite(op->a);
    if (!a.defined()) {
      expr = IndexExpr();
    }
    else if (a == op->a) {
      expr = op;
    }
    else {
      expr = new ReductionNode(op->op, op->var, a);
    }
  }
};

IndexExpr simplify(const IndexExpr& expr, const set<Access>& zeroed) {
  return Simplify(zeroed).rewrite(expr);
}


// class Access
Access::Access(const AccessNode* n) : IndexExpr(n) {
}

Access::Access(const TensorVar& tensor, const std::vector<IndexVar>& indices)
    : Access(new AccessNode(tensor, indices)) {
}

const TensorVar& Access::getTensorVar() const {
  return getNode(*this)->tensorVar;
}

const std::vector<IndexVar>& Access::getIndexVars() const {
  return getNode(*this)->indexVars;
}

Assignment Access::operator=(const IndexExpr& expr) {
  TensorVar result = getTensorVar();
  Assignment assignment = Assignment(result, getIndexVars(), expr);
  const_cast<AccessNode*>(getNode(*this))->setAssignment(assignment);
  return assignment;
}

Assignment Access::operator=(const Access& expr) {
  return operator=(static_cast<IndexExpr>(expr));
}

Assignment Access::operator=(const TensorVar& var) {
  return operator=(Access(var));
}

Assignment Access::operator+=(const IndexExpr& expr) {
  TensorVar result = getTensorVar();
  Assignment assignment = Assignment(result, getIndexVars(), expr, new AddNode);
  const_cast<AccessNode*>(getNode(*this))->setAssignment(assignment);
  return assignment;
}

template <> bool isa<Access>(IndexExpr e) {
  return isa<AccessNode>(e.ptr);
}

template <> Access to<Access>(IndexExpr e) {
  taco_iassert(isa<Access>(e));
  return Access(to<AccessNode>(e.ptr));
}


// class Literal
Literal::Literal(const LiteralNode* n) : IndexExpr(n) {
}

Literal::Literal(bool val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(unsigned char val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(unsigned short val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(unsigned int val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(unsigned long val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(unsigned long long val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(char val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(short val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(int val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(long val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(long long val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(int8_t val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(float val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(double val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(std::complex<float> val) : Literal(new LiteralNode(val)) {
}

Literal::Literal(std::complex<double> val) : Literal(new LiteralNode(val)) {
}

template <typename T> T Literal::getVal() const {
  return getNode(*this)->getVal<T>();
}
template bool Literal::getVal() const;
template unsigned char Literal::getVal() const;
template unsigned short Literal::getVal() const;
template unsigned int Literal::getVal() const;
template unsigned long Literal::getVal() const;
template unsigned long long Literal::getVal() const;
template char Literal::getVal() const;
template short Literal::getVal() const;
template int Literal::getVal() const;
template long Literal::getVal() const;
template long long Literal::getVal() const;
template int8_t Literal::getVal() const;
template float Literal::getVal() const;
template double Literal::getVal() const;
template std::complex<float> Literal::getVal() const;
template std::complex<double> Literal::getVal() const;

template <> bool isa<Literal>(IndexExpr e) {
  return isa<LiteralNode>(e.ptr);
}

template <> Literal to<Literal>(IndexExpr e) {
  taco_iassert(isa<Literal>(e));
  return Literal(to<LiteralNode>(e.ptr));
}


// class Neg
Neg::Neg(const NegNode* n) : IndexExpr(n) {
}

Neg::Neg(IndexExpr a) : Neg(new NegNode(a)) {
}

IndexExpr Neg::getA() const {
  return getNode(*this)->a;
}

template <> bool isa<Neg>(IndexExpr e) {
  return isa<NegNode>(e.ptr);
}

template <> Neg to<Neg>(IndexExpr e) {
  taco_iassert(isa<Neg>(e));
  return Neg(to<NegNode>(e.ptr));
}


// class Add
Add::Add(const AddNode* n) : IndexExpr(n) {
}

Add::Add(IndexExpr a, IndexExpr b) : Add(new AddNode(a, b)) {
}

IndexExpr Add::getA() const {
  return getNode(*this)->a;
}

IndexExpr Add::getB() const {
  return getNode(*this)->b;
}

template <> bool isa<Add>(IndexExpr e) {
  return isa<AddNode>(e.ptr);
}

template <> Add to<Add>(IndexExpr e) {
  taco_iassert(isa<Add>(e));
  return Add(to<AddNode>(e.ptr));
}


// class Sub
Sub::Sub(const SubNode* n) : IndexExpr(n) {
}

Sub::Sub(IndexExpr a, IndexExpr b) : Sub(new SubNode(a, b)) {
}

IndexExpr Sub::getA() const {
  return getNode(*this)->a;
}

IndexExpr Sub::getB() const {
  return getNode(*this)->b;
}

template <> bool isa<Sub>(IndexExpr e) {
  return isa<SubNode>(e.ptr);
}

template <> Sub to<Sub>(IndexExpr e) {
  taco_iassert(isa<Sub>(e));
  return Sub(to<SubNode>(e.ptr));
}


// class Mul
Mul::Mul(const MulNode* n) : IndexExpr(n) {
}

Mul::Mul(IndexExpr a, IndexExpr b) : Mul(new MulNode(a, b)) {
}

IndexExpr Mul::getA() const {
  return getNode(*this)->a;
}

IndexExpr Mul::getB() const {
  return getNode(*this)->b;
}

template <> bool isa<Mul>(IndexExpr e) {
  return isa<MulNode>(e.ptr);
}

template <> Mul to<Mul>(IndexExpr e) {
  taco_iassert(isa<Mul>(e));
  return Mul(to<MulNode>(e.ptr));
}


// class Div
Div::Div(const DivNode* n) : IndexExpr(n) {
}

Div::Div(IndexExpr a, IndexExpr b) : Div(new DivNode(a, b)) {
}

IndexExpr Div::getA() const {
  return getNode(*this)->a;
}

IndexExpr Div::getB() const {
  return getNode(*this)->b;
}

template <> bool isa<Div>(IndexExpr e) {
  return isa<DivNode>(e.ptr);
}

template <> Div to<Div>(IndexExpr e) {
  taco_iassert(isa<Div>(e));
  return Div(to<DivNode>(e.ptr));
}


// class Sqrt
Sqrt::Sqrt(const SqrtNode* n) : IndexExpr(n) {
}

Sqrt::Sqrt(IndexExpr a) : Sqrt(new SqrtNode(a)) {
}

IndexExpr Sqrt::getA() const {
  return getNode(*this)->a;
}

template <> bool isa<Sqrt>(IndexExpr e) {
  return isa<SqrtNode>(e.ptr);
}

template <> Sqrt to<Sqrt>(IndexExpr e) {
  taco_iassert(isa<Sqrt>(e));
  return Sqrt(to<SqrtNode>(e.ptr));
}

IndexExpr sqrt(IndexExpr expr) {
  return Sqrt(expr);
}


// class Sum
Reduction::Reduction(const ReductionNode* n) : IndexExpr(n) {
}

Reduction::Reduction(IndexExpr op, IndexVar var, IndexExpr expr)
    : Reduction(new ReductionNode(op, var, expr)) {
}

Reduction sum(IndexVar i, IndexExpr expr) {
  return Reduction(new AddNode, i, expr);
}


// class IndexStmt
IndexStmt::IndexStmt() : util::IntrusivePtr<const IndexStmtNode>(nullptr) {
}

IndexStmt::IndexStmt(const IndexStmtNode* n)
    : util::IntrusivePtr<const IndexStmtNode>(n) {
}

void IndexStmt::accept(IndexNotationVisitorStrict *v) const {
  ptr->accept(v);
}

std::vector<IndexVar> IndexStmt::getIndexVars() const {
  vector<IndexVar> vars;;
  set<IndexVar> seen;
  match(*this,
    std::function<void(const AssignmentNode*,Matcher*)>([&](
        const AssignmentNode* op, Matcher* ctx) {
      for (auto& var : op->lhs.getIndexVars()) {
        if (!util::contains(seen, var)) {
          vars.push_back(var);
          seen.insert(var);
        }
      }
      ctx->match(op->rhs);
    }),
    std::function<void(const AccessNode*)>([&](const AccessNode* op) {
      for (auto& var : op->indexVars) {
        if (!util::contains(seen, var)) {
          vars.push_back(var);
          seen.insert(var);
        }
      }
    })
  );
  return vars;
}

map<IndexVar,Dimension> IndexStmt::getIndexVarDomains() {
  map<IndexVar, Dimension> indexVarDomains;
  match(*this,
    std::function<void(const AssignmentNode*,Matcher*)>([&indexVarDomains](
        const AssignmentNode* op, Matcher* ctx) {
      ctx->match(op->lhs);
      ctx->match(op->rhs);
    }),
    function<void(const AccessNode*)>([&indexVarDomains](const AccessNode* op) {
      auto& type = op->tensorVar.getType();
      auto& vars = op->indexVars;
      for (size_t i = 0; i < vars.size(); i++) {
        if (!util::contains(indexVarDomains, vars[i])) {
          indexVarDomains.insert({vars[i], type.getShape().getDimension(i)});
        }
        else {
          taco_iassert(indexVarDomains.at(vars[i]) ==
                       type.getShape().getDimension(i))
              << "Index variable used to index incompatible dimensions";
        }
      }
    })
  );

  return indexVarDomains;
}

bool equals(IndexStmt a, IndexStmt b) {
  if (!a.defined() && !b.defined()) {
    return true;
  }
  if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
    return false;
  }
  return Equals().check(a,b);
}

std::ostream& operator<<(std::ostream& os, const IndexStmt& expr) {
  if (!expr.defined()) return os << "TensorExpr()";
  IndexNotationPrinter printer(os);
  printer.print(expr);
  return os;
}


// class Assignment
Assignment::Assignment(const AssignmentNode* n) : IndexStmt(n) {
}

Assignment::Assignment(Access lhs, IndexExpr rhs, IndexExpr op)
    : Assignment(new AssignmentNode(lhs, rhs, op)) {
}

Assignment::Assignment(TensorVar tensor, vector<IndexVar> indices,
                       IndexExpr rhs, IndexExpr op)
    : Assignment(Access(tensor, indices), rhs, op) {
}

Access Assignment::getLhs() const {
  return getNode(*this)->lhs;
}

IndexExpr Assignment::getRhs() const {
  return getNode(*this)->rhs;
}

IndexExpr Assignment::getOp() const {
  return getNode(*this)->op;
}

const std::vector<IndexVar>& Assignment::getFreeVars() const {
  return getLhs().getIndexVars();
}

std::vector<IndexVar> Assignment::getReductionVars() const {
  vector<IndexVar> freeVars = getLhs().getIndexVars();
  set<IndexVar> seen(freeVars.begin(), freeVars.end());
  vector<IndexVar> reductionVars;
  match(getRhs(),
    std::function<void(const AccessNode*)>([&](const AccessNode* op) {
    for (auto& var : op->indexVars) {
      if (!util::contains(seen, var)) {
        reductionVars.push_back(var);
        seen.insert(var);
      }
    }
    })
  );
  return reductionVars;
}

template <> bool isa<Assignment>(IndexStmt s) {
  return isa<AssignmentNode>(s.ptr);
}

template <> Assignment to<Assignment>(IndexStmt s) {
  taco_iassert(isa<Assignment>(s));
  return Assignment(to<AssignmentNode>(s.ptr));
}


// class Forall
Forall::Forall(const ForallNode* n) : IndexStmt(n) {
}

Forall::Forall(IndexVar indexVar, IndexStmt stmt)
    : Forall(new ForallNode(indexVar, stmt)) {
}

IndexVar Forall::getIndexVar() const {
  return getNode(*this)->indexVar;
}

IndexStmt Forall::getStmt() const {
  return getNode(*this)->stmt;
}

Forall forall(IndexVar i, IndexStmt expr) {
  return Forall(i, expr);
}

template <> bool isa<Forall>(IndexStmt s) {
  return isa<ForallNode>(s.ptr);
}

template <> Forall to<Forall>(IndexStmt s) {
  taco_iassert(isa<Forall>(s));
  return Forall(to<ForallNode>(s.ptr));
}


// class Where
Where::Where(const WhereNode* n) : IndexStmt(n) {
}

Where::Where(IndexStmt consumer, IndexStmt producer)
    : Where(new WhereNode(consumer, producer)) {
}

IndexStmt Where::getConsumer() {
  return getNode(*this)->consumer;
}

IndexStmt Where::getProducer() {
  return getNode(*this)->producer;
}

Where where(IndexStmt consumer, IndexStmt producer) {
  return Where(consumer, producer);
}

template <> bool isa<Where>(IndexStmt s) {
  return isa<WhereNode>(s.ptr);
}

template <> Where to<Where>(IndexStmt s) {
  taco_iassert(isa<Where>(s));
  return Where(to<WhereNode>(s.ptr));
}


// class Multi
Multi::Multi(const MultiNode* n) : IndexStmt(n) {
}

Multi::Multi(IndexStmt stmt1, IndexStmt stmt2)
    : Multi(new MultiNode(stmt1, stmt2)) {
}

IndexStmt Multi::getStmt1() const {
  return getNode(*this)->stmt1;
}

IndexStmt Multi::getStmt2() const {
  return getNode(*this)->stmt2;
}

Multi multi(IndexStmt stmt1, IndexStmt stmt2) {
  return Multi(stmt1, stmt2);
}

template <> bool isa<Multi>(IndexStmt s) {
  return isa<MultiNode>(s.ptr);
}

template <> Multi to<Multi>(IndexStmt s) {
  taco_iassert(isa<Multi>(s));
  return Multi(to<MultiNode>(s.ptr));
}


// class Sequence
Sequence::Sequence(const SequenceNode* n) :IndexStmt(n) {
}

Sequence::Sequence(IndexStmt definition, IndexStmt mutation)
    : Sequence(new SequenceNode(definition, mutation)) {
}

IndexStmt Sequence::getDefinition() const {
  return getNode(*this)->definition;
}

IndexStmt Sequence::getMutation() const {
  return getNode(*this)->mutation;
}

Sequence sequence(IndexStmt definition, IndexStmt mutation) {
  return Sequence(definition, mutation);
}

template <> bool isa<Sequence>(IndexStmt s) {
  return isa<SequenceNode>(s.ptr);
}

template <> Sequence to<Sequence>(IndexStmt s) {
  taco_iassert(isa<Sequence>(s));
  return Sequence(to<SequenceNode>(s.ptr));
}


// class IndexVar
struct IndexVar::Content {
  string name;
};

IndexVar::IndexVar() : IndexVar(util::uniqueName('i')) {}

IndexVar::IndexVar(const std::string& name) : content(new Content) {
  content->name = name;
}

std::string IndexVar::getName() const {
  return content->name;
}

bool operator==(const IndexVar& a, const IndexVar& b) {
  return a.content == b.content;
}

bool operator<(const IndexVar& a, const IndexVar& b) {
  return a.content < b.content;
}

std::ostream& operator<<(std::ostream& os, const IndexVar& var) {
  return os << var.getName();
}


// class TensorVar
struct TensorVar::Content {
  string name;
  Type type;
  Format format;

  /// An expression to compute this tensor variable.
  Assignment assignment;

  Schedule schedule;
};

TensorVar::TensorVar() : TensorVar(Type()) {
}

TensorVar::TensorVar(const Type& type) : TensorVar(type, Dense) {
}

TensorVar::TensorVar(const std::string& name, const Type& type)
    : TensorVar(name, type, Dense) {
}

TensorVar::TensorVar(const Type& type, const Format& format)
    : TensorVar(util::uniqueName('A'), type, format) {
}

TensorVar::TensorVar(const string& name, const Type& type, const Format& format)
    : content(new Content) {
  content->name = name;
  content->type = type;
  content->format = format;
}

std::string TensorVar::getName() const {
  return content->name;
}

int TensorVar::getOrder() const {
  return content->type.getShape().getOrder();
}

const Type& TensorVar::getType() const {
  return content->type;
}

const Format& TensorVar::getFormat() const {
  return content->format;
}

const Assignment& TensorVar::getAssignment() const {
  return content->assignment;
}

const Schedule& TensorVar::getSchedule() const {
  struct GetSchedule : public IndexNotationVisitor {
    using IndexNotationVisitor::visit;
    Schedule schedule;
    void visit(const BinaryExprNode* expr) {
      for (auto& operatorSplit : expr->getOperatorSplits()) {
        schedule.addOperatorSplit(operatorSplit);
      }
    }
  };
  GetSchedule getSchedule;
  content->schedule.clearOperatorSplits();
  getSchedule.schedule = content->schedule;
  getAssignment().getRhs().accept(&getSchedule);
  return content->schedule;
}

void TensorVar::setName(std::string name) {
  content->name = name;
}

void TensorVar::setAssignment(Assignment assignment) {
  auto freeVars = assignment.getLhs().getIndexVars();
  auto indexExpr = assignment.getRhs();

  auto shape = getType().getShape();
  taco_uassert(error::dimensionsTypecheck(freeVars, indexExpr, shape))
      << error::expr_dimension_mismatch << " "
      << error::dimensionTypecheckErrors(freeVars, indexExpr, shape);

  // The following are index expressions the implementation doesn't currently
  // support, but that are planned for the future.
  taco_uassert(!error::containsTranspose(this->getFormat(), freeVars, indexExpr))
      << error::expr_transposition;

  content->assignment = assignment;
}

const Access TensorVar::operator()(const std::vector<IndexVar>& indices) const {
  taco_uassert((int)indices.size() == getOrder()) <<
      "A tensor of order " << getOrder() << " must be indexed with " <<
      getOrder() << " variables, but is indexed with:  " << util::join(indices);
  return Access(new AccessNode(*this, indices));
}

Access TensorVar::operator()(const std::vector<IndexVar>& indices) {
  taco_uassert((int)indices.size() == getOrder()) <<
      "A tensor of order " << getOrder() << " must be indexed with " <<
      getOrder() << " variables, but is indexed with:  " << util::join(indices);
  return Access(new AccessNode(*this, indices));
}

Assignment TensorVar::operator=(const IndexExpr& expr) {
  taco_uassert(getOrder() == 0)
      << "Must use index variable on the left-hand-side when assigning an "
      << "expression to a non-scalar tensor.";
  Assignment assignment = Assignment(*this, {}, expr);
  setAssignment(assignment);
  return assignment;
}

Assignment TensorVar::operator+=(const IndexExpr& expr) {
  taco_uassert(getOrder() == 0)
      << "Must use index variable on the left-hand-side when assigning an "
      << "expression to a non-scalar tensor.";
  Assignment assignment = Assignment(*this, {}, expr, new AddNode);
  setAssignment(assignment);
  return assignment;
}

bool operator==(const TensorVar& a, const TensorVar& b) {
  return a.content == b.content;
}

bool operator<(const TensorVar& a, const TensorVar& b) {
  return a.content < b.content;
}

std::ostream& operator<<(std::ostream& os, const TensorVar& var) {
  return os << var.getName() << " : " << var.getType();
}


// functions
bool isEinsumNotation(const IndexStmt& stmt) {
  if (!isa<Assignment>(stmt)) {
    return false;
  }

  // Einsum notation until proved otherwise
  bool isEinsum = true;

  // Additions are not allowed under the first multiplication
  bool mulnodeVisited = false;

  match(stmt,
    std::function<void(const AddNode*,Matcher*)>([&](const AddNode* op,
                                                     Matcher* ctx) {
      if (mulnodeVisited) {
        isEinsum = false;
        return;
      }
      ctx->match(op->a);
      ctx->match(op->b);
    }),
    std::function<void(const SubNode*,Matcher*)>([&](const SubNode* op,
                                                     Matcher* ctx) {
      if (mulnodeVisited) {
        isEinsum = false;
        return;
      }
      ctx->match(op->a);
      ctx->match(op->b);
    }),
    std::function<void(const MulNode*,Matcher*)>([&](const MulNode* op,
                                                     Matcher* ctx) {
      bool topMulNode = !mulnodeVisited;
      mulnodeVisited = true;
      ctx->match(op->a);
      ctx->match(op->b);
      if (topMulNode) {
        mulnodeVisited = false;
      }
    }),
    std::function<void(const BinaryExprNode*)>([&](const BinaryExprNode* op) {
      isEinsum = false;
    }),
    std::function<void(const ReductionNode*)>([&](const ReductionNode* op) {
      isEinsum = false;
    })
  );
  return isEinsum;
}

bool isReductionNotation(const IndexStmt& stmt) {
  if (!isa<Assignment>(stmt)) {
    return false;
  }

  // Reduction notation until proved otherwise
  bool isReduction = true;

  util::ScopedMap<IndexVar,int> boundVars;
  for (auto& var : to<Assignment>(stmt).getFreeVars()) {
    boundVars.insert({var,0});
  }

  match(stmt,
    std::function<void(const ReductionNode*,Matcher*)>([&](
        const ReductionNode* op, Matcher* ctx) {
      boundVars.scope();
      boundVars.insert({op->var,0});
      ctx->match(op->a);
      boundVars.unscope();
    }),
    std::function<void(const AccessNode*)>([&](const AccessNode* op) {
      for (auto& var : op->indexVars) {
        if (!boundVars.contains(var)) {
          isReduction = false;  // Unbound variable
        }
      }
    })
  );
  return isReduction;
}

Assignment makeReductionNotation(const Assignment& assignment) {
  IndexExpr expr = assignment.getRhs();
  std::vector<IndexVar> free = assignment.getLhs().getIndexVars();
  if (!isEinsumNotation(assignment)) {
    return assignment;
  }

  struct Einsum : IndexNotationRewriter {
    Einsum(const std::vector<IndexVar>& free) : free(free.begin(), free.end()){}

    std::set<IndexVar> free;
    bool onlyOneTerm;

    IndexExpr addReductions(IndexExpr expr) {
      auto vars = getIndexVars(expr);
      for (auto& var : util::reverse(vars)) {
        if (!util::contains(free, var)) {
          expr = sum(var,expr);
        }
      }
      return expr;
    }

    IndexExpr einsum(const IndexExpr& expr) {
      onlyOneTerm = true;
      IndexExpr einsumexpr = rewrite(expr);

      if (onlyOneTerm) {
        einsumexpr = addReductions(einsumexpr);
      }

      return einsumexpr;
    }

    using IndexNotationRewriter::visit;

    void visit(const AddNode* op) {
      // Sum every reduction variables over each term
      onlyOneTerm = false;

      IndexExpr a = addReductions(op->a);
      IndexExpr b = addReductions(op->b);
      if (a == op->a && b == op->b) {
        expr = op;
      }
      else {
        expr = new AddNode(a, b);
      }
    }

    void visit(const SubNode* op) {
      // Sum every reduction variables over each term
      onlyOneTerm = false;

      IndexExpr a = addReductions(op->a);
      IndexExpr b = addReductions(op->b);
      if (a == op->a && b == op->b) {
        expr = op;
      }
      else {
        expr = new SubNode(a, b);
      }
    }
  };
  return Assignment(assignment.getLhs(), Einsum(free).einsum(expr),
                    assignment.getOp());
}

IndexStmt makeReductionNotation(const IndexStmt& s) {
  taco_iassert(isEinsumNotation(s));
  return makeReductionNotation(to<Assignment>(s));
}

}
