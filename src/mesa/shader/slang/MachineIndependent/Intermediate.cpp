//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//

//
// Build the intermediate representation.
//

#include "../Include/ShHandle.h"
#include "localintermediate.h"
#include "QualifierAlive.h"
#include "RemoveTree.h"
#include <float.h>

////////////////////////////////////////////////////////////////////////////
//
// First set of functions are to help build the intermediate representation.
// These functions are not member functions of the nodes.
// They are called from parser productions.
//
/////////////////////////////////////////////////////////////////////////////

//
// Add a terminal node for an identifier in an expression.
//
// Returns the added node.
//
TIntermSymbol* TIntermediate::addSymbol(int id, const TString& name, const TType& type, TSourceLoc line)
{
    TIntermSymbol* node = new TIntermSymbol(id, name, type);
    node->setLine(line);

    return node;
}

//
// Connect two nodes with a new parent that does a binary operation on the nodes.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addBinaryMath(TOperator op, TIntermTyped* left, TIntermTyped* right, TSourceLoc line, TSymbolTable& symbolTable)
{
    switch (op) {
    case EOpLessThan:
    case EOpGreaterThan:
    case EOpLessThanEqual:
    case EOpGreaterThanEqual:
        if (left->getType().isMatrix() || left->getType().isArray() || left->getType().isVector() || left->getType().getBasicType() == EbtStruct) {
            return 0;
        }
        break;
    case EOpLogicalOr:
    case EOpLogicalXor:
    case EOpLogicalAnd:
        if (left->getType().getBasicType() != EbtBool || left->getType().isMatrix() || left->getType().isArray() || left->getType().isVector()) {
            return 0;
        }
        break;
    case EOpAdd:
    case EOpSub:
    case EOpDiv:
    case EOpMul:
        if (left->getType().getBasicType() == EbtStruct || left->getType().getBasicType() == EbtBool)
            return 0;
    default: break; 
    }

    // 
    // First try converting the children to compatible types.
    //

    if (!(left->getType().getStruct() && right->getType().getStruct())) {
        TIntermTyped* child = addConversion(op, left->getType(), right);
        if (child)
            right = child;
        else {
            child = addConversion(op, right->getType(), left);
            if (child)
                left = child;
            else
                return 0;
        }
    } else {
        if (left->getType() != right->getType())
            return 0;
    }


    //
    // Need a new node holding things together then.  Make
    // one and promote it to the right type.
    //
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = right->getLine();
    node->setLine(line);

    node->setLeft(left);
    node->setRight(right);
    if (! node->promote(infoSink))
        return 0;

    TIntermConstantUnion *leftTempConstant = left->getAsConstantUnion();
    TIntermConstantUnion *rightTempConstant = right->getAsConstantUnion();
        
    if (leftTempConstant)
        leftTempConstant = copyConstUnion(left->getAsConstantUnion())->getAsConstantUnion();
    
    if (rightTempConstant)
        rightTempConstant = copyConstUnion(right->getAsConstantUnion())->getAsConstantUnion();
    
    if (right->getType().getQualifier() == EvqConst && left->getType().getQualifier() == EvqConst) {
        if (right->getAsAggregate()) {
            rightTempConstant = changeAggrToTempConst(right->getAsAggregate(), symbolTable, line);    
            if (rightTempConstant->getUnionArrayPointer() == 0)
                return 0;    
        }

        if (left->getAsAggregate()) {
            leftTempConstant = changeAggrToTempConst(left->getAsAggregate(), symbolTable, line);  
            if (leftTempConstant->getUnionArrayPointer() == 0)
                return 0;
        }
    }

    //
    // See if we can fold constants.
    //

    TIntermTyped* typedReturnNode = 0;
    if ( leftTempConstant && rightTempConstant) {
        if (leftTempConstant->getSize() == 1 && rightTempConstant->getSize() > 1)
            typedReturnNode = rightTempConstant->fold(node->getOp(), leftTempConstant, infoSink, false);
        else
            typedReturnNode = leftTempConstant->fold(node->getOp(), rightTempConstant, infoSink, true);

        if (typedReturnNode)
            return typedReturnNode;
        else {
            node->setLeft(leftTempConstant);
            node->setRight(rightTempConstant);
        }
    } else if (leftTempConstant) {
            node->setLeft(copyConstUnion(leftTempConstant));
    } else if (rightTempConstant) {
            node->setRight(rightTempConstant);
    }

    return node;
}

//
// Connect two nodes through an assignment.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addAssign(TOperator op, TIntermTyped* left, TIntermTyped* right, TSourceLoc line)
{
    //
    // Like adding binary math, except the conversion can only go
    // from right to left.
    //
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = left->getLine();
    node->setLine(line);

    if (right->getAsConstantUnion()) { // if the right node of assignment is a TempConstant node, allocate its own new space and remove the pointer to the symbol table value
        right = copyConstUnion(right->getAsConstantUnion()) ;
        if (right == 0)
            return 0;
    }

    TIntermTyped* child = addConversion(op, left->getType(), right);
    if (child == 0)
        return 0;

    node->setLeft(left);
    node->setRight(child);
    if (! node->promote(infoSink))
        return 0;

    return node;
}

//
// Connect two nodes through an index operator, where the left node is the base
// of an array or struct, and the right node is a direct or indirect offset.
//
// Returns the added node.
// The caller should set the type of the returned node.
//
TIntermTyped* TIntermediate::addIndex(TOperator op, TIntermTyped* base, TIntermTyped* index, TSourceLoc line)
{
    TIntermBinary* node = new TIntermBinary(op);
    if (line == 0)
        line = index->getLine();
    node->setLine(line);
    node->setLeft(base);
    node->setRight(index);

    // caller should set the type

    return node;
}

//
// Add one node as the parent of another that it operates on.
//
// Returns the added node.
//
TIntermTyped* TIntermediate::addUnaryMath(TOperator op, TIntermNode* childNode, TSourceLoc line, TSymbolTable& symbolTable)
{
    TIntermUnary* node;
    TIntermTyped* child = childNode->getAsTyped();

    if (child == 0) {
        infoSink.info.message(EPrefixInternalError, "Bad type in AddUnaryMath", line);
        return 0;
    }

    switch (op) {
    case EOpLogicalNot:
        if (child->getType().getBasicType() != EbtBool || child->getType().isMatrix() || child->getType().isArray() || child->getType().isVector()) {
            return 0;
        }
        break;

    case EOpPostIncrement:
    case EOpPreIncrement:
    case EOpPostDecrement:
    case EOpPreDecrement:
    case EOpNegative:
        if (child->getType().getBasicType() == EbtStruct)
            return 0;
    default: break;
    }
    
    //
    // Do we need to promote the operand?
    //
    // Note: Implicit promotions were removed from the language.
    //
    TBasicType newType = EbtVoid;
    switch (op) {
    case EOpConstructInt:   newType = EbtInt;   break;
    case EOpConstructBool:  newType = EbtBool;  break;
    case EOpConstructFloat: newType = EbtFloat; break;
    default: break;
    }

    if (newType != EbtVoid) {
        child = addConversion(op, TType(newType, EvqTemporary, child->getNominalSize(), 
                                                               child->isMatrix(), 
                                                               child->isArray()),
                              child);
        if (child == 0)
            return 0;
    }

    //
    // For constructors, we are now done, it's all in the conversion.
    //
    switch (op) {
    case EOpConstructInt:
    case EOpConstructBool:
    case EOpConstructFloat:
        return child;
    default: break;
    }
    
    if (child->getAsConstantUnion())
        child = copyConstUnion(child->getAsConstantUnion());

    if (child->getAsAggregate() && child->getType().getQualifier() == EvqConst) {
        child = changeAggrToTempConst(child->getAsAggregate(), symbolTable, line);    
        if (child->getAsConstantUnion()->getUnionArrayPointer() == 0)
            return 0;    
    }
    
    TIntermConstantUnion *childTempConstant = child->getAsConstantUnion();
    
    //
    // Make a new node for the operator.
    //
    node = new TIntermUnary(op);
    if (line == 0)
        line = child->getLine();
    node->setLine(line);
    node->setOperand(child);
    
    if (! node->promote(infoSink))
        return 0;

    if (childTempConstant)  {
        TIntermTyped* newChild = childTempConstant->fold(op, 0, infoSink, true);
        
        if (newChild) {
            return newChild;
        } 
    } 

    return node;
}

