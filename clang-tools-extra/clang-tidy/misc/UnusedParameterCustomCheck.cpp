//===--- UnusedParameterCustomCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "UnusedParameterCustomCheck.h"
//#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

void UnusedParameterCustomCheck::registerMatchers(MatchFinder *finder) {
  finder->addMatcher(functionDecl(isDefinition(),
                                  hasBody(stmt()),
                                  hasAnyParameter(parmVarDecl()))
                              .bind("functionWithParams"), this);
}


template <typename T>
static CharSourceRange getNodeLocationRange(const MatchFinder::MatchResult &result,
                                                       const T *prevNode, const T *currNode,
                                                       const T *nextNode) {
  if (nextNode)
    return CharSourceRange::getCharRange(currNode->getBeginLoc(),
                                         nextNode->getBeginLoc());

  if (prevNode)
    return CharSourceRange::getTokenRange(
        Lexer::getLocForEndOfToken(prevNode->getEndLoc(), 0,
                                   *result.SourceManager,
                                   result.Context->getLangOpts()),
        currNode->getEndLoc());

  return CharSourceRange::getTokenRange(currNode->getSourceRange());
}


UnusedParameterCustomCheck::~UnusedParameterCustomCheck() = default;
UnusedParameterCustomCheck::UnusedParameterCustomCheck(StringRef name, ClangTidyContext *context)
    : ClangTidyCheck(name, context) {}


FixItHint UnusedParameterCustomCheck::removeArgument(const MatchFinder::MatchResult &result,
                                                     const CallExpr *call, unsigned argIdx) {
  const Expr *currNode = call->getArg(argIdx);
  const Expr *prevNode = nullptr;
  const Expr *nextNode = nullptr;

  if (argIdx > 0)
    prevNode = call->getArg(argIdx - 1);

  if (argIdx + 1 < call->getNumArgs())
    nextNode = call->getArg(argIdx + 1);

  return FixItHint::CreateRemoval(getNodeLocationRange(result,
                                                       prevNode, currNode, nextNode)); 
}


FixItHint UnusedParameterCustomCheck::removeParameter(const MatchFinder::MatchResult &result,
                                                      const FunctionDecl *matchedFunctionDecl,
                                                      unsigned paramIdx) {
  const ParmVarDecl *currNode = matchedFunctionDecl->getParamDecl(paramIdx);
  const ParmVarDecl *prevNode = nullptr;
  const ParmVarDecl *nextNode = nullptr;

  if (paramIdx > 0)
    prevNode = matchedFunctionDecl->getParamDecl(paramIdx - 1);

  if (paramIdx + 1 < matchedFunctionDecl->getNumParams())
    nextNode = matchedFunctionDecl->getParamDecl(paramIdx + 1);

  return FixItHint::CreateRemoval(getNodeLocationRange(result,
                                                       prevNode, currNode, nextNode)); 
}


class UnusedParameterCustomCheck::FunctionCallVisitor
    : public RecursiveASTVisitor<FunctionCallVisitor> {
public:
  FunctionCallVisitor(ASTContext &context) { TraverseAST(context); }

  const std::unordered_set<const CallExpr *> getFunctionCalls(const FunctionDecl *func) {
    return functionCalls[func->getCanonicalDecl()];
  }

  bool VisitCallExpr(CallExpr *call) {
    // Getting FunctionDecl of Callee function from the CallExpr 
    if (const auto *calleeFunctionDecl = dyn_cast_or_null<FunctionDecl>(call->getCalleeDecl())) {
        calleeFunctionDecl = calleeFunctionDecl->getCanonicalDecl();
        // Add found FunctionDecl's CallExpr to list of callers
        functionCalls[calleeFunctionDecl].insert(call);
    }
    return true;
  }


private:
  // Need map due to function redeclarations with unique callers
  std::unordered_map<const FunctionDecl *,
                     std::unordered_set<const CallExpr *>> functionCalls;
};


void UnusedParameterCustomCheck::warnAndFixUnusedParameter(const MatchFinder::MatchResult &result,
                                                           const FunctionDecl *matchedFunctionDecl,
                                                           const unsigned paramIdx) {
  const auto unusedParamDecl = matchedFunctionDecl->getParamDecl(paramIdx);

  DiagnosticBuilder diagBuilder = diag(matchedFunctionDecl->getLocation(), "Parameter %0 is unused!") 
    << unusedParamDecl;

  SourceRange unusedParamLocation(unusedParamDecl->getLocation());

  // Comment out unused parameters
  //diagBuilder << FixItHint::CreateReplacement(unusedParamLocation, (" /*"+unusedParamDecl->getName()+"*/").str());

  // Visit all function call expression for the matchedFunctionDecl with unused parameter
  if (!functionCallVisitor) {
    functionCallVisitor = std::make_unique<FunctionCallVisitor>(*result.Context);
  }

  // Fix all declarations of matched function for parameter
  for (const FunctionDecl *fd : matchedFunctionDecl->redecls()) {
    SourceLocation paramLoc = fd->getParamDecl(paramIdx)->getLocation();
    diag(paramLoc, "Fixing parameter %0 in %1") << unusedParamDecl << fd->getNameInfo().getAsString()
      << removeParameter(result, fd, paramIdx);
  }

  for (const CallExpr *call : functionCallVisitor->getFunctionCalls(matchedFunctionDecl) ) {
    SourceLocation argLoc = call->getArg(paramIdx)->getExprLoc();
    diag(argLoc, "Fixing argument index %0 at call site  %1") << paramIdx << call->getDirectCallee()->getNameInfo().getAsString()
      << removeArgument(result, call, paramIdx);
  }

  return;
}


void UnusedParameterCustomCheck::check(const MatchFinder::MatchResult &result) {
  const auto *matchedFunctionDecl = result.Nodes.getNodeAs<FunctionDecl>("functionWithParams");

  // Check each parameter of function to see if it's used or referenced
  for (unsigned paramIdx = 0; paramIdx < matchedFunctionDecl->getNumParams(); paramIdx++) {
    const auto *paramDecl = matchedFunctionDecl->getParamDecl(paramIdx);

    if (paramDecl->isUsed() || paramDecl->isReferenced()) {
      continue;
    }
    warnAndFixUnusedParameter(result, matchedFunctionDecl, paramIdx);
  }
}

} // namespace clang::tidy::misc
