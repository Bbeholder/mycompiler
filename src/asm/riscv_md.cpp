/*****************************************************
 *  Implementation of RiscvDesc.
 *
 */

#include "asm/riscv_md.hpp"
#include "3rdparty/set.hpp"
#include "asm/offset_counter.hpp"
#include "asm/riscv_frame_manager.hpp"
#include "config.hpp"
#include "options.hpp"
#include "scope/scope.hpp"
#include "symb/symbol.hpp"
#include "tac/flow_graph.hpp"
#include "tac/tac.hpp"

#include <cstring>
#include <iomanip>
#include <sstream>

using namespace mind::assembly;
using namespace mind::tac;
using namespace mind::util;
using namespace mind;

// declaration of empty string
#define EMPTY_STR std::string()
#define WORD_SIZE 4

/* Constructor of RiscvReg.
 *
 * PARAMETERS:
 *   reg_name   - name of this register
 *   is_general - whether this register is a general-purpose one
 */
RiscvReg::RiscvReg(const char *reg_name, bool is_general) {
    name = reg_name;
    dirty = false;
    var = NULL;
    general = is_general;
}

/* Constructor of RiscvDesc.
 *
 */
RiscvDesc::RiscvDesc(void) {
    // {GLOBAL, LOCAL, PARAMETER}
    // Actually, we only use the parameter offset counter,
    // other two options are remained for extension
    int start[3] = {0, 0, 0}; 
    int dir[3] = {+1, -1, +1};
    _counter = new OffsetCounter(start, dir);

    // initializes the register vector
    // (we regard all general-purpose registers as caller-saved, which is
    // different from the Riscv specification)
    _reg[RiscvReg::ZERO] = new RiscvReg("zero", false); // zero
    _reg[RiscvReg::RA] = new RiscvReg("ra", false);     // return address
    _reg[RiscvReg::SP] = new RiscvReg("sp", false);     // stack pointer
    _reg[RiscvReg::GP] = new RiscvReg("gp", false);     // global pointer
    _reg[RiscvReg::TP] = new RiscvReg("tp", false);     // thread pointer
    _reg[RiscvReg::T0] = new RiscvReg("t0", true);
    _reg[RiscvReg::T1] = new RiscvReg("t1", true);
    _reg[RiscvReg::T2] = new RiscvReg("t2", true);
    _reg[RiscvReg::T3] = new RiscvReg("t3", true);
    _reg[RiscvReg::T4] = new RiscvReg("t4", true);
    _reg[RiscvReg::T5] = new RiscvReg("t5", true);
    _reg[RiscvReg::T6] = new RiscvReg("t6", true);
    _reg[RiscvReg::FP] = new RiscvReg("fp", false); // frame pointer
    _reg[RiscvReg::S1] = new RiscvReg("s1", true);
    _reg[RiscvReg::S2] = new RiscvReg("s2", true);
    _reg[RiscvReg::S3] = new RiscvReg("s3", true);
    _reg[RiscvReg::S4] = new RiscvReg("s4", true);
    _reg[RiscvReg::S5] = new RiscvReg("s5", true);
    _reg[RiscvReg::S6] = new RiscvReg("s6", true);
    _reg[RiscvReg::S7] = new RiscvReg("s7", true);
    _reg[RiscvReg::S8] = new RiscvReg("s8", true);
    _reg[RiscvReg::S9] = new RiscvReg("s9", true);
    _reg[RiscvReg::S10] = new RiscvReg("s10", true);
    _reg[RiscvReg::S11] = new RiscvReg("s11", true);
    _reg[RiscvReg::A0] = new RiscvReg("a0", false); // argument, return value
    _reg[RiscvReg::A1] = new RiscvReg("a1", false); // argument, return value
    _reg[RiscvReg::A2] = new RiscvReg("a2", false); // argument
    _reg[RiscvReg::A3] = new RiscvReg("a3", false); // argument
    _reg[RiscvReg::A4] = new RiscvReg("a4", false); // argument
    _reg[RiscvReg::A5] = new RiscvReg("a5", false); // argument
    _reg[RiscvReg::A6] = new RiscvReg("a6", false); // argument
    _reg[RiscvReg::A7] = new RiscvReg("a7", false); // argument

    _lastUsedReg = 0;
    _label_counter = 0;
}

/* Gets the offset counter for this machine.
 *
 * RETURNS:
 *   the offset counter for Riscv
 */
OffsetCounter *RiscvDesc::getOffsetCounter(void) { return _counter; }

/* 将给定的list转换为code和output
 *
 * PARAMETERS:
 *   ps    - the Piece list
 *   os    - the output stream
 */