//
// This is the safe way to change the operator on an aggregate, as it
// does lots of error checking and fixing.  Especially for establishing
// a function call's operation on it's set of parameters.  Sequences
// of instructions are also aggregates, but they just direnctly set
// their operator to EOpSequence.
//
// Returns an aggregate node, which could be the one passed in if
// it was already an aggregate.
//
TIntermAggregate* TIntermediate::setAggregateOperator(TIntermNode* node, TOperator op, TSourceLoc line)
{
    TIntermAggregate* aggNode;

    //
    // Make sure we have an aggregate.  If not turn it into one.
    //
    if (node) {
        aggNode = node->getAsAggregate();
        if (aggNode == 0 || aggNode->getOp() != EOpNull) {
            //
            // Make an aggregate containing this node.
            //
            aggNode = new TIntermAggregate();
            aggNode->getSequence().push_back(node);
            if (line == 0)
                line = node->getLine();
        }
    } else
        aggNode = new TIntermAggregate();

    //
    // Set the operator.
    //
    aggNode->setOperator(op);
    if (line != 0)
        aggNode->setLine(line);

    return aggNode;
}

//
// Convert one type to another.
//
// Returns the node representing the conversion, which could be the same
// node passed in if no conversion was needed.
//
// Return 0 if a conversion can't be done.
//
TIntermTyped* TIntermediate::addConversion(TOperator op, const TType& type, TIntermTyped* node)
{
    //
    // Does the base type allow operation?
    //
    switch (node->getBasicType()) {
    case EbtVoid:
    case EbtSampler1D:
    case EbtSampler2D:
    case EbtSampler3D:
    case EbtSamplerCube:
    case EbtSampler1DShadow:
    case EbtSampler2DShadow:
        return 0;
    default: break;
    }

    //
    // Otherwise, if types are identical, no problem
    //
    if (type == node->getType())
        return node;

    //
    // If one's a structure, then no conversions.
    //
    if (type.getStruct() || node->getType().getStruct())
        return 0;

    TBasicType promoteTo;
    
    switch (op) {
    //
    // Explicit conversions
    //
    case EOpConstructBool:
        promoteTo = EbtBool;
        break;
    case EOpConstructFloat:
        promoteTo = EbtFloat;
        break;
    case EOpConstructInt:
        promoteTo = EbtInt;
        break;
    default:
        //
        // implicit conversions were removed from the language.
        //
        if (type.getBasicType() != node->getType().getBasicType())
            return 0;
        //
        // Size and structure could still differ, but that's
        // handled by operator promotion.
        //
        return node;
    }
    
    //
    // Do conversion.
    //
    bool allConstant = true;
    // check to see if there is an aggregate node
    if (node->getAsAggregate()) {
        if (node->getAsAggregate()->getOp() != EOpFunctionCall) {
            // if the aggregate node is a constructor or a comma operator, look at its children, if they are constant
            // convert them into the right type
        TIntermSequence &sequenceVector = node->getAsAggregate()->getSequence() ;
        for (TIntermSequence::iterator p = sequenceVector.begin(); 
                                    p != sequenceVector.end(); p++) {
            if (!(*p)->getAsTyped()->getAsConstantUnion())
                allConstant = false;
        }
        } else
            allConstant = false;
    }
    if (allConstant && node->getAsAggregate()) {  // we can do the constant folding here as all the nodes of the aggregate are const
        TIntermSequence &sequenceVector = node->getAsAggregate()->getSequence() ;
        for (TIntermSequence::iterator p = sequenceVector.begin(); 
                                    p != sequenceVector.end(); p++) {
            TIntermTyped* newNode = 0;
            constUnion *unionArray = new constUnion[1];

            switch (promoteTo) {
            case EbtFloat:
                switch ((*p)->getAsTyped()->getType().getBasicType()) {
                
                case EbtInt:  
                    unionArray->fConst = static_cast<float>((*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->iConst);
                    newNode = addConstantUnion(unionArray, TType(EbtFloat, EvqConst), node->getLine());  break;
                case EbtBool: 
                    unionArray->fConst = static_cast<float>((*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->bConst);
                    newNode = newNode = addConstantUnion(unionArray, TType(EbtFloat, EvqConst), node->getLine());  break;
                default:
                    infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                    return 0;
                }
                break;
            case EbtInt:
                switch ((*p)->getAsTyped()->getType().getBasicType()) {
                case EbtFloat:
                    unionArray->iConst = static_cast<int>((*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->fConst);
                    newNode = addConstantUnion(unionArray, TType(EbtInt, EvqConst), node->getLine());  
                    break;
                case EbtBool:
                    unionArray->iConst = static_cast<int>((*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->bConst);
                    newNode = addConstantUnion(unionArray, TType(EbtInt, EvqConst), node->getLine());  
                    break;
                default:
                    infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                    return 0;
                }
                break;
            case EbtBool:
                switch ((*p)->getAsTyped()->getType().getBasicType()) {
                case EbtFloat:
                    unionArray->bConst = (*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->fConst != 0.0 ;
                    newNode = addConstantUnion(unionArray, TType(EbtBool, EvqConst), node->getLine());  
                    break;
                case EbtInt:
                    unionArray->bConst = (*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->iConst != 0 ;
                    newNode = addConstantUnion(unionArray, TType(EbtBool, EvqConst), node->getLine());  
                    break;
                default: 
                    infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                    return 0;
                } 
                break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                return 0;
            }
            if (newNode) {
                sequenceVector.erase(p); 
                sequenceVector.insert(p, newNode);
            }
        }
        return node->getAsAggregate();
    } else if (node->getAsConstantUnion()) {

        return (promoteConstantUnion(promoteTo, node->getAsConstantUnion()));
    } else {
    
        //
        // Add a new newNode for the conversion.
        //
        TIntermUnary* newNode = 0;

        TOperator newOp = EOpNull;
        switch (promoteTo) {
        case EbtFloat:
            switch (node->getBasicType()) {
            case EbtInt:   newOp = EOpConvIntToFloat;  break;
            case EbtBool:  newOp = EOpConvBoolToFloat; break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                return 0;
            }
            break;
        case EbtBool:
            switch (node->getBasicType()) {
            case EbtInt:   newOp = EOpConvIntToBool;   break;
            case EbtFloat: newOp = EOpConvFloatToBool; break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                return 0;
            }
            break;
        case EbtInt:
            switch (node->getBasicType()) {
            case EbtBool:   newOp = EOpConvBoolToInt;  break;
            case EbtFloat:  newOp = EOpConvFloatToInt; break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Bad promotion node", node->getLine());
                return 0;
            }
            break;
        default: 
            infoSink.info.message(EPrefixInternalError, "Bad promotion type", node->getLine());
            return 0;
        }

        TType type(promoteTo, EvqTemporary, node->getNominalSize(), node->isMatrix(), node->isArray());
        newNode = new TIntermUnary(newOp, type);
        newNode->setLine(node->getLine());
        newNode->setOperand(node);

        return newNode;
    }
}

//
// Safe way to combine two nodes into an aggregate.  Works with null pointers, 
// a node that's not a aggregate yet, etc.
//
// Returns the resulting aggregate, unless 0 was passed in for 
// both existing nodes.
//
TIntermAggregate* TIntermediate::growAggregate(TIntermNode* left, TIntermNode* right, TSourceLoc line)
{
    if (left == 0 && right == 0)
        return 0;

    TIntermAggregate* aggNode = 0;
    if (left)
        aggNode = left->getAsAggregate();
    if (!aggNode || aggNode->getOp() != EOpNull) {
        aggNode = new TIntermAggregate;
        if (left)
            aggNode->getSequence().push_back(left);
    }

    if (right)
        aggNode->getSequence().push_back(right);

    if (line != 0)
        aggNode->setLine(line);

    return aggNode;
}

//
// Turn an existing node into an aggregate.
//
// Returns an aggregate, unless 0 was passed in for the existing node.
//
TIntermAggregate* TIntermediate::makeAggregate(TIntermNode* node, TSourceLoc line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggNode = new TIntermAggregate;
    aggNode->getSequence().push_back(node);

    if (line != 0)
        aggNode->setLine(line);
    else
        aggNode->setLine(node->getLine());

    return aggNode;
}

//
// For "if" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are in the
// nodePair.
//
// Returns the selection node created.
//
TIntermNode* TIntermediate::addSelection(TIntermTyped* cond, TIntermNodePair nodePair, TSourceLoc line)
{
    //
    // For compile time constant selections, prune the code and 
    // test now.
    //
    
    if (cond->getAsTyped() && cond->getAsTyped()->getAsConstantUnion()) {
        if (cond->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->bConst)
            return nodePair.node1;
        else
            return nodePair.node2;
    }

    TIntermSelection* node = new TIntermSelection(cond, nodePair.node1, nodePair.node2);
    node->setLine(line);

    return node;
}


TIntermTyped* TIntermediate::addComma(TIntermTyped* left, TIntermTyped* right, TSourceLoc line)
{
    if (left->getType().getQualifier() == EvqConst && right->getType().getQualifier() == EvqConst) {
        return right;
    } else {
        TIntermTyped *commaAggregate = growAggregate(left, right, line);
        commaAggregate->getAsAggregate()->setOperator(EOpComma);    
        commaAggregate->setType(right->getType());
        commaAggregate->getTypePointer()->changeQualifier(EvqTemporary);
        return commaAggregate;
    }
}

//
// For "?:" test nodes.  There are three children; a condition,
// a true path, and a false path.  The two paths are specified
// as separate parameters.
//
// Returns the selection node created, or 0 if one could not be.
//
TIntermTyped* TIntermediate::addSelection(TIntermTyped* cond, TIntermTyped* trueBlock, TIntermTyped* falseBlock, TSourceLoc line)
{
    //
    // Get compatible types.
    //
    TIntermTyped* child = addConversion(EOpSequence, trueBlock->getType(), falseBlock);
    if (child)
        falseBlock = child;
    else {
        child = addConversion(EOpSequence, falseBlock->getType(), trueBlock);
        if (child)
            trueBlock = child;
        else
            return 0;
    }

    //
    // See if condition is constant, and select now.
    //

    if (cond->getAsConstantUnion()) {
        if (cond->getAsConstantUnion()->getUnionArrayPointer()->bConst)
            return trueBlock;
        else
            return falseBlock;
    }

    //
    // Make a selection node.
    //
    TIntermSelection* node = new TIntermSelection(cond, trueBlock, falseBlock, trueBlock->getType());
    node->setLine(line);

    return node;
}

//
// Constant terminal nodes.  Has a union that contains bool, float or int constants
//
// Returns the constant union node created.
//

TIntermConstantUnion* TIntermediate::addConstantUnion(constUnion* unionArrayPointer, const TType& t, TSourceLoc line)
{
    TIntermConstantUnion* node = new TIntermConstantUnion(unionArrayPointer, t);
    node->setLine(line);

    return node;
}

TIntermTyped* TIntermediate::addSwizzle(TVectorFields& fields, TSourceLoc line)
{
    
    TIntermAggregate* node = new TIntermAggregate(EOpSequence);

    node->setLine(line);
    TIntermConstantUnion* constIntNode;
    TIntermSequence &sequenceVector = node->getSequence();
    constUnion* unionArray;

    for (int i = 0; i < fields.num; i++) {
        unionArray = new constUnion[1];
        unionArray->iConst = fields.offsets[i];
        constIntNode = addConstantUnion(unionArray, TType(EbtInt, EvqConst), line);
        sequenceVector.push_back(constIntNode);
    }

    return node;
}

//
// Create loop nodes.
//
TIntermNode* TIntermediate::addLoop(TIntermNode* body, TIntermTyped* test, TIntermTyped* terminal, bool testFirst, TSourceLoc line)
{
    TIntermNode* node = new TIntermLoop(body, test, terminal, testFirst);
    node->setLine(line);
    
    return node;
}

//
// Add branches.
//
TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TSourceLoc line)
{
    return addBranch(branchOp, 0, line);
}

TIntermBranch* TIntermediate::addBranch(TOperator branchOp, TIntermTyped* expression, TSourceLoc line)
{
    TIntermBranch* node = new TIntermBranch(branchOp, expression);
    node->setLine(line);

    return node;
}

//
// This is to be executed once the final root is put on top by the parsing
// process.
//
bool TIntermediate::postProcess(TIntermNode* root, EShLanguage language)
{
    if (root == 0)
        return true;

    //
    // First, finish off the top level sequence, if any
    //
    TIntermAggregate* aggRoot = root->getAsAggregate();
    if (aggRoot && aggRoot->getOp() == EOpNull)
        aggRoot->setOperator(EOpSequence);

    return true;
}

//
// This deletes the tree.
//
void TIntermediate::remove(TIntermNode* root)
{
    if (root)
        RemoveAllTreeNodes(root);
}

////////////////////////////////////////////////////////////////
//
// Member functions of the nodes used for building the tree.
//
////////////////////////////////////////////////////////////////

//
// Say whether or not an operation node changes the value of a variable.
//
// Returns true if state is modified.
//
bool TIntermOperator::modifiesState() const
{
    switch (op) {    
    case EOpPostIncrement: 
    case EOpPostDecrement: 
    case EOpPreIncrement:  
    case EOpPreDecrement:  
    case EOpAssign:    
    case EOpAddAssign: 
    case EOpSubAssign: 
    case EOpMulAssign: 
    case EOpVectorTimesMatrixAssign:
    case EOpVectorTimesScalarAssign:
    case EOpMatrixTimesScalarAssign:
    case EOpMatrixTimesMatrixAssign:
    case EOpDivAssign: 
    case EOpModAssign: 
    case EOpAndAssign: 
    case EOpInclusiveOrAssign: 
    case EOpExclusiveOrAssign: 
    case EOpLeftShiftAssign:   
    case EOpRightShiftAssign:  
        return true;
    default:
        return false;
    }
}

//
// returns true if the operator is for one of the constructors
//
bool TIntermOperator::isConstructor() const
{
    switch (op) {
    case EOpConstructVec2:
    case EOpConstructVec3:
    case EOpConstructVec4:
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
    case EOpConstructFloat:
    case EOpConstructIVec2:
    case EOpConstructIVec3:
    case EOpConstructIVec4:
    case EOpConstructInt:
    case EOpConstructBVec2:
    case EOpConstructBVec3:
    case EOpConstructBVec4:
    case EOpConstructBool:
    case EOpConstructStruct:
        return true;
    default:
        return false;
    }
}
//
// Make sure the type of a unary operator is appropriate for its 
// combination of operation and operand type.
//
// Returns false in nothing makes sense.
//
bool TIntermUnary::promote(TInfoSink&)
{
    switch (op) {
    case EOpLogicalNot:
        if (operand->getBasicType() != EbtBool)
            return false;
        break;
    case EOpBitwiseNot:
        if (operand->getBasicType() != EbtInt)
            return false;
        break;
    case EOpNegative:
    case EOpPostIncrement:
    case EOpPostDecrement:
    case EOpPreIncrement:
    case EOpPreDecrement:
        if (operand->getBasicType() == EbtBool)
            return false;
        break;

    // operators for built-ins are already type checked against their prototype
    case EOpAny:
    case EOpAll:
    case EOpVectorLogicalNot:
        return true;

    default:
        if (operand->getBasicType() != EbtFloat)
            return false;
    }
    
    setType(operand->getType());

    return true;
}

//
// Establishes the type of the resultant operation, as well as
// makes the operator the correct one for the operands.
//
// Returns false if operator can't work on operands.
//
bool TIntermBinary::promote(TInfoSink& infoSink)
{
    int size = left->getNominalSize();
    if (right->getNominalSize() > size)
        size = right->getNominalSize();

    TBasicType type = left->getBasicType();

    //
    // Don't operate on arrays.
    //
    if (left->isArray() || right->isArray())
        return false;

    //
    // Base assumption:  just make the type the same as the left
    // operand.  Then only deviations from this need be coded.
    //
    setType(TType(type, EvqTemporary, left->getNominalSize(), left->isMatrix()));
    
    //
    // All scalars.  Code after this test assumes this case is removed!
    //
    if (size == 1) {

        switch (op) {

        //
        // Promote to conditional
        //
        case EOpEqual:
        case EOpNotEqual:
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
            setType(TType(EbtBool));
            break;

        //
        // And and Or operate on conditionals
        //
        case EOpLogicalAnd:
        case EOpLogicalOr:
            if (left->getBasicType() != EbtBool || right->getBasicType() != EbtBool)
                return false;           
            setType(TType(EbtBool));
            break;

        //
        // Check for integer only operands.
        //
        case EOpMod:
        case EOpRightShift:
        case EOpLeftShift:
        case EOpAnd:
        case EOpInclusiveOr:
        case EOpExclusiveOr:
            if (left->getBasicType() != EbtInt || right->getBasicType() != EbtInt)
                return false;
            break;
        case EOpModAssign:
        case EOpAndAssign:
        case EOpInclusiveOrAssign:
        case EOpExclusiveOrAssign:
        case EOpLeftShiftAssign:
        case EOpRightShiftAssign:
            if (left->getBasicType() != EbtInt || right->getBasicType() != EbtInt)
                return false;
            // fall through

        //
        // Everything else should have matching types
        //
        default:
            if (left->getBasicType() != right->getBasicType() ||
                left->isMatrix()     != right->isMatrix())
                return false;
        }

        return true;
    }

    //
    // Are the sizes compatible?
    //
    if ( left->getNominalSize() != size &&  left->getNominalSize() != 1 ||
        right->getNominalSize() != size && right->getNominalSize() != 1)
        return false;

    //
    // Can these two operands be combined?
    //
    switch (op) {
    case EOpMul:
        if (!left->isMatrix() && right->isMatrix()) {
            if (left->isVector())
                op = EOpVectorTimesMatrix;
            else {
                op = EOpMatrixTimesScalar;
                setType(TType(type, EvqTemporary, size, true));
            }
        } else if (left->isMatrix() && !right->isMatrix()) {
            if (right->isVector()) {
                op = EOpMatrixTimesVector;
                setType(TType(type, EvqTemporary, size, false));
            } else {
                op = EOpMatrixTimesScalar;
            }
        } else if (left->isMatrix() && right->isMatrix()) {
            op = EOpMatrixTimesMatrix;
        } else if (!left->isMatrix() && !right->isMatrix()) {
            if (left->isVector() && right->isVector()) {
                // leave as component product
            } else if (left->isVector() || right->isVector()) {
                op = EOpVectorTimesScalar;
                setType(TType(type, EvqTemporary, size, false));
            }
        } else {
            infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
            return false;
        }
        break;
    case EOpMulAssign:
        if (!left->isMatrix() && right->isMatrix()) {
            if (left->isVector())
                op = EOpVectorTimesMatrixAssign;
            else {
                return false;
            }
        } else if (left->isMatrix() && !right->isMatrix()) {
            if (right->isVector()) {
                return false;
            } else {
                op = EOpMatrixTimesScalarAssign;
            }
        } else if (left->isMatrix() && right->isMatrix()) {
            op = EOpMatrixTimesMatrixAssign;
        } else if (!left->isMatrix() && !right->isMatrix()) {
            if (left->isVector() && right->isVector()) {
                // leave as component product
            } else if (left->isVector() || right->isVector()) {
                if (! left->isVector())
                    return false;
                op = EOpVectorTimesScalarAssign;
                setType(TType(type, EvqTemporary, size, false));
            }
        } else {
            infoSink.info.message(EPrefixInternalError, "Missing elses", getLine());
            return false;
        }
        break;      
    case EOpAssign:
        if (left->getNominalSize() != right->getNominalSize())
            return false;
        // fall through
    case EOpAdd:
    case EOpSub:
    case EOpDiv:
    case EOpMod:
    case EOpAddAssign:
    case EOpSubAssign:
    case EOpDivAssign:
    case EOpModAssign:
        if (left->isMatrix() && right->isVector() ||
            left->isVector() && right->isMatrix() ||
            left->getBasicType() != right->getBasicType())
            return false;
        setType(TType(type, EvqTemporary, size, left->isMatrix() || right->isMatrix()));
        break;
        
    case EOpEqual:
    case EOpNotEqual:
    case EOpLessThan:
    case EOpGreaterThan:
    case EOpLessThanEqual:
    case EOpGreaterThanEqual:
        if (left->isMatrix() && right->isVector() ||
            left->isVector() && right->isMatrix() ||
            left->getBasicType() != right->getBasicType())
            return false;
        setType(TType(EbtBool));
        break;

default:
        return false;
    }

    //
    // One more check for assignment.  The Resulting type has to match the left operand.
    //
    switch (op) {
    case EOpAssign:
    case EOpAddAssign:
    case EOpSubAssign:
    case EOpMulAssign:
    case EOpDivAssign:
    case EOpModAssign:
    case EOpAndAssign:
    case EOpInclusiveOrAssign:
    case EOpExclusiveOrAssign:
    case EOpLeftShiftAssign:
    case EOpRightShiftAssign:
        if (getType() != left->getType())
            return false;
        break;
    default: 
        break;
    }
    
    return true;
}

bool compareStructure(const TType& leftNodeType, constUnion* rightUnionArray, constUnion* leftUnionArray, int& index)
{
    TTypeList* fields = leftNodeType.getStruct();

    size_t structSize = fields->size();

    for (size_t j = 0; j < structSize; j++) {
        int size = (*fields)[j].type->getInstanceSize();
        for (int i = 0; i < size; i++) {
            switch ((*fields)[j].type->getBasicType()) {
            case EbtFloat:
                if (leftUnionArray[index].fConst != rightUnionArray[index].fConst)
                    return false;
                index++;
                break;
            case EbtInt:
                if (leftUnionArray[index].iConst != rightUnionArray[index].iConst)
                    return false;
                index++;
                break;
            case EbtBool:
                if (leftUnionArray[index].bConst != rightUnionArray[index].bConst)
                    return false;
                index++;
                break;
            case EbtStruct:
                if (!compareStructure(*(*fields)[j].type, rightUnionArray, leftUnionArray, index))
                    return false;
                break;
            default: 
                assert(true && "Cannot compare");
                break;
            }    
            
        }
    }
    return true;
} 

//
// The fold functions see if an operation on a constant can be done in place,
// without generating run-time code.
//
// Returns the node to keep using, which may or may not be the node passed in.
//

TIntermTyped* TIntermConstantUnion::fold(TOperator op, TIntermTyped* constantNode, TInfoSink& infoSink, bool leftOperand)
{   
    constUnion *unionArray = this->getUnionArrayPointer(); 

    if (constantNode) {
        if (constantNode->getAsConstantUnion() && constantNode->getSize() == 1 && constantNode->getType().getBasicType() != EbtStruct
            && this->getSize() > 1) {
            TIntermConstantUnion *node = constantNode->getAsConstantUnion();
            TIntermConstantUnion *newNode;
            constUnion* tempConstArray;
            switch(op) {
            case EOpAdd: 
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].fConst = unionArray[i].fConst + node->getUnionArrayPointer()->fConst; break;
                        case EbtInt:   tempConstArray[i].iConst = unionArray[i].iConst + node->getUnionArrayPointer()->iConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"+\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;
            case EOpMatrixTimesScalar: 
            case EOpVectorTimesScalar:
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].fConst = unionArray[i].fConst * node->getUnionArrayPointer()->fConst; break;
                        case EbtInt:   tempConstArray[i].iConst = unionArray[i].iConst * node->getUnionArrayPointer()->iConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"*\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;
            case EOpSub:
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: 
                            if (leftOperand)
                                tempConstArray[i].fConst = unionArray[i].fConst - node->getUnionArrayPointer()->fConst; 
                            else
                                tempConstArray[i].fConst = node->getUnionArrayPointer()->fConst - unionArray[i].fConst; 
                        break;

                        case EbtInt:   
                            if (leftOperand) 
                                tempConstArray[i].iConst = unionArray[i].iConst - node->getUnionArrayPointer()->iConst; 
                            else
                                tempConstArray[i].iConst = node->getUnionArrayPointer()->iConst - unionArray[i].iConst; 
                            break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"-\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpDiv:
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: 
                            if (leftOperand) {
                                if (node->getUnionArrayPointer()->fConst == 0.0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].fConst = FLT_MAX;
                                } else
                                    tempConstArray[i].fConst = unionArray[i].fConst / node->getUnionArrayPointer()->fConst; 
                            } else {
                                if (unionArray[i].fConst == 0.0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].fConst = FLT_MAX;
                                } else
                                    tempConstArray[i].fConst = node->getUnionArrayPointer()->fConst / unionArray[i].fConst; 
                            }
                            break;

                        case EbtInt:   
                            if (leftOperand) {
                                if (node->getUnionArrayPointer()->iConst == 0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].iConst = INT_MAX;
                                } else
                                    tempConstArray[i].iConst = unionArray[i].iConst / node->getUnionArrayPointer()->iConst; 
                            } else {
                                if (unionArray[i].iConst == 0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].iConst = INT_MAX;
                                } else
                                    tempConstArray[i].iConst = node->getUnionArrayPointer()->iConst / unionArray[i].iConst; 
                            }
                            break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"/\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpLogicalAnd: // this code is written for possible future use, will not get executed currently
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst && node->getUnionArrayPointer()->bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"&&\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;
            
            case EOpLogicalXor: // this code is written for possible future use, will not get executed currently
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst ^ node->getUnionArrayPointer()->bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"^^\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpLogicalOr: // this code is written for possible future use, will not get executed currently                
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst || node->getUnionArrayPointer()->bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"||\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            default:
                infoSink.info.message(EPrefixInternalError, "Invalid operator for constant folding", this->getLine());
                return 0;
            }
            newNode = new TIntermConstantUnion(tempConstArray, this->getType());
            newNode->setLine(this->getLine());
                
            return newNode;
        } else if (constantNode->getAsConstantUnion() && (this->getSize() > 1 || this->getType().getBasicType() == EbtStruct)) {
            TIntermConstantUnion *node = constantNode->getAsConstantUnion();
            constUnion *rightUnionArray = node->getUnionArrayPointer(); 
            constUnion* tempConstArray = 0;
            TIntermConstantUnion *tempNode;
            int index = 0;
            bool boolNodeFlag = false;
            switch(op) {
            case EOpAdd: 
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].fConst = unionArray[i].fConst + rightUnionArray[i].fConst; break;
                        case EbtInt:   tempConstArray[i].iConst = unionArray[i].iConst + rightUnionArray[i].iConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"+\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;
            case EOpSub: 
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {                                
                        case EbtFloat: 
                            if (leftOperand)
                                tempConstArray[i].fConst = unionArray[i].fConst - rightUnionArray[i].fConst; 
                            else
                                tempConstArray[i].fConst = rightUnionArray[i].fConst - unionArray[i].fConst; 
                        break;

                        case EbtInt:   
                            if (leftOperand) 
                                tempConstArray[i].iConst = unionArray[i].iConst - rightUnionArray[i].iConst;
                            else
                                tempConstArray[i].iConst = rightUnionArray[i].iConst - unionArray[i].iConst; 
                            break;
            
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"-\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;
            case EOpMul: 
                if (this->isVector()) { // two vectors multiplied together
                    int size = this->getSize();
                    tempConstArray = new constUnion[size];
                    
                    for (int i = 0; i < size; i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: tempConstArray[i].fConst = unionArray[i].fConst * rightUnionArray[i].fConst;     break;
                        case EbtInt:   tempConstArray[i].iConst = unionArray[i].iConst * rightUnionArray[i].iConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for vector multiply", this->getLine());
                            return 0;
                        }
                    }
                } 
                break;
            case EOpMatrixTimesMatrix:                
                if (this->getType().getBasicType() != EbtFloat || node->getBasicType() != EbtFloat) {
                        infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix multiply", this->getLine());
                        return 0;
                }
                {// support MSVC++6.0
                    int size = this->getNominalSize();
                    tempConstArray = new constUnion[size*size];
                    for (int row = 0; row < size; row++) {
                        for (int column = 0; column < size; column++) {
                            tempConstArray[size * column + row].fConst = 0.0;
                            for (int i = 0; i < size; i++) {
                                tempConstArray[size * column + row].fConst += unionArray[i * size + row].fConst * (rightUnionArray[column * size + i].fConst); 
                            }
                        }
                    }
                }
                break;
            case EOpDiv: 
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtFloat: 
                            if (leftOperand) {
                                if (rightUnionArray[i].fConst == 0.0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].fConst = FLT_MAX;
                                } else
                                    tempConstArray[i].fConst = unionArray[i].fConst / rightUnionArray[i].fConst; 
                            } else {
                                if (unionArray[i].fConst == 0.0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].fConst = FLT_MAX;
                                } else
                                    tempConstArray[i].fConst = rightUnionArray[i].fConst / unionArray[i].fConst; 
                            }
                            break;

                        case EbtInt:   
                            if (leftOperand) {
                                if (rightUnionArray[i].iConst == 0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].iConst = INT_MAX;
                                } else
                                    tempConstArray[i].iConst = unionArray[i].iConst / rightUnionArray[i].iConst; 
                            } else {
                                if (unionArray[i].iConst == 0) {
                                    infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                                    tempConstArray[i].iConst = INT_MAX;
                                } else
                                    tempConstArray[i].iConst = rightUnionArray[i].iConst / unionArray[i].iConst; 
                            }
                            break;            
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"/\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpMatrixTimesVector: 
                if (node->getBasicType() != EbtFloat) {
                        infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for matrix times vector", this->getLine());
                        return 0;
                }
                tempConstArray = new constUnion[this->getNominalSize()];
                
                {// support MSVC++6.0                    
                    for (int size = this->getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].fConst = 0.0;
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].fConst += ((unionArray[j*size + i].fConst) * rightUnionArray[j].fConst); 
                        }
                    }
                }
                
                tempNode = new TIntermConstantUnion(tempConstArray, node->getType());
                tempNode->setLine(this->getLine());

                return tempNode;                

            case EOpVectorTimesMatrix:
                if (this->getType().getBasicType() != EbtFloat) {
                    infoSink.info.message(EPrefixInternalError, "Constant Folding cannot be done for vector times matrix", this->getLine());
                    return 0;
                }  

                tempConstArray = new constUnion[this->getNominalSize()];
                {// support MSVC++6.0
                    for (int size = this->getNominalSize(), i = 0; i < size; i++) {
                        tempConstArray[i].fConst = 0.0;
                        for (int j = 0; j < size; j++) {
                            tempConstArray[i].fConst += ((unionArray[j].fConst) * rightUnionArray[i*size + j].fConst); 
                        }
                    }
                }
                break;

            case EOpLogicalAnd: // this code is written for possible future use, will not get executed currently
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst && rightUnionArray[i].bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"&&\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpLogicalXor: // this code is written for possible future use, will not get executed currently
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst ^ rightUnionArray[i].bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"^^\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpLogicalOr: // this code is written for possible future use, will not get executed currently
                tempConstArray = new constUnion[this->getSize()];
                {// support MSVC++6.0
                    for (int i = 0; i < this->getSize(); i++) {            
                        switch (this->getType().getBasicType()) {
                        case EbtBool:   tempConstArray[i].bConst = unionArray[i].bConst || rightUnionArray[i].bConst; break;
                        default: 
                            infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"||\"", this->getLine());
                            return 0;
                        }
                    }
                }
                break;

            case EOpEqual: 
                   
                switch (this->getType().getBasicType()) {
                case EbtFloat: 
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].fConst != rightUnionArray[i].fConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;

                case EbtInt:   
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].iConst != rightUnionArray[i].iConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;
                case EbtBool:   
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].bConst != rightUnionArray[i].bConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;
                case EbtStruct:
                    if (!compareStructure(node->getType(), node->getUnionArrayPointer(), unionArray, index))
                        boolNodeFlag = true;
                    break;
                
                default: 
                        infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"==\"", this->getLine());
                        return 0;
                }

                tempConstArray = new constUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->bConst = true;
                }
                else {
                    tempConstArray->bConst = false;
                }
                
                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EvqConst));
                tempNode->setLine(this->getLine());
    
                return tempNode;         

            case EOpNotEqual: 
                switch (this->getType().getBasicType()) {
                case EbtFloat: 
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].fConst == rightUnionArray[i].fConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;

                case EbtInt:   
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].iConst == rightUnionArray[i].iConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;
                case EbtBool:   
                    {// support MSVC++6.0
                        for (int i = 0; i < this->getSize(); i++) {    
                            if (unionArray[i].bConst == rightUnionArray[i].bConst) {
                                boolNodeFlag = true;
                                break;  // break out of for loop
                            }
                        }
                    }
                    break;
                case EbtStruct:
                    if (compareStructure(node->getType(), node->getUnionArrayPointer(), unionArray, index))
                        boolNodeFlag = true;
                    break;
                default: 
                        infoSink.info.message(EPrefixInternalError, "Constant folding cannot be done for \"!=\"", this->getLine());
                        return 0;
                }

                tempConstArray = new constUnion[1];
                if (!boolNodeFlag) {
                    tempConstArray->bConst = true;
                }
                else {
                    tempConstArray->bConst = false;
                }
                
                tempNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EvqConst));
                tempNode->setLine(this->getLine());
    
                return tempNode;         
          
            default: 
                infoSink.info.message(EPrefixInternalError, "Invalid operator for constant folding", this->getLine());
                return 0;
            }
            tempNode = new TIntermConstantUnion(tempConstArray, this->getType());
            tempNode->setLine(this->getLine());

            return tempNode;                
        } else if (this->getSize() == 1 && this->getType().getBasicType() != EbtStruct 
                    && constantNode->getSize() == 1 && constantNode->getType().getBasicType() != EbtStruct ) {  // scalar constant folding
            constUnion *unionArray = new constUnion[1];
            TIntermConstantUnion* newNode = 0;

            switch (this->getType().getBasicType()) {
            case EbtInt:
                {
                    //
                    // Dealing with two operands, us and constant.
                    //
                    // Do Binary operations.
                    //
                    int rightValue = constantNode->getAsConstantUnion()->getUnionArrayPointer()->iConst;
                    int leftValue = this->getUnionArrayPointer()->iConst;
                    //int line = this->getLine();

                    switch(op) {
                    //?? add constant intrinsics
                    case EOpAdd: unionArray->iConst = leftValue + rightValue; break;
                    case EOpSub: unionArray->iConst = leftValue - rightValue; break;
                    case EOpMul: unionArray->iConst = leftValue * rightValue; break;
                    case EOpDiv: 
                        if (rightValue == 0) {
                            infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                            unionArray->iConst = INT_MAX;
                        } else
                            unionArray->iConst = leftValue / rightValue; break;
                            
                    case EOpMod: unionArray->iConst = leftValue % rightValue; break;
                    
                    case EOpRightShift: unionArray->iConst = leftValue >> rightValue; break;
                    case EOpLeftShift:  unionArray->iConst = leftValue << rightValue; break;
                    
                    case EOpAnd:         unionArray->iConst = leftValue & rightValue; break;
                    case EOpInclusiveOr: unionArray->iConst = leftValue | rightValue; break;
                    case EOpExclusiveOr: unionArray->iConst = leftValue ^ rightValue; break;

                    // the following assume it's okay to have memory leaks
                    case EOpEqual:            
                        unionArray->bConst = leftValue == rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpNotEqual:         
                        unionArray->bConst = leftValue != rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpLessThan:         
                        unionArray->bConst = leftValue < rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpGreaterThan:      
                        unionArray->bConst = leftValue > rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpLessThanEqual:    
                        unionArray->bConst = leftValue <= rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpGreaterThanEqual: 
                        unionArray->bConst = leftValue >= rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;

                    default:
                        //infoSink.info.message(EPrefixInternalError, "Binary operation not folded into constant int", line);
                        return 0;
                    }
                    if (!newNode) {
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtInt, EvqConst)); 
                    }
                    newNode->setLine(constantNode->getLine());
                    return newNode;
                }
            case EbtFloat:
                {
                    float rightValue = constantNode->getAsConstantUnion()->getUnionArrayPointer()->fConst;
                    float leftValue = this->getUnionArrayPointer()->fConst;

                    switch(op) {
                    //?? add constant intrinsics
                    case EOpAdd: unionArray->fConst = leftValue + rightValue; break;
                    case EOpSub: unionArray->fConst = leftValue - rightValue; break;
                    case EOpMul: unionArray->fConst = leftValue * rightValue; break;
                    case EOpDiv: 
                        if (rightValue == 0.0) {
                            infoSink.info.message(EPrefixWarning, "Divide by zero error during constant folding", this->getLine());
                            unionArray->fConst = FLT_MAX;
                        } else
                            unionArray->fConst = leftValue / rightValue; break;

                    // the following assume it's okay to have memory leaks (cleaned up by pool allocator)
                    case EOpEqual:            
                        unionArray->bConst = leftValue == rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpNotEqual:         
                        unionArray->bConst = leftValue != rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpLessThan:         
                        unionArray->bConst = leftValue < rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpGreaterThan:      
                        unionArray->bConst = leftValue > rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpLessThanEqual:    
                        unionArray->bConst = leftValue <= rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;
                    case EOpGreaterThanEqual: 
                        unionArray->bConst = leftValue >= rightValue;
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                        break;

                    default: 
                        //infoSink.info.message(EPrefixInternalError, "Binary operation not folded into constant float", line);
                        return 0;
                    }
                    if (!newNode) {
                        newNode = new TIntermConstantUnion(unionArray, TType(EbtFloat, EvqConst)); 
                    }
                    newNode->setLine(constantNode->getLine());
                    return newNode;
                }
            case EbtBool:
                {
                    bool rightValue = constantNode->getAsConstantUnion()->getUnionArrayPointer()->bConst;
                    bool leftValue = this->getUnionArrayPointer()->bConst;

                    switch(op) {
                    //?? add constant intrinsics
                    case EOpLogicalAnd: unionArray->bConst = leftValue & rightValue; break;
                    case EOpLogicalXor: unionArray->bConst = leftValue ^ rightValue; break;
                    case EOpLogicalOr:  unionArray->bConst = leftValue | rightValue; break;
                    default: 
                        infoSink.info.message(EPrefixInternalError, "Binary operator cannot be folded into constant bool", line);
                        return 0;
                    }
                    newNode = new TIntermConstantUnion(unionArray, TType(EbtBool, EvqConst)); 
                    newNode->setLine(constantNode->getLine());
                    return newNode;
                }
            default:
                infoSink.info.message(EPrefixInternalError, "Cannot fold constant", this->getLine());
                return 0;
            }
        }
    } else { 
        //
        // Do unary operations
        //
        TIntermConstantUnion *newNode = 0;
        constUnion* tempConstArray = new constUnion[this->getSize()];
        if (this->getSize() > 1) {
            for (int i = 0; i < this->getSize(); i++) {
                switch(op) {
                case EOpNegative:                                       
                    switch (this->getType().getBasicType()) {
                    case EbtFloat: tempConstArray[i].fConst = -(unionArray[i].fConst);      break;
                    case EbtInt:   tempConstArray[i].iConst = -(unionArray[i].iConst); break;
                    default: 
                        infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", this->getLine());
                        return 0;
                    }
                    break;
                case EOpLogicalNot: // this code is written for possible future use, will not get executed currently                                      
                    switch (this->getType().getBasicType()) {
                    case EbtBool:  tempConstArray[i].bConst = !(unionArray[i].bConst); break;
                    default: 
                        infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", this->getLine());
                        return 0;
                    }
                    break;
                default: 
                    return 0;
                }
            }
            newNode = new TIntermConstantUnion(tempConstArray, this->getType());
            newNode->setLine(this->getLine());
            return newNode;     
        } else {
            switch(op) {
            //?? add constant intrinsics
            case EOpNegative:   
                switch (this->getType().getBasicType()) {
                case EbtInt:
                    tempConstArray->iConst = -(this->getUnionArrayPointer()->iConst);
                    newNode = new TIntermConstantUnion(tempConstArray, TType(EbtInt, EvqConst));
                    break;
                case EbtFloat:
                    tempConstArray->fConst = -(this->getUnionArrayPointer()->fConst);
                    newNode = new TIntermConstantUnion(tempConstArray, TType(EbtFloat, EvqConst));
                    break;
                default: 
                    infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", line);
                    return 0;
                }
                break;
            case EOpLogicalNot:   
                switch (this->getType().getBasicType()) {
                case EbtBool:
                    tempConstArray->bConst = !this->getUnionArrayPointer()->bConst;
                    newNode = new TIntermConstantUnion(tempConstArray, TType(EbtBool, EvqConst));
                    break;
                default: 
                    infoSink.info.message(EPrefixInternalError, "Unary operation not folded into constant", line);
                    return 0;
                }
                break;
            default: 
                return 0;
            }
            newNode->setLine(this->getLine());
            return newNode;
        
        }
    }

    return this;
}

