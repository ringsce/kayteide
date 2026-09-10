#include "pasdoxflexlexer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

namespace {

const char *kVersion = "1.0.0";

void PrintVersionInfo() {
    std::cout << "pas2dox " << kVersion << "\n"
              << "Pascal/Delphi -> pseudo-C++ filter for Doxygen.\n"
              << "Originally by Darren Bowles (2002-2003); rewritten for KayteIDE.\n";
}

void PrintUsage() {
    std::cerr << "Usage: pas2dox [options] <file.pas>\n"
              << "Converts Pascal/Delphi source into pseudo-C++ so Doxygen can parse it.\n"
              << "Intended for use as a Doxygen INPUT_FILTER / FILTER_PATTERNS program:\n"
              << "reads <file.pas> and writes the converted text to stdout. With no file\n"
              << "argument, reads from stdin instead.\n"
              << "  -v, --version   print version information\n"
              << "  -h, --help      show this help\n";
}

std::string Trim(const std::string &s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool IEquals(const std::string &a, const std::string &b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

bool IStartsWith(const std::string &s, const std::string &prefix) {
    if (s.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i)
        if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)prefix[i])) return false;
    return true;
}

// Splits on `delim`, but only where paren/bracket nesting depth is 0 - so a
// parameter list's own internal grouping isn't broken by a comma/semicolon
// that belongs to a nested "(...)"/"[...]".
std::vector<std::string> SplitTopLevel(const std::string &s, char delim) {
    std::vector<std::string> out;
    int depth = 0;
    std::string cur;
    for (char c : s) {
        if (c == '(' || c == '[') depth++;
        else if (c == ')' || c == ']') depth--;
        if (c == delim && depth <= 0) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

std::string SwapQuotes(const std::string &s) {
    // Pascal string literals use single quotes; the closest thing to a
    // faithful, context-free conversion is a straight '->" swap, including
    // Pascal's ''-doubled-quote escape (which just becomes "" - harmless
    // for Doxygen's purposes, it never evaluates these as real strings).
    std::string out = s;
    std::replace(out.begin(), out.end(), '\'', '"');
    return out;
}

bool IsBareModifier(const std::string &s) {
    static const std::regex re(
        R"(^(virtual|override|abstract|dynamic|overload|reintroduce|cdecl|register|stdcall|safecall|inline|export)$)",
        std::regex::icase);
    return std::regex_match(s, re);
}

bool IsCommentOnly(const std::string &s) {
    static const std::regex re(R"(^(\s*(/\*[\s\S]*?\*/|//[^\n]*)\s*)+$)");
    return !s.empty() && std::regex_match(s, re);
}

} // namespace

PasDoxFlexLexer::PasDoxFlexLexer(std::istream *in, std::ostream *out)
    : yyFlexLexer(in, out)
{
}

PasDoxFlexLexer::~PasDoxFlexLexer() = default;

// ─────────────────────────────────────────────────────────────────────────
// Comment / string conversion
// ─────────────────────────────────────────────────────────────────────────

std::string PasDoxFlexLexer::ConvertCommentText(const std::string &raw) {
    std::string inner;
    if (!raw.empty() && raw.front() == '{') {
        inner = raw.substr(1, raw.size() - 2);
    } else { // "(* ... *)"
        inner = raw.substr(2, raw.size() - 4);
    }

    bool isDoc = false, trailing = false;
    if (!inner.empty() && inner.front() == '*') {
        isDoc = true;
        inner.erase(0, 1);
        if (!inner.empty() && inner.front() == '<') {
            trailing = true;
            inner.erase(0, 1);
        }
    }

    if (isDoc) return trailing ? ("/**<" + inner + " */") : ("/**" + inner + " */");
    return "/*" + inner + " */";
}

void PasDoxFlexLexer::OnComment(const std::string &raw) {
    std::string converted = ConvertCommentText(raw);
    if (m_parenDepth == 0 && Trim(m_stmt).empty())
        std::cout << converted << "\n";
    else
        m_stmt += converted;
}

void PasDoxFlexLexer::OnLineComment(const std::string &raw) {
    if (m_parenDepth == 0 && Trim(m_stmt).empty())
        std::cout << raw << "\n";
    else
        m_stmt += raw;
}

void PasDoxFlexLexer::OnString(const std::string &raw) {
    m_stmt += SwapQuotes(raw);
}

// ─────────────────────────────────────────────────────────────────────────
// Structural tokens
// ─────────────────────────────────────────────────────────────────────────

void PasDoxFlexLexer::OnOpenParen() { m_parenDepth++; m_stmt += "("; }
void PasDoxFlexLexer::OnCloseParen() { if (m_parenDepth > 0) m_parenDepth--; m_stmt += ")"; }

void PasDoxFlexLexer::OnSemicolon() {
    m_stmt += ";";
    if (m_parenDepth <= 0) {
        std::string s = m_stmt;
        m_stmt.clear();
        m_parenDepth = 0;
        FlushStatement(s);
    }
}

void PasDoxFlexLexer::OnCode(const std::string &text) { m_stmt += text; }

void PasDoxFlexLexer::Flush() {
    if (Trim(m_stmt).empty()) { m_stmt.clear(); return; }
    std::string s = m_stmt;
    m_stmt.clear();
    m_parenDepth = 0;
    FlushStatement(s);
}

void PasDoxFlexLexer::OnHardBoundary(const std::string &kw) {
    if (m_parenDepth > 0) { m_stmt += kw; return; }
    Flush(); // "interface" / "implementation": nothing else to emit
}

void PasDoxFlexLexer::OnSectionKeyword(const std::string &kw) {
    // "const"/"var" (and, in principle, any of these) can also appear as a
    // parameter-passing modifier inside a parameter list, e.g.
    // "function Realloc(var NewCapacity: Longint)" - that's not a section
    // boundary at all, just ordinary code to accumulate.
    if (m_parenDepth > 0) { m_stmt += kw; return; }

    Flush();
    if (IEquals(kw, "type")) m_section = Section::Type;
    else if (IEquals(kw, "const")) m_section = Section::Const;
    else if (IEquals(kw, "var")) m_section = Section::Var;
    else if (IEquals(kw, "resourcestring")) m_section = Section::ResourceString;
}

void PasDoxFlexLexer::OnVisibility(const std::string &kw) {
    if (m_parenDepth > 0) { m_stmt += kw; return; }
    Flush();
    std::string lower = kw;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    std::cout << lower << ":\n";
}

void PasDoxFlexLexer::OnBeginBody() {
    Flush();
    std::cout << "#ifndef DOXYGEN_SKIP\n{\n";
    m_rawBodyDepth = 1;
}

void PasDoxFlexLexer::OnBareEnd() {
    if (m_parenDepth > 0) { m_stmt += "end"; return; }

    if (!m_inClassBody && !m_inRecordBody) {
        // Might be closing a body whose header is still sitting unflushed
        // in m_stmt (e.g. "TMyClass = class" with no members at all, so no
        // visibility keyword ever showed up to force the header out) -
        // flushing now gives EmitClassOrRecordOrEnumOrAlias a chance to
        // recognize it and set m_inClassBody/m_inRecordBody.
        Flush();
    }

    if (m_inClassBody || m_inRecordBody) {
        std::cout << "};\n";
        m_inClassBody = false;
        m_inRecordBody = false;
    }
    // Otherwise: a bare "end" with nothing recognized as an open body -
    // stray/malformed input; nothing sensible to emit.
}

void PasDoxFlexLexer::OnEndOfProgram() {
    Flush();
    std::cout << "\n// finished\n";
}

void PasDoxFlexLexer::RawBeginLike(const std::string &kw) {
    m_rawBodyDepth++;
    std::cout << kw;
}

void PasDoxFlexLexer::RawEnd() {
    m_rawBodyDepth--;
    if (m_rawBodyDepth <= 0) {
        std::cout << "}\n#endif /* DOXYGEN_SKIP */\n";
    } else {
        std::cout << "end";
    }
}

void PasDoxFlexLexer::RawEmit(const std::string &text) { std::cout << text; }

// ─────────────────────────────────────────────────────────────────────────
// Type conversion
// ─────────────────────────────────────────────────────────────────────────

std::string PasDoxFlexLexer::Convert_Type(std::string s) {
    s = Trim(s);
    if (!s.empty() && s.front() == '^') return Trim(s.substr(1)) + "*";
    static const std::regex arrOfRe(R"(^array\s+of\s+(.+)$)", std::regex::icase);
    std::smatch m;
    if (std::regex_match(s, m, arrOfRe)) return Trim(m[1].str()) + "*";
    return s;
}

// ─────────────────────────────────────────────────────────────────────────
// Parameter list conversion: "(var A: T1; const B, C: T2)" -> "(T1 &A, const T2 B, const T2 C)"
// ─────────────────────────────────────────────────────────────────────────

std::string PasDoxFlexLexer::ConvertParamList(const std::string &parenText) {
    std::string inner = Trim(parenText);
    if (inner.size() >= 2 && inner.front() == '(' && inner.back() == ')')
        inner = inner.substr(1, inner.size() - 2);
    inner = Trim(inner);
    if (inner.empty()) return "()";

    std::vector<std::string> outParams;
    std::vector<std::string> pendingNames;

    auto flushPending = [&](const std::string &type, bool isVar, bool isConst) {
        std::string t = Convert_Type(Trim(type));
        for (auto &nm : pendingNames) {
            std::string decl = (isConst ? "const " : "") + t + " " + (isVar ? "&" : "") + Trim(nm);
            outParams.push_back(decl);
        }
        pendingNames.clear();
    };

    for (auto &group : SplitTopLevel(inner, ';')) {
        for (auto &piece0 : SplitTopLevel(group, ',')) {
            std::string p = Trim(piece0);
            if (p.empty()) continue;
            bool isVar = false, isConst = false;
            if (IStartsWith(p, "var ")) { isVar = true; p = Trim(p.substr(4)); }
            else if (IStartsWith(p, "const ")) { isConst = true; p = Trim(p.substr(6)); }
            else if (IStartsWith(p, "out ")) { isVar = true; p = Trim(p.substr(4)); }

            size_t colon = p.find(':');
            if (colon == std::string::npos) {
                pendingNames.push_back(p);
            } else {
                pendingNames.push_back(Trim(p.substr(0, colon)));
                // Drop a trailing "= default" initializer, if any.
                std::string type = p.substr(colon + 1);
                size_t eq = type.find('=');
                if (eq != std::string::npos) type = type.substr(0, eq);
                flushPending(type, isVar, isConst);
            }
        }
    }
    for (auto &nm : pendingNames) outParams.push_back(Trim(nm)); // malformed leftovers, kept rather than dropped

    std::string result = "(";
    for (size_t i = 0; i < outParams.size(); ++i) {
        if (i) result += ", ";
        result += outParams[i];
    }
    return result + ")";
}

// ─────────────────────────────────────────────────────────────────────────
// Statement dispatch
// ─────────────────────────────────────────────────────────────────────────

void PasDoxFlexLexer::FlushStatement(std::string raw) {
    std::string stmt = Trim(raw);
    if (stmt.empty()) return;

    if (IsCommentOnly(stmt)) { std::cout << stmt << "\n"; return; }

    if (!stmt.empty() && stmt.back() == ';') stmt.pop_back();
    stmt = Trim(stmt);
    if (stmt.empty()) return;

    if (IStartsWith(stmt, "unit ") || IEquals(stmt, "uses") || IStartsWith(stmt, "uses "))
        return; // unit/uses declarations carry nothing doc-worthy

    // OnBareEnd() is the primary path for closing a class/record body (it
    // fires the moment "end" is tokenized, not waiting for a semicolon) -
    // these IStartsWith("end") checks are just a defensive fallback for
    // "end" arriving glued to other text via some path that bypassed it.
    if (m_inClassBody) {
        if (IStartsWith(stmt, "end")) {
            std::string trailing = Trim(stmt.substr(3));
            std::cout << "};\n";
            if (!trailing.empty()) std::cout << trailing << "\n";
            m_inClassBody = false;
            // Deliberately NOT resetting m_section here: Pascal's "type"
            // section holds many declarations under one keyword, so the
            // next flushed statement (another class/record/enum/alias) is
            // still part of the same type section, not top-level code.
            return;
        }
        EmitClassMember(stmt);
        return;
    }

    if (m_inRecordBody) {
        if (IStartsWith(stmt, "end")) {
            std::string trailing = Trim(stmt.substr(3));
            std::cout << "};\n";
            if (!trailing.empty()) std::cout << trailing << "\n";
            m_inRecordBody = false;
            return; // see the m_inClassBody case above
        }
        EmitRecordField(stmt);
        return;
    }

    switch (m_section) {
        case Section::Const:
        case Section::ResourceString:
            EmitConstOrResourceString(stmt);
            return;
        case Section::Type:
            EmitClassOrRecordOrEnumOrAlias(stmt);
            return;
        case Section::Var:
        case Section::Top:
        default:
            EmitTopLevelDecl(stmt);
            return;
    }
}

void PasDoxFlexLexer::EmitClassOrRecordOrEnumOrAlias(const std::string &stmt) {
    static const std::regex classRe(R"(^(\w+)\s*=\s*class\s*(\(\s*(\w+)\s*\))?\s*$)", std::regex::icase);
    static const std::regex recordRe(R"(^(\w+)\s*=\s*record\b)", std::regex::icase);
    static const std::regex enumRe(R"(^(\w+)\s*=\s*\(([\s\S]*)\)\s*$)");
    static const std::regex setOfRe(R"(^(\w+)\s*=\s*set\s+of\s*\(([\s\S]*)\)\s*$)", std::regex::icase);
    static const std::regex funcTypeRe(
        R"(^(\w+)\s*=\s*function\s*(\([\s\S]*?\))?\s*(:\s*(\w+))?\s*of\s+object\s*$)", std::regex::icase);
    static const std::regex aliasRe(R"(^(\w+)\s*=\s*([\s\S]+)$)");
    std::smatch m;

    if (std::regex_match(stmt, m, classRe)) {
        m_currentClassName = m[1].str();
        std::string parent = m[3].matched ? (" : public " + m[3].str()) : "";
        std::cout << "class " << m_currentClassName << parent << "\n{\n";
        m_inClassBody = true;
        return;
    }
    if (std::regex_search(stmt, m, recordRe)) {
        m_currentClassName = m[1].str();
        std::cout << "struct " << m_currentClassName << "\n{\n";
        m_inRecordBody = true;
        return;
    }
    if (std::regex_match(stmt, m, setOfRe) || std::regex_match(stmt, m, enumRe)) {
        std::string name = m[1].str();
        std::string body = m[2].str();
        std::cout << "enum " << name << "\n{\n";
        auto members = SplitTopLevel(body, ',');
        for (size_t i = 0; i < members.size(); ++i) {
            std::string mem = Trim(members[i]);
            if (mem.empty()) continue;
            std::cout << mem << (i + 1 < members.size() ? "," : "") << "\n";
        }
        std::cout << "};\n";
        return;
    }
    if (std::regex_match(stmt, m, funcTypeRe)) {
        std::string ret = m[4].matched ? Convert_Type(m[4].str()) : "void";
        std::cout << "typedef " << ret << " (*" << m[1].str() << ")();\n";
        return;
    }
    if (std::regex_match(stmt, m, aliasRe)) {
        std::cout << "typedef " << Trim(m[2].str()) << " " << m[1].str() << ";\n";
        return;
    }
    std::cout << "/* pas2dox: unrecognized type declaration: " << stmt << " */\n";
}

void PasDoxFlexLexer::EmitClassMember(const std::string &stmt) {
    if (IsBareModifier(stmt)) return;

    static const std::regex dtorRe(R"(^destructor\s+\w+\b)", std::regex::icase);
    static const std::regex ctorRe(R"(^constructor\s+\w+\s*(\([\s\S]*\))?)", std::regex::icase);
    static const std::regex propRe(
        R"(^property\s+(\w+)\s*:\s*(\w+)\s+read\s+(\w+)(\s+write\s+(\w+))?)", std::regex::icase);
    static const std::regex methodRe(
        R"(^(procedure|function)\s+(\w+)\s*(\([\s\S]*\))?\s*(:\s*(\w+))?\s*$)", std::regex::icase);
    static const std::regex fieldRe(R"(^([\w,\s]+?)\s*:\s*([\s\S]+)$)");
    std::smatch m;

    if (std::regex_search(stmt, m, dtorRe)) {
        std::cout << "~" << m_currentClassName << "();\n";
        return;
    }
    if (std::regex_search(stmt, m, ctorRe)) {
        std::string params = m[1].matched ? ConvertParamList(m[1].str()) : "()";
        std::cout << m_currentClassName << params << ";\n";
        return;
    }
    if (std::regex_search(stmt, m, propRe)) {
        std::string sa = "\\sa " + m[3].str() + " For reading";
        if (m[5].matched) sa += "   \\sa " + m[5].str() + " For writing";
        std::cout << "/** " << sa << " */\n" << Convert_Type(m[2].str()) << " " << m[1].str() << ";\n";
        return;
    }
    if (std::regex_match(stmt, m, methodRe)) {
        bool isFunc = IEquals(m[1].str(), "function");
        std::string params = m[3].matched ? ConvertParamList(m[3].str()) : "()";
        std::string ret = (isFunc && m[5].matched) ? Convert_Type(m[5].str()) : "void";
        std::cout << ret << " " << m[2].str() << params << ";\n";
        return;
    }
    if (std::regex_match(stmt, m, fieldRe)) {
        std::string type = Convert_Type(Trim(m[2].str()));
        for (auto &nm : SplitTopLevel(m[1].str(), ',')) std::cout << type << " " << Trim(nm) << ";\n";
        return;
    }
    std::cout << "/* pas2dox: unrecognized class member: " << stmt << " */\n";
}

void PasDoxFlexLexer::EmitRecordField(const std::string &stmt) {
    static const std::regex fieldRe(R"(^([\w,\s]+?)\s*:\s*([\s\S]+)$)");
    std::smatch m;
    if (std::regex_match(stmt, m, fieldRe)) {
        std::string type = Convert_Type(Trim(m[2].str()));
        for (auto &nm : SplitTopLevel(m[1].str(), ',')) std::cout << type << " " << Trim(nm) << ";\n";
        return;
    }
    std::cout << "/* pas2dox: unrecognized record field: " << stmt << " */\n";
}

void PasDoxFlexLexer::EmitConstOrResourceString(const std::string &stmt) {
    static const std::regex arrRe(
        R"(^(\w+)\s*=\s*array\s*\[\s*([\w']+)\s*\.\.\s*([\w']+)\s*\]\s*of\s+(\w+)$)", std::regex::icase);
    static const std::regex typedRe(R"(^(\w+)\s*:\s*(\w+)\s*=\s*([\s\S]+)$)");
    static const std::regex simpleRe(R"(^(\w+)\s*=\s*([\s\S]+)$)");
    std::smatch m;

    if (std::regex_match(stmt, m, arrRe)) {
        std::cout << Convert_Type(m[4].str()) << " " << m[1].str() << "[" << m[3].str() << "]; /*!< ["
                  << m[2].str() << ".." << m[3].str() << "] */\n";
        return;
    }
    if (std::regex_match(stmt, m, typedRe)) {
        std::cout << "const " << Convert_Type(m[2].str()) << " " << m[1].str() << " = " << Trim(m[3].str())
                  << ";\n";
        return;
    }
    if (std::regex_match(stmt, m, simpleRe)) {
        std::cout << "const " << m[1].str() << " = " << Trim(m[2].str()) << ";\n";
        return;
    }
    std::cout << "/* pas2dox: unrecognized const declaration: " << stmt << " */\n";
}

void PasDoxFlexLexer::EmitTopLevelDecl(const std::string &stmt) {
    if (IsBareModifier(stmt)) return;

    static const std::regex methodRe(
        R"(^(procedure|function)\s+((?:\w+\.)?\w+)\s*(\([\s\S]*\))?\s*(:\s*(\w+))?\s*$)", std::regex::icase);
    static const std::regex fieldRe(R"(^([\w,\s]+?)\s*:\s*([\s\S]+)$)");
    std::smatch m;

    if (std::regex_match(stmt, m, methodRe)) {
        bool isFunc = IEquals(m[1].str(), "function");
        std::string raw = m[2].str();
        std::string qualified = raw;
        size_t dot = raw.find('.');
        if (dot != std::string::npos) qualified = raw.substr(0, dot) + "::" + raw.substr(dot + 1);

        std::string params = m[3].matched ? ConvertParamList(m[3].str()) : "()";
        std::string ret = (isFunc && m[5].matched) ? Convert_Type(m[5].str()) : "void";
        std::cout << ret << " " << qualified << params << ";\n";
        return;
    }
    if (std::regex_match(stmt, m, fieldRe)) {
        std::string type = Convert_Type(Trim(m[2].str()));
        for (auto &nm : SplitTopLevel(m[1].str(), ',')) std::cout << type << " " << Trim(nm) << ";\n";
        return;
    }
    // Anything else at top level (stray labels, malformed input, etc.) is
    // silently dropped rather than emitted as noisy "unrecognized" output -
    // top-level Pascal has the widest variety of constructs we don't model.
}

// ─────────────────────────────────────────────────────────────────────────
// main()
// ─────────────────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    std::string inputPath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--version") { PrintVersionInfo(); return 0; }
        if (arg == "-h" || arg == "--help") { PrintUsage(); return 0; }
        inputPath = arg; // Doxygen appends the input file path as the final argument
    }

    std::ifstream file;
    std::istream *in = &std::cin;
    if (!inputPath.empty()) {
        file.open(inputPath);
        if (!file) {
            std::cerr << "pas2dox: cannot open '" << inputPath << "'\n";
            return 1;
        }
        in = &file;
    }

    PasDoxFlexLexer lexer(in, &std::cout);
    lexer.yylex();
    lexer.Flush();
    return 0;
}