void RiscvDesc::emitPieces(scope::GlobalScope *gscope, Piece *ps,
                           std::ostream &os) {

    _result = &os;
    // output to .data and .bss segment
    std::ostringstream _data, _bss;

    if (Option::getLevel() == Option::ASMGEN) {
        // program preamble
        emit(EMPTY_STR, ".text", NULL);
        emit(EMPTY_STR, ".globl main", NULL);
        emit(EMPTY_STR, ".align 2", NULL);
    }
    // translates node by node

    while (NULL != ps) {
        switch (ps->kind) {
        case Piece::FUNCTY:
            emitFuncty(ps->as.functy);
            break;
        case Piece::GLOBAL:
            emit(EMPTY_STR, ".data", NULL);
            emit(EMPTY_STR, ((std::string)(".global ") + ps->as.globalVar->name).c_str(), NULL);
            emit(EMPTY_STR, ((std::string)(".size ") + ps->as.globalVar->name + (std::string)(", ") + std::to_string(ps->as.globalVar->size)).c_str(), NULL);
            emit(ps->as.globalVar->name.c_str(), NULL, NULL);
            if(ps->as.globalVar->value != 0){
                emit(EMPTY_STR, ((std::string)(".word ") + std::to_string(ps->as.globalVar->value)).c_str(), NULL);
            }
            else emit(EMPTY_STR, ((std::string)(".zero ") + std::to_string(ps->as.globalVar->size)).c_str(), NULL);
            break;

        default:
            mind_assert(false); // unreachable
            break;
        }

        ps = ps->next;
    }
}

/* Allocates a new label (for a basic block).
 *
 * RETURNS:
 *   a new label guaranteed to be non-conflict with the existing ones
 */
const char *RiscvDesc::getNewLabel(void) {
    mind_assert(_label_counter < 10000);

    char *buf = new char[10];
    std::sprintf(buf, "__LL%d", _label_counter++);

    return buf;
}

/* Translates a single basic block into Riscv instructions.
 * 将基本块翻译为riscv语句
 * PARAMETERS:
 *   b     - the basic block to translate
 *   g     - the control-flow graph
 * RETURNS:
 *   the Riscv instruction sequence of this basic block
 */
RiscvInstr *RiscvDesc::prepareSingleChain(BasicBlock *b, FlowGraph *g) {
    RiscvInstr leading;
    int r0;

    _tail = &leading;
    for (Tac *t = b->tac_chain; t != NULL; t = t->next)
        emitTac(t);

    switch (b->end_kind) {
    case BasicBlock::BY_JUMP:
        spillDirtyRegs(b->LiveOut);
        addInstr(RiscvInstr::J, NULL, NULL, NULL, 0,
                 std::string(g->getBlock(b->next[0])->entry_label), NULL);
        // "B" for "branch"
        break;

    case BasicBlock::BY_JZERO:
        r0 = getRegForRead(b->var, 0, b->LiveOut);
        spillDirtyRegs(b->LiveOut);
        // uses "branch if equal to zero" instruction
        addInstr(RiscvInstr::BEQZ, _reg[r0], NULL, NULL, 0,
                 std::string(g->getBlock(b->next[0])->entry_label), NULL);
        addInstr(RiscvInstr::J, NULL, NULL, NULL, 0,
                 std::string(g->getBlock(b->next[1])->entry_label), NULL);
        break;

    case BasicBlock::BY_RETURN:
        r0 = getRegForRead(b->var, 0, b->LiveOut);
        spillDirtyRegs(b->LiveOut); // just to deattach all temporary variables
        addInstr(RiscvInstr::MOVE, _reg[RiscvReg::A0], _reg[r0], NULL, 0,
                 EMPTY_STR, NULL);
        addInstr(RiscvInstr::MOVE, _reg[RiscvReg::SP], _reg[RiscvReg::FP], NULL,
                 0, EMPTY_STR, NULL);
        addInstr(RiscvInstr::LW, _reg[RiscvReg::RA], _reg[RiscvReg::FP], NULL,
                 -4, EMPTY_STR, NULL);
        addInstr(RiscvInstr::LW, _reg[RiscvReg::FP], _reg[RiscvReg::FP], NULL,
                 -8, EMPTY_STR, NULL);
        addInstr(RiscvInstr::RET, NULL, NULL, NULL, 0, EMPTY_STR, NULL);
        break;

    default:
        mind_assert(false); // unreachable
    }
    _tail = NULL;
    return leading.next;
}

/* Translates a single TAC into Riscv instructions (and records the result.
 * 将三元式翻译成riscv语句
 * PARAMETERS:
 *   t     - the TAC to translate
 * SIDE-EFFECT:
 *   modifies the "_tail" field
 */
