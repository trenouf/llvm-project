//===--- BracesAroundStatementsCheck.h - clang-tidy -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Modifications Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_BRACESAROUNDSTATEMENTSCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_BRACESAROUNDSTATEMENTSCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace readability {

/// Checks that bodies of `if` statements and loops (`for`, `range-for`,
/// `do-while`, and `while`) are inside braces
///
/// Before:
///
/// \code
///   if (condition)
///     statement;
/// \endcode
///
/// After:
///
/// \code
///   if (condition) {
///     statement;
///   }
/// \endcode
///
/// Additionally, one can define an option `ShortStatementLines` defining the
/// minimal number of lines that the statement should have in order to trigger
/// this check.
///
/// The number of lines is counted from the end of condition or initial keyword
/// (`do`/`else`) until the last line of the inner statement.  Default value 0
/// means that braces will be added to all statements (not having them already).
class BracesAroundStatementsCheck : public ClangTidyCheck {
public:
  BracesAroundStatementsCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void onEndOfTranslationUnit() override;

private:
  bool checkStmt(const ast_matchers::MatchFinder::MatchResult &Result,
                 const Stmt *S, SourceLocation StartLoc,
                 SourceLocation EndLocHint = SourceLocation());
  bool checkSimpleStmt(const ast_matchers::MatchFinder::MatchResult &Result,
                       const Stmt *S, SourceLocation StartLoc,
                       SourceLocation EndLocHint);
  void checkCompoundStmt(const ast_matchers::MatchFinder::MatchResult &Result,
                         const CompoundStmt *S, SourceLocation InitialLoc,
                         SourceLocation EndLocHint);
  template <typename IfOrWhileStmt>
  SourceLocation findRParenLoc(const IfOrWhileStmt *S, const SourceManager &SM,
                               const ASTContext *Context);

private:
  std::set<const Stmt *> ForceBracesStmts;
  const unsigned ShortStatementLines;
  const bool AddBraces;
  const bool RemoveUnnecessaryBraces;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_BRACESAROUNDSTATEMENTSCHECK_H
