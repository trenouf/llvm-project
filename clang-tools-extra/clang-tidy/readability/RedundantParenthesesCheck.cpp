//===-- RedundantParenthesesCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RedundantParenthesesCheck.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Lexer.h"

#include <cassert>
#include <string>
#include <utility>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

// Constructor: read any options
RedundantParenthesesCheck::RedundantParenthesesCheck(StringRef Name,
                                                     ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

// Store options
void RedundantParenthesesCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {}

// Register matchers
void RedundantParenthesesCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("&&"), hasOperatorName("||")))
          .bind("and-or-paren"),
      this);
  Finder->addMatcher(binaryOperator(hasOperatorName("="))
                         .bind("assign-paren"),
                     this);
  Finder->addMatcher(conditionalOperator().bind("conditional"), this);
  Finder->addMatcher(
      returnStmt(hasReturnValue(parenExpr())).bind("return-paren"), this);
}

// Handle a match from one of the registered matchers
void RedundantParenthesesCheck::check(const MatchFinder::MatchResult &Result) {

  // Check for binary '&&' or '||' where an operand has redundant parentheses.
  if (const auto *BinOp =
          Result.Nodes.getNodeAs<BinaryOperator>("and-or-paren")) {
    checkParentheses(BinOp->getLHS(), true);
    checkParentheses(BinOp->getRHS(), true);
    return;
  }

  // Check for assignment where the RHS has redundant parentheses.
  if (const auto *BinOp =
          Result.Nodes.getNodeAs<BinaryOperator>("assign-paren")) {
    checkParentheses(BinOp->getRHS(), true);
    return;
  }

  // Check for conditional operator '?:' where any operand has redundant
  // parentheses.
  if (const auto *CondOp =
          Result.Nodes.getNodeAs<ConditionalOperator>("conditional")) {
    checkParentheses(CondOp->getCond(), false);
    checkParentheses(CondOp->getTrueExpr(), false);
    checkParentheses(CondOp->getFalseExpr(), false);
    return;
  }

  // Check for return statement where the return value has redundant
  // parentheses.
  if (const auto *RetSt = Result.Nodes.getNodeAs<ReturnStmt>("return-paren")) {
    checkParentheses(RetSt->getRetValue(), false);
    return;
  }
}

// Check an expression for having redundant parentheses.
//
// Parentheses around a comparison binary operator are always counted as
// redundant. Thus this can only be called in a situation where the
// parenthesized expression is not a subexpression, or it is an operand to a
// binary operator of lower precedence than a comparison.
//
// Parentheses around a logical binary operator are counted as redundant only if
// the parenthesized expression does not appear in another expression.
//
// FIXME: This whole check could be enhanced to properly take into account
// operator precedence, and then take into account which combinations of
// operators are seen as needing theoretically redundant parentheses by warnings
// on popular C/C++ compilers.
void RedundantParenthesesCheck::checkParentheses(const Expr *E, bool InExpr) {
  auto ParenE = dyn_cast<ParenExpr>(E->IgnoreImpCasts());
  if (!ParenE)
    return;
  E = ParenE->getSubExpr();

  bool RemoveParen = false;
  if (auto Op = dyn_cast<CXXOperatorCallExpr>(E)) {
    if (Op->isInfixBinaryOp()) {
      // Remove parentheses if the overloaded binary operator is a comparison.
      auto Opc = Op->getOperator();
      RemoveParen = Opc == OO_EqualEqual || Opc == OO_ExclaimEqual ||
                    Opc == OO_LessEqual || Opc == OO_GreaterEqual ||
                    Opc == OO_Less || Opc == OO_Greater;
      // If this ParenExpr is not in a '&&' or '||' expression, also remove
      // parentheses if the overloaded binary operator is '&&' or '||'.
      if (!InExpr)
        RemoveParen |= Opc == OO_AmpAmp || Opc == OO_PipePipe;
    } else {
      // Remove parentheses if the overloaded operator is not binary.
      RemoveParen = true;
    }
  } else if (auto Op = dyn_cast<BinaryOperator>(E)) {
    // Remove parentheses if the binary operator is a comparison.
    RemoveParen = Op->isComparisonOp();
    // If this ParenExpr is not in a '&&' or '||' expression, also remove
    // parentheses if binary operator is '&&' or '||'.
    if (!InExpr)
      RemoveParen |= Op->isLogicalOp();
  } else
    RemoveParen = !isa<ConditionalOperator>(E);

  if (!RemoveParen)
    return;

  auto Diag = diag(ParenE->getLParen(), "redundant parentheses");
  Diag << FixItHint::CreateRemoval(ParenE->getLParen());
  Diag << FixItHint::CreateRemoval(ParenE->getRParen());
}

} // namespace readability
} // namespace tidy
} // namespace clang