void RiscvDesc::emitTac(Tac *t) {
    std::ostringstream oss;
    t->dump(oss);
    addInstr(RiscvInstr::COMMENT, NULL, NULL, NULL, 0, EMPTY_STR, oss.str().c_str() + 4);

    switch (t->op_code) {
    case Tac::LOAD_IMM4:
        emitLoadImm4Tac(t);
        break;
    /*按位取反*/
    case Tac::BNOT:
        emitUnaryTac(RiscvInstr::NOT, t);
        break;
    /*逻辑非*/
    case Tac::LNOT:
        emitUnaryTac(RiscvInstr::SEQZ, t);
        break;
    case Tac::NEG:
        emitUnaryTac(RiscvInstr::NEG, t);
        break;
    /*等于（做减法为0）*/
    case Tac::EQU:
        emitBinaryTac(RiscvInstr::SUB, t);
        t->op1 = t->op0;
        emitUnaryTac(RiscvInstr::SEQZ, t);
        break;
    /*不等于（做减法不为0）*/
    case Tac::NEQ:
        emitBinaryTac(RiscvInstr::SUB, t);
        t->op1 = t->op0;
        emitUnaryTac(RiscvInstr::SNEZ, t);
        break;
    /*大于等于*/    
    case Tac::GEQ:
        emitBinaryTac(RiscvInstr::SLT, t);
        t->op1 = t->op0;
        emitUnaryTac(RiscvInstr::SEQZ, t);
        break;
    /*大于*/
    case Tac::GTR:
        emitBinaryTac(RiscvInstr::SGT, t);
        break;
    /*与(按位与变逻辑与)*/
    case Tac::LAND:
        
        emitLandTac(RiscvInstr::MUL, t);
        t->op1 = t->op0;
        // emitUnaryTac(RiscvInstr::SNEZ, t);
        break;
    /*或(按位或变逻辑或)*/
    case Tac::LOR:
        emitBinaryTac(RiscvInstr::OR, t);
        t->op1 = t->op0;
        emitUnaryTac(RiscvInstr::SNEZ, t);
        break;
    /*加*/
    case Tac::ADD:
        emitBinaryTac(RiscvInstr::ADD, t);
        break;
    /*减*/
    case Tac::SUB:
        emitBinaryTac(RiscvInstr::SUB, t);
        break;
    /*乘*/
    case Tac::MUL:
        emitBinaryTac(RiscvInstr::MUL, t);
        break;
    /*除*/
    case Tac::DIV:
        emitBinaryTac(RiscvInstr::DIV, t);
        break;
    /*模*/
    case Tac::MOD:
        emitBinaryTac(RiscvInstr::REM, t);
        break;
    /*赋值*/
    case Tac::ASSIGN:
        emitUnaryTac(RiscvInstr::MOVE, t);
        break;
    /*进栈*/
    case Tac::PUSH:
        emitPushTac(t);
        break;
    /*出栈*/
    case Tac::POP:
        addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, 4, EMPTY_STR, NULL);
        break;
    /*调用*/
    case Tac::CALL: 
        emitCallTac(t);
        break;
    /*函数下一个参数*/
    case Tac::PARAM:
        break;
    /*加载 symbol 符号所代表的地址*/
    case Tac::LOAD_SYMBOL:
        emitLoadSymbolTac(t);
        break;
    /*载与该地址相差 offset 个偏移的内存地址中的数据*/
    case Tac::LOAD:
        emitLoadTac(t);
        break;
    case Tac::STORE:
        emitStoreTac(t);
        break;
    /*分配 size 字节的内存，并返回内存首地址*/
    case Tac::ALLOC:
        emitAllocTac(t);
        break;
    default:
        printf("%d ????\n", t->op_code);
        mind_assert(false); // should not appear inside a basic block
    }
}

