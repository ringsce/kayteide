// Global variables and definitions
const char* version = "0.50rc1";
const char* date = __DATE__;
FILE* OUTPUT = NULL; // Global OUTPUT stream
const char* versionFile = "Pas2dox_v%d.txt"; // File to store version information
const char* dateFile = "Pas2dox_v%d.txt"; // File to store date information

// C/C++ block
// -- PasDoxFlexLexer Class Declaration --
class PasDoxFlexLexer : public FlexLexer {
public:
    // Constructor declaration
    PasDoxFlexLexer(std::istream* in = nullptr, std::ostream* out = nullptr);

    // Member function definitions
    string Convert_Type(string s);
    string RemoveSpaces(string s);
    string Convert_Op(string s);
    void DisplayEnum();
    void RemoveParams(string s);
    void ProcessParameterType();
    void ProcessComment();
    void ProcessEnd();
    void OutputInheritedClass();
    void ProcessEnums();
    void ProcessArrays();
    void ProcessFunction();
    void DisplayParams();
    void DisplayVariables();
    string ChangeStrings(string s);
    void OutputLog(const char* s, bool newline = true); // Declaration
    void OutputLog2(const char* s1, const char* s2);   // Declaration
    void OutputLogNum(const char* s, int num);         // Declaration
    void OutputLogHead(const char* s);                 // Declaration
    void PushText(const string& txt);                  // Declaration
    void version_info();                               // Declaration
};

// -- PasDoxFlexLexer Member Function Definitions --
// Constructor definition
PasDoxFlexLexer::PasDoxFlexLexer(std::istream* in, std::ostream* out)
    : FlexLexer(in, out)
{
    OUTPUT = stdout; // Initialize global FILE* OUTPUT if still needed
}

// OutputLog member function definition
void PasDoxFlexLexer::OutputLog(const char* s, bool newline) {
    if (g_bDebugMode) {
        fprintf(stderr, "%s", s);
        if (newline) fprintf(stderr, "\n");
    }
}

// OutputLog2 member function definition
void PasDoxFlexLexer::OutputLog2(const char* s1, const char* s2) {
    if (g_bDebugMode) {
        fprintf(stderr, "%s%s", s1, s2);
    }
}

// OutputLogNum member function definition
void PasDoxFlexLexer::OutputLogNum(const char* s, int num) {
    if (g_bDebugMode) {
        fprintf(stderr, "%s%d\n", s, num);
    }
}

// OutputLogHead member function definition
void PasDoxFlexLexer::OutputLogHead(const char* s) {
    if (g_bDebugMode) {
        fprintf(stderr, "*****************************************************\n");
        fprintf(stderr, "LOG: %s\n", s);
    }
}

// PushText member function definition
void PasDoxFlexLexer::PushText(const string& txt) {
    for (int i = static_cast<int>(txt.length()) - 1; i >= 0; --i) {
        unput(txt[i]);
    }
}

// -- main function -- --
int main(int argc, char** argv) {
    // Declare global variables
    const char* versionFile = "Pas2dox_v%d.txt"; // File to store version information
    const char* dateFile = "Pas2dox_v%d.txt"; // File to store date information
    const char* outputFile = "Pas2dox_v%d.out"; // File to output the processed program

    // Declare global OUTPUT stream
    FILE* outputFile = NULL; // Global OUTPUT stream

    // Declare global variables
    const char* version = "0.50rc1";
    const char* date = __DATE__;
    int nComments = 0;
    bool g_bDebugMode = false;
    bool g_bKeepBody = false;
    bool g_bJavaDoc = false;
    bool g_bAtEnd = false;
    bool g_bInterfaceOnly = false;
    bool g_bConvertBody = false;
    bool g_bGenerateSa = true;
    bool g_bNamespace = false;
    bool g_bIgnoreImplementation = false;
    bool g_bDebugMode = false;
    bool g_bKeepBody = false;
    bool g_bJavaDoc = false;
    bool g_bAtEnd = false;
    bool g_bInterfaceOnly = false;
    bool g_bConvertBody = false;
    bool g_bGenerateSa = true;
    bool g_bNamespace = false;
    bool g_bIgnoreImplementation = false;
    int g_nCommentCaller = 0;
    int g_nDirectiveCaller = 0;
    int g_nGeneralCaller = 0;
    bool g_bCallback = false;

    // Declare global variables
    string g_strTypedef = "";
    string g_strTypedefType = "";
    string g_strTypedefName = "";
    int g_nSize = 0;

    // Declare global variables
    bool g_bComments = false;

    // Declare global variables
    bool g_bDebugMode = false;
    bool g_bKeepBody = false;
    bool g_bJavaDoc = false;
    bool g_bAtEnd = false;
    bool g_bInterfaceOnly = false;
    bool g_bConvertBody = false;
    bool g_bGenerateSa = true;
    bool g_bNamespace = false;
    bool g_bIgnoreImplementation = false;

    // Declare global variables
    int g_nCommentCaller = 0;
    int g_nDirectiveCaller = 0;
    int g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false;

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0;
    string g_nGeneralCaller = 0;

    // Declare global variables
    bool g_bCallback = false

    // Declare global variables
    string g_nCommentCaller = 0;
    string g_nDirectiveCaller = 0
};

// -- main function -- --
// Output program version information
void version_info(void) {
    fprintf(stdout, "Pas2dox version %s %s\n", version, date);
    fprintf(stdout, "Copyright Darren Bowles 2002,2003\n\n");
    fprintf(stdout, "Pas2dox is a Pascal/Delphi/... \n");
}

// -- main function -- --
int main(int argc, char** argv) {
    // Declare global variables
    const char* versionFile = "Pas2dox_v%d.txt"; // File to store version information
    const char* dateFile = "Pas2dox_v%d.txt"; // File to store date information

    // Instantiate lexer with correct streams
    PasDoxFlexLexer lexer(currentInStream, currentOutStream);
    lexer.version_info(); // Call a member function

    // Perform lexing
    lexer.yylex();

    // Close file streams if they were opened
    if (inputFileStream.is_open()) {
        inputFileStream.close();
    }
    if (outputFileStream.is_open()) { // Close outputFileStream if it was opened
        outputFileStream.close();
    }

    return 0;
}