TIntermConstantUnion* TIntermediate::changeAggrToTempConst(TIntermAggregate* node, TSymbolTable& symbolTable, TSourceLoc line) 
{
    constUnion* unionArray = new constUnion[node->getType().getInstanceSize()];    
    bool returnVal;

    if (node->getSequence().size() == 1 && node->getSequence()[0]->getAsTyped()->getAsConstantUnion())  {
        returnVal = parseConstTree(line, node, unionArray, node->getOp(), symbolTable,  node->getType(), true);
    }
    else {
        returnVal = parseConstTree(line, node, unionArray, node->getOp(), symbolTable, node->getType());
    }

    if (returnVal)
        unionArray = 0;

    return (addConstantUnion(unionArray, node->getType(), node->getLine()));    
}

TIntermTyped* TIntermediate::copyConstUnion(TIntermConstantUnion* node) 
{
    constUnion *unionArray = node->getUnionArrayPointer();

    if (!unionArray)
        return 0;
 
    int size;
    if (node->getType().getBasicType() == EbtStruct)
        //?? We should actually be calling getStructSize() function and not setStructSize. This problem occurs in case
        // of nested/embedded structs.
        size = node->getType().setStructSize(node->getType().getStruct());
        //size = node->getType().getStructSize();
    else
        size = node->getType().getInstanceSize();
    
    constUnion *newSpace = new constUnion[size];

    for (int i = 0; i < size; i++)
        newSpace[i] = unionArray[i];

    node->setUnionArrayPointer(newSpace);
    return node;
}