void RiscvDesc::emitCallTac(Tac *t) {

    Set<Temp>* liveness = t->LiveOut->clone();

    {
        int cnt = 0;
        for(auto temp : *liveness){
            cnt -= 4;
            int r1 = getRegForRead(temp, 0, t->LiveOut);
            addInstr(RiscvInstr::SW,  _reg[r1], _reg[RiscvReg::SP], NULL, cnt, EMPTY_STR, NULL);
        }
        addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, cnt, EMPTY_STR, NULL);
    }

    int count = 0;
    for(Tac *it = t->prev; it != NULL && it->op_code == Tac::PARAM; it = it->prev) count += 4;

    if(count > 0){
        addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, -count, EMPTY_STR, NULL);
        int cnt = count;
        for(Tac *it = t->prev; it != NULL && it->op_code == Tac::PARAM; it = it->prev){
            cnt -= 4;
            int r1 = getRegForRead(it->op0.var, 0, it->LiveOut);
            addInstr(RiscvInstr::SW,  _reg[r1], _reg[RiscvReg::SP], NULL, cnt, EMPTY_STR, NULL);
        }
    }
    count += liveness->size() * 4;

    addInstr(RiscvInstr::CALL, NULL, NULL, NULL, 0, std::string("_") + t->op1.label->str_form, NULL);
    {
        int cnt = 0;
        addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, count, EMPTY_STR, NULL);
        for(auto temp: *liveness){
            cnt -= 4;
            int r1 = getRegForWrite(temp, 0, 0, t->LiveOut);
            addInstr(RiscvInstr::LW,  _reg[r1], _reg[RiscvReg::SP], NULL, cnt, EMPTY_STR, NULL);
        }
    }
    
    int r0 = getRegForWrite(t->op0.var, 0, 0, t->LiveOut);
    addInstr(RiscvInstr::MOVE, _reg[r0], _reg[RiscvReg::A0], NULL, 0, EMPTY_STR, NULL);
}


void RiscvDesc::emitPushTac(Tac *t) {
    int r1 = getRegForRead(t->op0.var, 0, t->LiveOut);
    addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, -4, EMPTY_STR, NULL);
    addInstr(RiscvInstr::SW,  _reg[r1], _reg[RiscvReg::SP], NULL, 0, EMPTY_STR, NULL);
}

void RiscvDesc::emitAllocTac(Tac *t) {
    if (!t->LiveOut->contains(t->op0.var))
        return;
    int r0 = getRegForWrite(t->op0.var, 0, 0, t->LiveOut);
    addInstr(RiscvInstr::ADDI, _reg[RiscvReg::SP], _reg[RiscvReg::SP], NULL, -t->op1.size, EMPTY_STR, NULL);
    addInstr(RiscvInstr::MOVE, _reg[r0], _reg[RiscvReg::SP], NULL, 0, EMPTY_STR, NULL);
}

/* Translates a LoadImm4 TAC into Riscv instructions.
 *
 * PARAMETERS:
 *   t     - the LoadImm4 TAC
 */
void RiscvDesc::emitLoadImm4Tac(Tac *t) {
    // eliminates useless assignments
    if (!t->LiveOut->contains(t->op0.var))
        return;

    // uses "load immediate number" instruction
    int r0 = getRegForWrite(t->op0.var, 0, 0, t->LiveOut);
    addInstr(RiscvInstr::LI, _reg[r0], NULL, NULL, t->op1.ival, EMPTY_STR,
             NULL);
}

void RiscvDesc::emitLoadSymbolTac(Tac *t) {
    if (!t->LiveOut->contains(t->op0.var))
        return;
    int r0 = getRegForWrite(t->op0.var, 0, 0, t->LiveOut);
    addInstr(RiscvInstr::LA, _reg[r0], NULL, NULL, 0, t->op1.name,
             NULL);
}

void RiscvDesc::emitLoadTac(Tac *t) {
    if (!t->LiveOut->contains(t->op0.var))
        return;
    int r1 = getRegForRead(t->op1.var, 0, t->LiveOut);
    int r0 = getRegForWrite(t->op0.var, r1, 0, t->LiveOut);
    addInstr(RiscvInstr::LW, _reg[r0], _reg[r1], NULL, t->op1.offset, EMPTY_STR,
             NULL);
}
void RiscvDesc::emitStoreTac(Tac *t) {

    int r0 = getRegForRead(t->op0.var, 0, t->LiveOut);
    int r1 = getRegForRead(t->op1.var, r0, t->LiveOut);
    addInstr(RiscvInstr::SW, _reg[r0], _reg[r1], NULL, t->op1.offset, EMPTY_STR,
             NULL);
}
/* Translates a Unary TAC into Riscv instructions.
 *
 * PARAMETERS:
 *   t     - the Unary TAC
 */
void RiscvDesc::emitUnaryTac(RiscvInstr::OpCode op, Tac *t) {
    // eliminates useless assignments
    if (!t->LiveOut->contains(t->op0.var))
        return;

    int r1 = getRegForRead(t->op1.var, 0, t->LiveOut);
    int r0 = getRegForWrite(t->op0.var, r1, 0, t->LiveOut);

    addInstr(op, _reg[r0], _reg[r1], NULL, 0, EMPTY_STR, NULL);
}

