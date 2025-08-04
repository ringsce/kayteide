#ifndef PASDOXFLEXLEXER_H
#define PASDOXFLEXLEXER_H

#include <FlexLexer.h> // Essential for inheriting from yyFlexLexer
#include <string>      // For std::string
#include <iostream>    // For std::istream, std::ostream

// Forward declarations of any custom classes/structs used in member function signatures
// or as member variables, if their full definition is not in this header.
// E.g., if func_info, param_info etc. are custom types used within PasDoxFlexLexer methods:
// class func_info;
// struct param_info;
// ... (add other necessary forward declarations) ...

class PasDoxFlexLexer : public yyFlexLexer {
public:
    // Constructor declaration
    PasDoxFlexLexer(std::istream* in = nullptr, std::ostream* out = nullptr);
    virtual ~PasDoxFlexLexer(); // Declare destructor

    // Member function declarations
    // If these functions are meant to be pure virtual or overridden from FlexLexer/yyFlexLexer,
    // ensure their signatures match the base class.
    // Based on your previous context, these seem to be custom methods.
    std::string Convert_Type(std::string s);
    std::string RemoveSpaces(std::string s);
    std::string Convert_Op(std::string s);
    void DisplayEnum();
    void RemoveParams(std::string s);
    void ProcessParameterType();
    void ProcessComment();
    void ProcessEnd();
    void OutputInheritedClass();
    void ProcessEnums();
    void ProcessArrays();
    void ProcessFunction();
    void DisplayParams();
    void DisplayVariables();
    std::string ChangeStrings(std::string s);
    void OutputLog(const char* s, bool newline = true);
    void OutputLog2(const char* s1, const char* s2);
    void OutputLogNum(const char* s, int num);
    void OutputLogHead(const char* s);
    void PushText(const std::string& txt); // Takes std::string
    void version_info(); // Declaration

    // If yylex() is overridden, declare it here
    virtual int yylex();

    // If you override these (LexerInput, LexerOutput, LexerError, yywrap, switch_streams, etc.)
    // from yyFlexLexer, declare them as 'virtual' here with matching signatures:
    // virtual int LexerInput(char* buf, int max_size);
    // virtual void LexerOutput(const char* buf, int size);
    // virtual void LexerError(const char* msg);
    // virtual int yywrap();
    // virtual void switch_streams(std::istream* new_in = 0, std::ostream* new_out = 0);
    // etc.
};

#endif // PASDOXFLEXLEXER_H