TIntermTyped* TIntermediate::promoteConstantUnion(TBasicType promoteTo, TIntermConstantUnion* node) 
{
    constUnion *rightUnionArray = node->getUnionArrayPointer();
    int size = node->getType().getInstanceSize();

    constUnion *leftUnionArray = new constUnion[size];

    for (int i=0; i < size; i++) {
        
        switch (promoteTo) {
        case EbtFloat:
            switch (node->getType().getBasicType()) {
            case EbtInt:
                (leftUnionArray[i]).fConst = static_cast<float>(rightUnionArray[i].iConst);
                break;
            case EbtBool:
                (leftUnionArray[i]).fConst = static_cast<float>(rightUnionArray[i].bConst);
                break;
            case EbtFloat:
                (leftUnionArray[i]).fConst = rightUnionArray[i].fConst;
                break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                return 0;
            }                
            break;
        case EbtInt:
            switch (node->getType().getBasicType()) {
            case EbtInt:
                (leftUnionArray[i]).iConst = rightUnionArray[i].iConst;
                break;
            case EbtBool:
                (leftUnionArray[i]).iConst = static_cast<int>(rightUnionArray[i].bConst);
                break;
            case EbtFloat:
                (leftUnionArray[i]).iConst = static_cast<int>(rightUnionArray[i].fConst);
                break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                return 0;
            }                
            break;
        case EbtBool:
            switch (node->getType().getBasicType()) {
            case EbtInt:
                (leftUnionArray[i]).bConst = rightUnionArray[i].iConst != 0;
                break;
            case EbtBool:
                (leftUnionArray[i]).bConst = rightUnionArray[i].bConst;
                break;
            case EbtFloat:
                (leftUnionArray[i]).bConst = rightUnionArray[i].fConst != 0.0;
                break;
            default: 
                infoSink.info.message(EPrefixInternalError, "Cannot promote", node->getLine());
                return 0;
            }                
            
            break;
        default:
            infoSink.info.message(EPrefixInternalError, "Incorrect data type found", node->getLine());
            return 0;
        }
    
    }
    
    const TType& t = node->getType();
    
    return addConstantUnion(leftUnionArray, TType(promoteTo, t.getQualifier(), t.getNominalSize(), t.isMatrix(), t.isArray()), node->getLine());
}