void RiscvDesc::emitLandTac(RiscvInstr::OpCode op, Tac *t) {
    // eliminates useless assignments
    if (!t->LiveOut->contains(t->op0.var))
        return;
    int r1 = getRegForRead(t->op1.var, 0, t->LiveOut);
    int r2 = getRegForRead(t->op2.var, r1, t->LiveOut);
    int r0 = getRegForWrite(t->op0.var, r1, r2, t->LiveOut);
    addInstr(RiscvInstr::MUL, _reg[r0], _reg[r1],  _reg[r2], 0, EMPTY_STR, NULL);
    addInstr(RiscvInstr::SNEZ, _reg[r0], _reg[r0], NULL, 0, EMPTY_STR, NULL);

}

/* Translates a Binary TAC into Riscv instructions.
 *
 * PARAMETERS:
 *   t     - the Binary TAC
 */
void RiscvDesc::emitBinaryTac(RiscvInstr::OpCode op, Tac *t) {
    // eliminates useless assignments
    if (!t->LiveOut->contains(t->op0.var))
        return;

    Set<Temp>* liveness = t->LiveOut->clone();
    liveness->add(t->op1.var);
    liveness->add(t->op2.var);
    int r1 = getRegForRead(t->op1.var, 0, liveness);
    int r2 = getRegForRead(t->op2.var, r1, liveness);
    int r0 = getRegForWrite(t->op0.var, r1, r2, liveness);

    addInstr(op, _reg[r0], _reg[r1], _reg[r2], 0, EMPTY_STR, NULL);
}

/* Outputs a single instruction line.
 *
 * PARAMETERS:
 *   label   - label of this line
 *   body    - instruction
 *   comment - comment of this line
 */
void RiscvDesc::emit(std::string label, const char *body, const char *comment) {
    std::ostream &os(*_result);

    if ((NULL != comment) && (label.empty()) && (NULL == body)) {
        os << "                                  # " << comment;

    } else {
        if (!label.empty())
            os << label << std::left << std::setw(40 - label.length()) << ":";
        else if (NULL != body)
            os << "          " << std::left << std::setw(30) << body;

        if (NULL != comment)
            os << "# " << comment;
    }

    os << std::endl;
}

/* Translates a "Functy" object into assembly code and output.
 *
 * PARAMETERS:
 *   f     - the Functy object
 */
void RiscvDesc::emitFuncty(Functy f) {
    mind_assert(NULL != f);

    _frame = new RiscvStackFrameManager(-3 * WORD_SIZE);
    FlowGraph *g = FlowGraph::makeGraph(f);
    g->simplify();        // simple optimization
    g->analyzeLiveness(); // computes LiveOut set of the basic blocks

    for (FlowGraph::iterator it = g->begin(); it != g->end(); ++it) {
        // all variables shared between basic blocks should be reserved
        Set<Temp> *liveout = (*it)->LiveOut;
        for (Set<Temp>::iterator sit = liveout->begin(); sit != liveout->end();
             ++sit) {
            _frame->reserve(*sit);
        }
        (*it)->entry_label = getNewLabel(); // adds entry label of a basic block
    }
    for (FlowGraph::iterator it = g->begin(); it != g->end(); ++it) {
        BasicBlock *b = *it;
        b->analyzeLiveness(); // computes LiveOut set of every TAC
        _frame->reset();
        // translates the TAC sequences of this block
        b->instr_chain = prepareSingleChain(b, g);
        if (Option::doOptimize()) // use "-O" option to enable optimization
            simplePeephole((RiscvInstr *)b->instr_chain);
        b->mark = 0; // clears the marks (for the next step)
    }
    if (Option::getLevel() == Option::DATAFLOW) {
        std::cout << "Control-flow Graph of " << f->entry << ":" << std::endl;
        g->dump(std::cout);
        // TO STUDENTS: You might not want to get lots of outputs when
        // debugging.
        //              You can enable the following line so that the program
        //              will terminate after the first Functy is done.
        // std::exit(0);
        return;
    }

    mind_assert(!f->entry->str_form.empty()); // this assertion should hold for every Functy
    // outputs the header of a function
    emitProlog(f->entry, _frame->getStackFrameSize());
    // chains up the assembly code of every basic block and output.
    //
    // ``A trace is a sequence of statements that could be consecutively
    //   executed during the execution of the program. It can include
    //   conditional branches.''
    //           -- Modern Compiler Implementation in Java (the ``Tiger Book'')
    for (FlowGraph::iterator it = g->begin(); it != g->end(); ++it)
        emitTrace(*it, g);
}

/* Outputs the leading code of a function.
 * 栈桢建立
 * PARAMETERS:
 *   entry_label - the function label
 *   frame_size  - stack-frame size of this function
 * NOTE:
 *   the prolog code is used to save context and establish the stack frame.
 */
