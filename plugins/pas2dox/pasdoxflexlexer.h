#ifndef PASDOXFLEXLEXER_H
#define PASDOXFLEXLEXER_H

// flex's C++ scanner support is, in its own header's words, "a mess": the
// generated .cpp (from pas2dox.l) unconditionally does its own
// `#include <FlexLexer.h>` (guarded by `#define yyFlexLexer yyFlexLexer`,
// i.e. a self-identity no-op rename) to pull in the concrete `yyFlexLexer`
// class *and* to supply the out-of-line bodies for LexerInput/LexerOutput/
// yy_create_buffer/etc. that FlexLexer.h only declares. Those bodies are
// only ever emitted under the literal name "yyFlexLexer" - not under
// whatever %option yyclass names - so our class must inherit from that
// exact class, not a renamed copy of it (renaming here just produces a
// second, body-less class and undefined-symbol link errors).
//
// The only real problem is that plainly `#include <FlexLexer.h>` again
// here would re-enter FlexLexer.h's own class-definition block (guarded by
// `defined(yyFlexLexer) || !defined(yyFlexLexerOnce)`) a second time while
// that macro is still self-defined, redefining `yyFlexLexer` and failing to
// compile. `#undef` before including clears that, so this second
// inclusion is a no-op and reuses the class flex's generated file already
// defined.
#undef yyFlexLexer
#include <FlexLexer.h>

#include <string>
#include <iostream>

// Pascal/Delphi -> pseudo-C++ filter for Doxygen.
//
// This does not produce a faithful C++ translation - it produces just
// enough C++-shaped syntax (classes, records-as-structs, enums,
// function/procedure prototypes, fields, properties, converted comments)
// for Doxygen's C++ parser to pick up the declarations and whatever
// documentation comments were attached to them in the original Pascal
// source. Statement bodies (begin...end) are copied through unmodified but
// wrapped in "#ifndef DOXYGEN_SKIP" / "#endif" so Doxygen's own
// preprocessor hides them instead of choking on Pascal executable syntax it
// can't parse as C++.
//
// Intended usage is as a Doxygen INPUT_FILTER / FILTER_PATTERNS program:
// given a single file path, it writes the converted text to stdout - see
// main() in pas2dox.cpp.
class PasDoxFlexLexer : public yyFlexLexer {
public:
    explicit PasDoxFlexLexer(std::istream *in = nullptr, std::ostream *out = nullptr);
    ~PasDoxFlexLexer() override;

    // The flex-generated scanner (from pas2dox.l, via %option yyclass).
    int yylex() override;

    // Flushes whatever statement text is still pending. Called once after
    // yylex() returns (at EOF) - also safe to call defensively from within
    // lexer actions before a hard section boundary.
    void Flush();

    // ── Called directly from the flex actions in pas2dox.l ──────────────
    void OnComment(const std::string &raw);      // {*...}  {...}  (*...*)
    void OnLineComment(const std::string &raw);  // // ...  and /// ...
    void OnString(const std::string &raw);       // 'Pascal string literal'
    void OnOpenParen();
    void OnCloseParen();
    void OnSemicolon();
    void OnHardBoundary(const std::string &kw);  // interface / implementation
    void OnSectionKeyword(const std::string &kw);// type / const / var / resourcestring
    void OnVisibility(const std::string &kw);    // private/public/protected/published
    void OnBeginBody();                          // top-level "begin"
    void OnBareEnd();                            // "end" closing a class/record (or an empty one)
    void OnEndOfProgram();                       // "end."
    void OnCode(const std::string &text);        // catch-all accumulation

    // ── RAWBODY start-condition actions (Pascal statement bodies) ───────
    void RawBeginLike(const std::string &kw);    // nested begin/case/try
    void RawEnd();                               // "end" - may close the body
    void RawEmit(const std::string &text);

    // Pascal type spelling -> pseudo-C++ type spelling. Deliberately mostly
    // a passthrough (Doxygen doesn't need real type resolution, just a
    // syntactically plausible token) with a couple of small conveniences:
    // "^Foo" and "array of Foo" both become "Foo*".
    static std::string Convert_Type(std::string s);

private:
    void FlushStatement(std::string stmt);

    void EmitClassOrRecordOrEnumOrAlias(const std::string &stmt);
    void EmitClassMember(const std::string &stmt);
    void EmitRecordField(const std::string &stmt);
    void EmitConstOrResourceString(const std::string &stmt);
    void EmitTopLevelDecl(const std::string &stmt);

    static std::string ConvertCommentText(const std::string &raw);
    static std::string ConvertParamList(const std::string &parenText);

    // Persistent parser state - spans multiple yylex() action invocations,
    // since a single Pascal declaration/statement can be split across many
    // tokens (and even interrupted mid-way by a comment token).
    std::string m_stmt;
    int m_parenDepth = 0;

    bool m_inClassBody  = false;
    bool m_inRecordBody = false;
    std::string m_currentClassName;

    enum class Section { Top, Type, Const, Var, ResourceString };
    Section m_section = Section::Top;

    // Raw passthrough for Pascal statement bodies (begin...end), hidden
    // from Doxygen behind #ifndef DOXYGEN_SKIP.
    int m_rawBodyDepth = 0;
};

#endif // PASDOXFLEXLEXER_H