//
// This method inserts the child nodes into the parent node at the given location specified
// by parentNodeIter. offset tells the integer offset into the parent vector that points to 
// the child node. sequenceVector is the parent vector.
// Returns reference to the last inserted child node
// increments the offset based on the number of child nodes added
//
void TIntermediate::removeChildNode(TIntermSequence &parentSequence, TType& parentType, int& offset, TIntermSequence::iterator& parentNodeIter, TIntermAggregate* child)
{
    if (!child)
        return;

    parentNodeIter = parentSequence.begin() + offset;

    TIntermSequence& childSequence = child->getSequence();
    int oldSize = static_cast<int>(parentSequence.size());
    if (childSequence.size() == 1) {
        if (!removeMatrixConstNode(parentSequence, parentType, child, offset)) {
            for (int i = 0; i < child->getType().getInstanceSize(); i++) {
                constUnion* constantUnion = new constUnion[1];
                *constantUnion = *(childSequence[0]->getAsConstantUnion()->getUnionArrayPointer());
                TIntermConstantUnion *constant = new TIntermConstantUnion(constantUnion,
                    childSequence[0]->getAsConstantUnion()->getType());
                constant->setLine(child->getLine());
                parentNodeIter = parentSequence.begin() + offset;
                parentSequence.insert(parentNodeIter, constant);
            }
        }
    } else    
        parentSequence.insert(parentNodeIter, childSequence.begin(), childSequence.end());

    int newSize = static_cast<int>(parentSequence.size());
    offset = offset + newSize - oldSize;
    parentNodeIter = parentSequence.begin() + offset;
    parentNodeIter = parentSequence.erase(parentNodeIter);
    offset--;
    parentNodeIter--;
}