void RiscvDesc::emitProlog(Label entry_label, int frame_size) {
    std::ostringstream oss;

    emit(EMPTY_STR, NULL, NULL); // an empty line
    emit(EMPTY_STR, ".text", NULL);
    if (entry_label->str_form == "main") {
        oss << "main";
    } else {
        oss << entry_label;
    }
    emit(oss.str(), NULL, "function entry"); // marks the function entry label
    oss.str("");
    // saves old context
    emit(EMPTY_STR, "sw    ra, -4(sp)", NULL); // saves old frame pointer
    emit(EMPTY_STR, "sw    fp, -8(sp)", NULL); // saves return address
    // establishes new stack frame (new context)
    emit(EMPTY_STR, "mv    fp, sp", NULL);
    oss << "addi  sp, sp, -" << (frame_size + 2 * WORD_SIZE); // 2 WORD's for old $fp and $ra
    emit(EMPTY_STR, oss.str().c_str(), NULL);
}

/* Outputs a single instruction.
 *
 * PARAMETERS:
 *   i     - the instruction to output
 */
void RiscvDesc::emitInstr(RiscvInstr *i) {
    if (i->cancelled)
        return;
    std::ostringstream oss;
    oss << std::left << std::setw(6);

    switch (i->op_code) {
    case RiscvInstr::COMMENT:
        emit(EMPTY_STR, NULL, i->comment);
        return;

    case RiscvInstr::LI:
        oss << "li" << i->r0->name << ", " << i->i;
        break;

    case RiscvInstr::NEG:
        oss << "neg" << i->r0->name << ", " << i->r1->name;
        break;
    /*按位取反*/
    case RiscvInstr::NOT:
        oss << "not" << i->r0->name << ", " << i->r1->name;//not t1,t2
        break;
    /*逻辑非*/
    case RiscvInstr::SEQZ:
        oss << "seqz" << i->r0->name << ", " << i->r1->name;//seqz t1,t2
        break;
    /*等于0*/
    case RiscvInstr::SNEZ:
        oss << "snez" << i->r0->name << ", " << i->r1->name;
        break;
    /*与*/
    case RiscvInstr::AND:
        oss << "and" << i->r0->name << ", " << i->r1->name << "," << i->r2->name;
        break;
    /*或*/
    case RiscvInstr::OR:
        oss << "or" << i->r0->name << ", " << i->r1->name << "," << i->r2->name;
        break;
    case RiscvInstr::MOVE:
        oss << "mv" << i->r0->name << ", " << i->r1->name;
        break;

    case RiscvInstr::LW:
        oss << "lw" << i->r0->name << ", " << i->i << "(" << i->r1->name << ")";
        break;

    case RiscvInstr::SW:
        oss << "sw" << i->r0->name << ", " << i->i << "(" << i->r1->name << ")";
        break;
    
    case RiscvInstr::RET:
        oss << "ret";
        break;
    /*小于*/
    case RiscvInstr::SLT:
        oss << "slt" << i->r0->name << ", " << i->r1->name << "," << i->r2->name;
        break;
    /*小于（无符号）*/
    case RiscvInstr::SLTU:
        oss << "sltu" << i->r0->name << ", " << i->r1->name << "," << i->r2->name;
        break;
    /*大于*/
    case RiscvInstr::SGT:
        oss << "sgt" << i->r0->name << ", " << i->r1->name << "," << i->r2->name;
        break;
    /*加*/
    case RiscvInstr::ADD:
        oss << "add" << i->r0->name << ", " << i->r1->name << ", " << i->r2->name;
        break;
    case RiscvInstr::ADDI:
        oss << "addi" << i->r0->name << ", " << i->r1->name << ", "<< i->i;
        break;
    /*减*/
    case RiscvInstr::SUB:
        oss << "sub" << i->r0->name << ", " << i->r1->name << ", " << i->r2->name;//sub t1,t2,t3
        break;
    /*乘*/
    case RiscvInstr::MUL:
        oss << "mul" << i->r0->name << ", " << i->r1->name << ", " << i->r2->name;//mul t1,t2,t3
        break;
    /*除*/
    case RiscvInstr::DIV:
        oss << "div" << i->r0->name << ", " << i->r1->name << ", " << i->r2->name;//div t1,t2,t3
        break;
    /*模*/
    case RiscvInstr::REM:
        oss << "rem" << i->r0->name << ", " << i->r1->name << ", " << i->r2->name;//rem t1,t2,t3
        break;
    case RiscvInstr::BEQZ:
        oss << "beqz" << i->r0->name << ", " << i->l;
        break;

    case RiscvInstr::J:
        oss << "j" << i->l;
        break;
    
    case RiscvInstr::CALL:
        oss << "call" << i->l;
        break;

    case RiscvInstr::LA:
        oss << "la" << i->r0->name << ", " << i->l;
        break;
    
    case RiscvInstr::XOR:
        oss << "xor" << i->r0->name << ", " << i->r0->name << "," <<  "0x1";
        break;
    
    
    default:
        mind_assert(false); // other instructions not supported
    }

    emit(EMPTY_STR, oss.str().c_str(), i->comment);
}

