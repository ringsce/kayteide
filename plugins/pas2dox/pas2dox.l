%{
// pas2dox.l - tokenizer for the Pascal/Delphi -> pseudo-C++ Doxygen filter.
//
// This file only tokenizes: it recognizes comments, string literals,
// structural punctuation, and the handful of bare keywords that mark
// section/mode boundaries (type/const/var/resourcestring/interface/
// implementation/private/public/protected/published/begin/"end."), and
// hands each one to a PasDoxFlexLexer::On*() method (see pasdoxflexlexer.h
// and pas2dox.cpp) which does the actual Pascal-to-pseudo-C++ conversion.
// Everything else is accumulated verbatim via OnCode() until a semicolon
// (at paren-depth 0) or a section/visibility keyword closes off the
// current declaration.
//
// The RAWBODY start condition handles Pascal statement bodies
// (begin...end): once entered, it just tracks begin/case/try/end nesting
// and copies everything through unchanged - the body's contents are hidden
// from Doxygen behind "#ifndef DOXYGEN_SKIP" anyway (see OnBeginBody() /
// RawEnd()), so there's nothing to gain from interpreting them.
#include "pasdoxflexlexer.h"
#include <string>
%}

%option c++
%option yyclass="PasDoxFlexLexer"
%option noyywrap nounistd case-insensitive
%option never-interactive

%x RAWBODY

%%

"{"[^}]*"}"                    { OnComment(std::string(yytext, yyleng)); }
"(*"([^*]|"*"+[^)*])*"*"+")"   { OnComment(std::string(yytext, yyleng)); }
"//".*                          { OnLineComment(std::string(yytext, yyleng)); }
'([^'\n]|'')*'                  { OnString(std::string(yytext, yyleng)); }

"("                              { OnOpenParen(); }
")"                              { OnCloseParen(); }
";"                              { OnSemicolon(); }

"interface"                      { OnHardBoundary("interface"); }
"implementation"                 { OnHardBoundary("implementation"); }

"type"                           { OnSectionKeyword("type"); }
"const"                          { OnSectionKeyword("const"); }
"var"                            { OnSectionKeyword("var"); }
"resourcestring"                 { OnSectionKeyword("resourcestring"); }

  /* Delphi's "strict private"/"strict protected" - the visibility keyword
     that follows is handled by its own rule below; "strict" itself carries
     no separate meaning for a Doxygen-facing declaration. */
"strict"                         { }

"private"                        { OnVisibility("private"); }
"public"                         { OnVisibility("public"); }
"protected"                      { OnVisibility("protected"); }
"published"                      { OnVisibility("published"); }

"begin"                          { OnBeginBody(); BEGIN(RAWBODY); }
"end"[ \t]*"."                   { OnEndOfProgram(); }
"end"                            { OnBareEnd(); }

  /* Must come after the keyword rules above: flex breaks length-ties by
     rule order, so a bare keyword (same length as itself, matched here
     too) still wins there - but a longer identifier that merely *contains*
     a keyword as a substring (e.g. "AConst", "TCustomItemDrawText") wins
     the maximal-munch comparison against this rule regardless of order,
     which is exactly what stops those keyword rules from firing mid-token. */
[A-Za-z_][A-Za-z0-9_]*           { OnCode(std::string(yytext, yyleng)); }

\n                               { OnCode("\n"); }
.                                { OnCode(std::string(yytext, yyleng)); }

<RAWBODY>{
"begin"                          { RawBeginLike(std::string(yytext, yyleng)); }
"case"                           { RawBeginLike(std::string(yytext, yyleng)); }
"try"                            { RawBeginLike(std::string(yytext, yyleng)); }
"end"                            { RawEnd(); if (m_rawBodyDepth <= 0) BEGIN(INITIAL); }
  /* Same maximal-munch protection as INITIAL's identifier rule - stops
     e.g. "EndValue" or "MyCase" from being misread as the bare keywords
     above. */
[A-Za-z_][A-Za-z0-9_]*           { RawEmit(std::string(yytext, yyleng)); }
\n                               { RawEmit("\n"); }
.                                { RawEmit(std::string(yytext, yyleng)); }
}

<<EOF>>                          { Flush(); yyterminate(); }

%%
