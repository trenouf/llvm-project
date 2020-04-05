//===-- DeclarationInlineCommentCheck.cpp - clang-tidy --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Modifications Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// Notified per clause 4(b) of the license.
//
//===----------------------------------------------------------------------===//

#include "DeclarationInlineCommentCheck.h"
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
DeclarationInlineCommentCheck::DeclarationInlineCommentCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

// Store options
void DeclarationInlineCommentCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {}

// Register matchers
void DeclarationInlineCommentCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(cxxMethodDecl(isUserProvided()).bind("function"), this);
  Finder->addMatcher(functionDecl(unless(cxxMethodDecl())).bind("function"),
                    this);
  //Finder->addMatcher(varDecl().bind("vardecl"), this);
  //Finder->addMatcher(fieldDecl().bind("vardecl"), this);
  //Finder->addMatcher(enumConstantDecl().bind("vardecl"), this);
}

// Handle a match from one of the registered matchers
void DeclarationInlineCommentCheck::check(
    const MatchFinder::MatchResult &Result) {
  const SourceManager &SM = *Result.SourceManager;
  const ASTContext *Context = Result.Context;

  if (auto Func = Result.Nodes.getNodeAs<FunctionDecl>("function")) {
    // Function (or method) declaration.
    // Ignore any function at an invalid location or in a macro.
    if (!Func->getSourceRange().getBegin().isValid())
      return;
    if (Func->getSourceRange().getBegin().isMacroID())
      return;
    unsigned FuncIndent =
        std::max(
            SM.getPresumedLoc(Func->getSourceRange().getBegin()).getColumn(),
            1U) -
        1;

    std::string NewComment;
    SmallVector<SourceRange, 4> RemoveRanges;

    // Initial blank line (in comment) and regain function indent.
    NewComment += "//\n";
    NewComment.append(FuncIndent, ' ');

    // Scan each parameter checking for an inline comment.
    for (auto ParmIt : Func->parameters()) {
      ParmVarDecl *Parm = &*ParmIt;
      if (!Parm->getSourceRange().getEnd().isValid())
        continue;
      // Scan from the end of a single parameter.
      SourceRange RemoveRange;
      StringRef OldComment = findInlineComment(Parm->getSourceRange().getEnd(),
                                               SM, Context, RemoveRange);
      if (OldComment.empty())
        continue;
      RemoveRanges.push_back(RemoveRange);

      // Construct the replacement comment that will be placed just before the
      // function.
      NewComment += "// @param ";
      while (!OldComment.empty() && OldComment[0] == '/')
        OldComment = OldComment.slice(1, StringRef::npos);
      if (!OldComment.empty() && OldComment[0] == '<')
        OldComment = OldComment.slice(1, StringRef::npos);
      while (!OldComment.empty() && isWhitespace(OldComment[0]))
        OldComment = OldComment.slice(1, StringRef::npos);
      if (!OldComment.empty() && OldComment[0] == '[') {
        // Lex out a [out] etc tag. We add it to the new comment here, before
        // the parameter name, but only if it is not [in].
        auto EndTagIdx = OldComment.find(']');
        if (EndTagIdx != StringRef::npos) {
          StringRef Tag = OldComment.slice(0, EndTagIdx + 1);
          OldComment = OldComment.slice(EndTagIdx + 1, StringRef::npos);
          while (!OldComment.empty() && isWhitespace(OldComment[0]))
            OldComment = OldComment.slice(1, StringRef::npos);
          if (Tag != "[in]") {
            NewComment += Tag;
            NewComment += " ";
          }
        }
      }
      // Add the parameter name then ':'.
      const char *Name = SM.getCharacterData(Parm->getLocation());
      NewComment +=
          StringRef(Name, Lexer::MeasureTokenLength(Parm->getLocation(), SM,
                                                    Context->getLangOpts()));
      NewComment += " : ";
      // Add the rest of the comment, folding multiple lines.
      appendInlineComment(OldComment, NewComment);
      // Finish with a newline and re-indent the start of the function.
      NewComment += "\n";
      NewComment.append(FuncIndent, ' ');
    }
    if (!RemoveRanges.empty()) {
      auto Diag = diag(Func->getSourceRange().getBegin(),
                       "function or method with parameter inline comments");
      for (auto &RemoveRange : RemoveRanges)
        Diag << FixItHint::CreateRemoval(
            CharSourceRange::getCharRange(RemoveRange));
      Diag << FixItHint::CreateInsertion(Func->getSourceRange().getBegin(),
                                         NewComment);
    }
    return;
  }

  // File variable or field declaration. Look for an inline comment on the same
  // line as the end of the declaration.
  if (auto Decln = Result.Nodes.getNodeAs<Decl>("vardecl")) {
    if (auto VDecln = dyn_cast<VarDecl>(Decln)) {
      if (!VDecln->isFileVarDecl())
        return;
    }
    if (!Decln->getLocation().isValid() || Decln->getLocation().isMacroID())
      return;

    // Scan from the end of the decl.
    SourceRange RemoveRange;
    StringRef OldComment = findInlineComment(Decln->getSourceRange().getEnd(),
                                             SM, Context, RemoveRange);
    if (OldComment.empty())
      return;

    // We have an inline comment to move.
    std::string NewComment;
    appendInlineComment(OldComment, NewComment);
    // Finish with a newline and re-indent the start of the declaration.
    unsigned DeclIndent =
        std::max(
            SM.getPresumedLoc(Decln->getSourceRange().getBegin()).getColumn(),
            1U) -
        1;
    NewComment += "\n";
    NewComment.append(DeclIndent, ' ');

    // Report and apply fix-its.
    auto Diag =
        diag(RemoveRange.getBegin(), "variable or field inline comment");
    Diag << FixItHint::CreateRemoval(
        CharSourceRange::getCharRange(RemoveRange));
    Diag << FixItHint::CreateInsertion(Decln->getSourceRange().getBegin(),
                                       NewComment);
    return;
  }
}