/* Outputs a "trace" (see also: RiscvDesc::emitFuncty).
 * 栈桢销毁
 * PARAMETERS:
 *   b     - the leading basic block of this trace
 *   g     - the control-flow graph
 * NOTE:
 *   we just do a simple depth-first search against the CFG
 */
void RiscvDesc::emitTrace(BasicBlock *b, FlowGraph *g) {
    // a trace is a series of consecutive basic blocks
    if (b->mark > 0)
        return;
    b->mark = 1;
    emit(std::string(b->entry_label), NULL, NULL);

    RiscvInstr *i = (RiscvInstr *)b->instr_chain;
    while (NULL != i) {
        emitInstr(i);
        i = i->next;
    }
    switch (b->end_kind) {
    case BasicBlock::BY_JUMP:
        emitTrace(g->getBlock(b->next[0]), g);
        break;

    case BasicBlock::BY_JZERO:
        emitTrace(g->getBlock(b->next[1]), g);
        break;

    case BasicBlock::BY_RETURN:
        break;

    default:
        mind_assert(false); // unreachable
    }
}

/* Appends an instruction line to "_tail". (internal helper function)
 * 把指令放入指令序列
 * PARAMETERS:
 *   op_code - operation code
 *   r0      - the first register operand (if any)
 *   r1      - the second register operand (if any)
 *   r2      - the third register operand (if any)
 *   i       - immediate number or offset (if any)
 *   l       - label operand (for LA and jumps)
 *   cmt     - comment of this line
 */
void RiscvDesc::addInstr(RiscvInstr::OpCode op_code, RiscvReg *r0, RiscvReg *r1,
                         RiscvReg *r2, int i, std::string l, const char *cmt) {
    mind_assert(NULL != _tail);

    // we should eliminate all the comments when doing optimization
    if (Option::doOptimize() && (RiscvInstr::COMMENT == op_code))
        return;
    _tail->next = new RiscvInstr();
    _tail = _tail->next;
    _tail->op_code = op_code;
    _tail->r0 = r0;
    _tail->r1 = r1;
    _tail->r2 = r2;
    _tail->i = i;
    _tail->l = l;
    _tail->comment = cmt;
}


/******************** a simple peephole optimizer 窥孔优化*********************/

/* Performs a peephole optimization pass to the instruction sequence.
 *
 * PARAMETERS:
 *   iseq  - the instruction sequence to optimize
 */
void RiscvDesc::simplePeephole(RiscvInstr *iseq) {
    // if you are interested in peephole optimization, you can implement here
    // of course, beyond our requirements
    
}

/******************* REGISTER ALLOCATOR 寄存器分配 ***********************/

/* Acquires a register to read some variable.
 *  为源操作数对应的临时变量分配寄存器
 *
 * PARAMETERS:
 *   v      - the variable to read
 *   avoid1 - the register which should not be selected
 *   live   - current liveness set
 * RETURNS:
 *   number of the register containing the content of v
 */
int RiscvDesc::getRegForRead(Temp v, int avoid1, LiveSet *live) {
    std::ostringstream oss;

    int i = lookupReg(v);

    if (i < 0) {
        // we will load the content into some register
        i = lookupReg(NULL);

        if (i < 0) {
            i = selectRegToSpill(avoid1, RiscvReg::ZERO, live);
            spillReg(i, live);
        }

        _reg[i]->var = v;

        if (v->is_offset_fixed) {
            RiscvReg *base = _reg[RiscvReg::FP];
            oss << "load " << v << " from (" << base->name
                << (v->offset < 0 ? "" : "+") << v->offset << ") into "
                << _reg[i]->name;
            addInstr(RiscvInstr::LW, _reg[i], base, NULL, v->offset, EMPTY_STR,
                     oss.str().c_str());

        } else {
            oss << "initialize " << v << " with 0";
            addInstr(RiscvInstr::MOVE, _reg[i], _reg[RiscvReg::ZERO], NULL, 0,
                     EMPTY_STR, oss.str().c_str());
        }
        _reg[i]->dirty = false;
    }

    return i;
}

