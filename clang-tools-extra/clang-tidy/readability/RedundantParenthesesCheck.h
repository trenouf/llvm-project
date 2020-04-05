//===--- RedundantParenthesesCheck.h clang-tidy -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Modifications Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_REDUNDANT_PARENTHESES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_REDUNDANT_PARENTHESES_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace readability {

/// Looks for specific cases of redundant parentheses.
class RedundantParenthesesCheck : public ClangTidyCheck {
public:
  RedundantParenthesesCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Options) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  void checkParentheses(const Expr *E, bool InExpr);
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_REDUNDANT_PARENTHESES_H