//
// The parent has only one child node. This method is not implemented
// for parent that is a structure
//
TIntermTyped* TIntermediate::removeChildNode(TIntermTyped* parent, TType* parentType, TIntermAggregate* child)
{
    TIntermTyped* resultNode = 0;

    if (parentType->getInstanceSize() == 1) {
        resultNode = child->getSequence()[0]->getAsTyped(); 
    } else {
        int size = parentType->getInstanceSize();
        TIntermSequence& parentSequence = parent->getAsAggregate()->getSequence();
        TIntermSequence& childSequence = child->getSequence();

        if (childSequence.size() == 1) {
            if (!removeMatrixConstNode(parentSequence, *parentType, child, 1))
                parentSequence.push_back(child->getSequence()[0]);
        } else {
            for (int i = 0; i < size; i++) {
                parentSequence.push_back(child->getSequence()[i]);
            }
        }
        parentSequence.erase(parentSequence.begin());

        return parent;
    }

    return resultNode;
}

bool TIntermediate::removeMatrixConstNode(TIntermSequence &parentSequence, TType& parentType, TIntermAggregate* child, int offset)
{
    if (!child)
        return false;

    TIntermSequence::iterator parentNodeIter;
    TIntermSequence &childSequence = child->getSequence();

    switch (child->getOp()) {
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
        {// support MSVC++6.0
            for (int i = 0; i < child->getType().getInstanceSize(); i++) {
                constUnion* constantUnion = new constUnion[1];
                if (i % (child->getType().getNominalSize() + 1) == 0) {
                    *constantUnion = *(childSequence[0]->getAsConstantUnion()->getUnionArrayPointer());
                } else {
                    switch (parentType.getBasicType()) {
                    case EbtInt:   constantUnion->iConst = 0;     break;
                    case EbtFloat: constantUnion->fConst = 0.0;   break;
                    case EbtBool:  constantUnion->bConst = false; break;
                    default: ; /* default added by BrianP */
                    }
                }
                TIntermConstantUnion *constant = new TIntermConstantUnion(constantUnion,
                    childSequence[0]->getAsConstantUnion()->getType());
                constant->setLine(child->getLine());
                parentNodeIter = parentSequence.begin() + offset + i;
                parentSequence.insert(parentNodeIter, constant);
            }
        }
        return true;
    default:
        return false;
    }
}

void TIntermAggregate::addToPragmaTable(const TPragmaTable& pTable)
{
    assert (!pragmaTable);
    pragmaTable = new TPragmaTable();
    *pragmaTable = pTable;
}

