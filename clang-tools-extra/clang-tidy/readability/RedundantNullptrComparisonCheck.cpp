//===-- RedundantNullptrComparisonCheck.cpp - clang-tidy ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Modifications Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//

#include "RedundantNullptrComparisonCheck.h"
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
RedundantNullptrComparisonCheck::RedundantNullptrComparisonCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

// Store options
void RedundantNullptrComparisonCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {}

// Register matchers for "== nullptr" and "!= nullptr"
void RedundantNullptrComparisonCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      binaryOperator(hasOperatorName("=="),
                     hasRHS(ignoringImpCasts(cxxNullPtrLiteralExpr())))
          .bind("equals-nullptr"),
      this);
  Finder->addMatcher(
      binaryOperator(hasOperatorName("!="),
                     hasRHS(ignoringImpCasts(cxxNullPtrLiteralExpr())))
          .bind("not-equals-nullptr"),
      this);
}

// Handle a match from one of the registered matchers
void RedundantNullptrComparisonCheck::check(
    const MatchFinder::MatchResult &Result) {

  // Check for "== nullptr". The fix-it prepends a "!" to the LHS. If the LHS is
  // a binary or ternary operator, then it needs parentheses adding.
  if (const auto *BinOp =
          Result.Nodes.getNodeAs<BinaryOperator>("equals-nullptr")) {
    bool NeedParen = false;
    if (auto LHSOp = dyn_cast<CXXOperatorCallExpr>(BinOp->getLHS()))
      NeedParen = LHSOp->isInfixBinaryOp();
    else {
      NeedParen = isa<BinaryOperator>(BinOp->getLHS()) ||
                  isa<ConditionalOperator>(BinOp->getLHS());
    }
    SourceRange RemoveNullptr(BinOp->getOperatorLoc(),
                              BinOp->getRHS()->getEndLoc());

    auto Diag = diag(BinOp->getOperatorLoc(),
                     "redundant '==' comparison with nullptr; use '!'");
    if (NeedParen) {
      Diag << FixItHint::CreateInsertion(BinOp->getLHS()->getBeginLoc(), "!(");
      Diag << FixItHint::CreateReplacement(RemoveNullptr, ")");
    } else {
      Diag << FixItHint::CreateInsertion(BinOp->getLHS()->getBeginLoc(), "!");
      Diag << FixItHint::CreateRemoval(RemoveNullptr);
    }
    return;
  }

  // Check for "!= nullptr".
  if (const auto *BinOp =
          Result.Nodes.getNodeAs<BinaryOperator>("not-equals-nullptr")) {
    SourceRange RemoveNullptr(BinOp->getOperatorLoc(),
                              BinOp->getRHS()->getEndLoc());

    auto Diag =
        diag(BinOp->getOperatorLoc(), "redundant '!=' comparison with nullptr");
    Diag << FixItHint::CreateRemoval(RemoveNullptr);
    return;
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
