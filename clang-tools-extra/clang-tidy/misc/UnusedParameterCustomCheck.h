//===--- UnusedparametercustomCheck.h - clang-tidy --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNUSEDPARAMETERCUSTOMCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNUSEDPARAMETERCUSTOMCHECK_H

#include "../ClangTidyCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace clang::tidy::misc {

/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc/UnusedParameterCustom.html

/**
 *  A clang-tidy check that warns and fixes instances of
 *  unused parameters in function declarations along with
 *  arguments at those function call sites.
 *
 *  An unused parameter is defined as a parameter that has
 *  not been used or referenced in the body of a function
 *  declaration. Instances of unused parameters typically
 *  occur after a refactoring operation and can be cumbersome
 *  to track and fix manually. This includes fixes call sites
 *  that previous used the now unused parameter.
 */
class UnusedParameterCustomCheck : public ClangTidyCheck {
public:

  UnusedParameterCustomCheck(StringRef name, ClangTidyContext *context);
  ~UnusedParameterCustomCheck();
  
  void registerMatchers(ast_matchers::MatchFinder *finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &result) override;
  bool isLanguageVersionSupported(const LangOptions &langOpts) const override {
    return langOpts.CPlusPlus;
  }

private:

/** Implementation of RecursiveASTVisitor for function calls
 *
 * Based on the RecursiveASTVisitor, implements a
 * the custom logic that should occur when
 * the Abstract Syntax Tree (AST) is traversed
 * and lands on a CallExpr node.
 */
  class FunctionCallVisitor;
  std::unique_ptr<FunctionCallVisitor> functionCallVisitor;

/** Displays warnings and executes fixes for a given unused parameter
 * 
 * Given the function declaration and a parameter index, check if
 * the parameter is considered unused and is never referenced. If
 * it is considered so, output warning with a FixItHint diagnostic.
 *
 * NOTE: FixItHint diagnostics are only applied to source when
 * given permission too (--fix argument to clang-tidy)
 *
 * @param result the result of an AST matching query
 * @param matchedFunctionDecl function declaration with parameters to be checked
 * @param paramIdx index of the parameter being checked
 */
  void warnAndFixUnusedParameter(const ast_matchers::MatchFinder::MatchResult &result,
                                 const FunctionDecl *matchedFunctionDecl, unsigned paramIdx);

/** Create source removal transformation hint for call expression argument
 * 
 * For a given function call expression, create a source transformation hint
 * for the removal of an argument in the call expression.
 *
 * @param result the result of an AST matching query
 * @param call call expression targeted for modification
 * @param paramIdx index of the desired argument in the call expression
 */
  FixItHint removeArgument(const ast_matchers::MatchFinder::MatchResult &result,
                           const CallExpr *call, unsigned paramIdx);

/** Create source removal transformation hint for function declaration parameter
 * 
 * For a given function declaration, create a source transformation hint 
 * for the removal of a parameter in the function declaration.
 *
 * @param result the result of an AST matching query
 * @param func function declaration targeted for modification
 * @paramIdx index of the desired parameter in the function declaration
 */
  FixItHint removeParameter(const ast_matchers::MatchFinder::MatchResult &result,
                           const FunctionDecl *func, unsigned paramIdx);   
};

} // namespace clang::tidy::misc

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNUSEDPARAMETERCUSTOMCHECK_H