// Starting at the given token location, scan for an inline comment on the same
// line. Returns a StringRef of the text of the comment (and further inline
// comments on subsequent lines, as long as the line contains only the comment),
// and sets CommentRange to a range used to remove the comment.
StringRef DeclarationInlineCommentCheck::findInlineComment(
    SourceLocation Loc, const SourceManager &SM, const ASTContext *Context,
    SourceRange &RemoveRange) {
  Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  for (;;) {
    auto CharData = SM.getCharacterData(Loc);
    if (!CharData || *CharData == '\n')
      return "";
    if (isWhitespace(*CharData)) {
      Loc = Loc.getLocWithOffset(1);
      continue;
    }
    SourceLocation EndLoc =
        Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
    if (memcmp(CharData, "//", 2)) {
      Loc = EndLoc;
      continue;
    }
    // We have found an inline comment after the parameter. Now check for
    // further lines with _only_ an inline comment to append.
    SourceLocation BeginLoc = Loc;
    Loc = EndLoc.getLocWithOffset(1);
    for (;;) {
      CharData = SM.getCharacterData(Loc);
      if (!CharData || *CharData == '\n')
        break;
      if (isWhitespace(*CharData)) {
        Loc = Loc.getLocWithOffset(1);
        continue;
      }
      if (memcmp(CharData, "//", 2))
        break;
      EndLoc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
      Loc = EndLoc.getLocWithOffset(1);
    }
    RemoveRange = SourceRange(BeginLoc, EndLoc);
    CharData = SM.getCharacterData(BeginLoc);
    return StringRef(CharData, SM.getCharacterData(EndLoc) - CharData);
  }
}

// Append the input StringRef to the string, removing any inline comment line
// continuations (NL followed by non-NL whitespace then "//").
// This also converts "///" or "///<" to "//".
void DeclarationInlineCommentCheck::appendInlineComment(
    StringRef OldComment, std::string &NewComment) {
  while (!OldComment.empty() && OldComment[0] == '/')
    OldComment = OldComment.slice(1, StringRef::npos);
  if (!OldComment.empty() && OldComment[0] == '<')
    OldComment = OldComment.slice(1, StringRef::npos);
  while (!OldComment.empty()) {
    auto NLIdx = OldComment.find('\n');
    if (NLIdx == StringRef::npos) {
      NewComment += OldComment;
      break;
    }
    NewComment += OldComment.slice(0, NLIdx);
    NewComment += " ";
    OldComment = OldComment.slice(NLIdx + 1, StringRef::npos);
    auto SlashIdx = OldComment.find('/');
    OldComment = OldComment.slice(SlashIdx + 2, StringRef::npos);
    while (!OldComment.empty() && isWhitespace(OldComment[0]))
      OldComment = OldComment.slice(1, StringRef::npos);
    while (!OldComment.empty() && OldComment[0] == '/')
      OldComment = OldComment.slice(1, StringRef::npos);
    if (!OldComment.empty() && OldComment[0] == '<')
      OldComment = OldComment.slice(1, StringRef::npos);
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
