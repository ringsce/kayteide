#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <list>
#include <vector> // Ensure vector is included if used by globals

// Forward declarations for classes/structs used by global pointers/lists
// Only if the full definition isn't included here or in another common header
class func_info;
class class_info;
class prop_info;
class array_info;
struct param_info; // Assuming param_info is a struct

// Global variables (declarations)
extern const char* version;
extern const char* date;
extern FILE* OUTPUT;
extern const char* versionFile;
extern const char* dateFile;
extern const char* outputFile; // This should be for a filename string, not a FILE*

extern class func_info* pFunc;
extern class class_info* pClass;
extern class prop_info* pProperty;
extern class array_info* pArray;

extern std::list<class param_info> lstParams;
extern std::list<std::string> g_lstComments;
extern std::list<class param_info> g_lstParams;
extern std::list<int> g_lstSize;
extern std::string g_defaultvalue;
extern std::string g_processtype;
extern std::list<std::string> g_lParamType;

extern unsigned int g_nNamespaces;
extern unsigned int g_nBegins;

// Sections
extern bool g_bClassBody;
extern bool g_bFuncBody;
extern bool g_bType;
extern bool g_bImplementation;
extern bool g_bIgnoreTypeSection;
extern bool g_bConstSec;
extern bool g_bParams;
extern bool g_bEnum;
extern bool g_bCase;
extern bool g_bInterface;
extern bool g_bStructure;
extern bool g_bVars;
extern bool g_bSet;
extern bool g_bIgnore;

extern bool g_bFom;
extern bool g_bConst;
extern bool g_bVar;
extern bool g_bOut;
extern bool g_bArray;
extern std::list<std::string> g_lArrayStack;

extern std::string g_strTypedef;
extern std::string g_strTypedefType;
extern std::string g_strTypedefName;
extern int g_nSize;

extern bool g_bComments;

extern bool g_bDebugMode;
extern bool g_bKeepBody;
extern bool g_bJavaDoc;
extern bool g_bAtEnd;
extern bool g_bInterfaceOnly;
extern bool g_bConvertBody;
extern bool g_bGenerateSa;
extern bool g_bNamespace;
extern bool g_bIgnoreImplementation;

extern int g_nCommentCaller;
extern int g_nDirectiveCaller;
extern int g_nGeneralCaller;
extern bool g_bCallback;

#endif // GLOBALS_H