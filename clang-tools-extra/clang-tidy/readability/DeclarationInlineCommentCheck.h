//===--- DeclarationInlineCommentCheck.h clang-tidy -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_DECLARATION_INLINE_COMMENT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_DECLARATION_INLINE_COMMENT_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace readability {

/// Looks for a variable, member, enum constant or parameter declaration with an
/// inline comment on the same line.
///
/// This is used when converting from a coding style where a parameter in a
/// function declaration, or a class/struct field declaration, or a variable
/// declaration, or enum constant, is commented with an inline comment after it.
/// Typically, such comments are aligned to each other, which causes spurious
/// whitespace changes when a parameter or member is changed or added in such a
/// way that all comments need realigning. This check provides a way of
/// converting from that style into a style where the comment is before the
/// declaration.
///
/// This check spots such a comment, and applies a fix-it as follows:
/// - A comment on a parameter is moved to just before the function
///   declaration, and indented the same as it. The new comment consists of:
///   - '\param';
///   - the '[in/out]' etc tag from the original comment (omitted if it was
///     '[in]');
///   - the parameter name;
///   - ':'
///   - the rest of the original comment.
/// - A comment on a member or variable is moved to just before the declaration,
///   and indented the same as it.
///
/// Where such a comment continues onto subsequent lines that contain only an
/// inline comment, the lines are joined together. The idea is that you then use
/// clang-format to reimpose a line length limit.
class DeclarationInlineCommentCheck : public ClangTidyCheck {
public:
  DeclarationInlineCommentCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Options) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  StringRef findInlineComment(SourceLocation Loc, const SourceManager &SM,
                              const ASTContext *Context,
                              SourceRange &RemoveRange);
  void appendInlineComment(StringRef OldComment, std::string &NewString);
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_DECLARATION_INLINE_COMMENT_H