/* Acquires a register to write some variable.
 * 为目标操作数对应的临时变量分配寄存器
 * PARAMETERS:
 *   v      - the variable to write
 *   avoid1 - the register which should not be selected
 *   avoid2 - the same as "avoid1"
 *   live   - the current liveness set
 * RETURNS:
 *   number of the register which can be safely written to
 */
int RiscvDesc::getRegForWrite(Temp v, int avoid1, int avoid2, LiveSet *live) {
    if (NULL == v || !live->contains(v))
        return RiscvReg::ZERO;

    int i = lookupReg(v);

    if (i < 0) {
        i = lookupReg(NULL);

        if (i < 0) {
            i = selectRegToSpill(avoid1, avoid2, live);
            spillReg(i, live);
        }
        _reg[i]->var = v;
    }

    _reg[i]->dirty = true;

    return i;
}

/* Spills a specified register (into memory, i.e. into the stack-frame).
 * 溢出指定寄存器
 * PARAMETERS:
 *   i     - number of the register to spill
 *   live  - the current liveness set
 * NOTE:
 *   if the variable contained in $i is no longer alive,
 *   we don't save it into memory.
 */
void RiscvDesc::spillReg(int i, LiveSet *live) {
    std::ostringstream oss;

    Temp v = _reg[i]->var;

    if ((NULL != v) && _reg[i]->dirty && live->contains(v)) {
        RiscvReg *base = _reg[RiscvReg::FP];

        if (!v->is_offset_fixed) {
            _frame->getSlotToWrite(v, live);
        }

        oss << "spill " << v << " from " << _reg[i]->name << " to ("
            << base->name << (v->offset < 0 ? "" : "+") << v->offset << ")";
        addInstr(RiscvInstr::SW, _reg[i], base, NULL, v->offset, EMPTY_STR,
                 oss.str().c_str());
    }

    _reg[i]->var = NULL;
    _reg[i]->dirty = false;
}

/* Spills all dirty (and alive) registers into memory.
 * 将所有脏寄存器和活跃寄存器溢出到内存
 * PARAMETERS:
 *   live  - the current liveness set
 */
void RiscvDesc::spillDirtyRegs(LiveSet *live) {
    int i;
    // determines whether we should spill the registers
    for (i = 0; i < RiscvReg::TOTAL_NUM; ++i) {
        if ((_reg[i]->var != NULL) && _reg[i]->dirty && live->contains(_reg[i]->var))
            break;

        _reg[i]->var = NULL;
        _reg[i]->dirty = false;
    }

    if (i < RiscvReg::TOTAL_NUM) {
        addInstr(RiscvInstr::COMMENT, NULL, NULL, NULL, 0, EMPTY_STR,
                 "(save modified registers before control flow changes)");

        for (; i < RiscvReg::TOTAL_NUM; ++i)
            spillReg(i, live);
    }
}

/* Looks up a register containing the specified variable.
 * 找已经存了特定变量的寄存器
 * PARAMETERS:
 *   v     - the specified variable
 * RETURNS:
 *   number of the register if found; -1 if not found
 */
int RiscvDesc::lookupReg(tac::Temp v) {
    for (int i = 0; i < RiscvReg::TOTAL_NUM; ++i)
        if (_reg[i]->general && _reg[i]->var == v)
            return i;

    return -1;
}

/* Selects a register to spill into memory (so that it can be released).
 * 找可以溢出到内存的寄存器
 * 简而言之就是找一个不活跃的或者找一个没有被读的，如果都没有就开摆（LRU）
 *
 * PARAMETERS:
 *   avoid1 - the register that should not be selected
 *   avoid2 - the same as avoid1
 *   live   - the current liveness set
 * RETURNS:
 *   number of the selected register
 */
int RiscvDesc::selectRegToSpill(int avoid1, int avoid2, LiveSet *live) {
    // looks for a "ready" one
    for (int i = 0; i < RiscvReg::TOTAL_NUM; ++i) {
        if (!_reg[i]->general)
            continue;

        if ((i != avoid1) && (i != avoid2) && !live->contains(_reg[i]->var))
            return i;
    }

    // looks for a clean one (so that we could save a "store")
    for (int i = 0; i < RiscvReg::TOTAL_NUM; ++i) {
        if (!_reg[i]->general)
            continue;

        if ((i != avoid1) && (i != avoid2) && !_reg[i]->dirty)
            return i;
    }

    // the worst case: all are live and all are dirty.
    // chooses one register w.r.t a policy similar to the LRU algorithm (random choice)
    do {
        _lastUsedReg = (_lastUsedReg + 1) % RiscvReg::TOTAL_NUM;
    } while ((_lastUsedReg == avoid1) || (_lastUsedReg == avoid2) ||
             !_reg[_lastUsedReg]->general);

    return _lastUsedReg;
}
