/******************************************************************************
 *
 *
 *
 * Copyright (C) 2002-2006 by Darren Bowles.
 * Copyright (C) 2016-2025 by Pedro Dias Vicente.
 * Modified by Pedro Dias Vicente.
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 */

%option c++
%option noyywrap
%option nounput
%option noinput
%option noreject
%option yylineno
%option warn
%option outfile="pas2dox_lexer.cpp"
%option prefix="Pas2Dox"

%{ // C++ Headers needed for the generated lexer
   //#include <FlexLexer.h>
   #include <string>
   #include <list>
   #include <vector>
   #include <iostream>
   #include <fstream>
   #include <cstdio>
   #include <algorithm>
   #include <cctype>
   #include <sstream>

   // Project-specific headers
   //#include "pasdoxflexlexer.h"
   #include "globals.h"

   // --- Global Variable Definitions ---

   const char* version = "0.50rc1";
   const char* date = __DATE__;

   FILE* OUTPUT = nullptr;

   const char* versionFile = "Pas2dox_v%d.txt";
   const char* dateFile    = "Pas2dox_v%d.txt";
   const char* outputFile  = "Pas2dox_v%d.out";

   // Struct/class pointers
   func_info*  pFunc     = nullptr;
   class_info* pClass    = nullptr;
   prop_info*  pProperty = nullptr;
   array_info* pArray    = nullptr;

   // Global data lists
   std::list<param_info>    lstParams;
   std::list<param_info>    g_lstParams;
   std::list<std::string>   g_lstComments;
   std::list<int>           g_lstSize;
   std::list<std::string>   g_lParamType;
   std::list<std::string>   g_lArrayStack;

   std::string g_defaultvalue;
   std::string g_processtype;
   std::string g_strTypedef;
   std::string g_strTypedefType;
   std::string g_strTypedefName;

   int g_nSize = 0;
   unsigned int g_nNamespaces = 0;
   unsigned int g_nBegins = 0;

   // Boolean flags
   bool g_bClassBody             = false;
   bool g_bFuncBody              = false;
   bool g_bType                  = false;
   bool g_bImplementation        = false;
   bool g_bIgnoreTypeSection     = false;
   bool g_bConstSec              = false;
   bool g_bParams                = false;
   bool g_bEnum                  = false;
   bool g_bCase                  = false;
   bool g_bInterface             = false;
   bool g_bStructure             = false;
   bool g_bVars                  = false;
   bool g_bSet                   = false;
   bool g_bIgnore                = false;

   bool g_bFom                   = false;
   bool g_bConst                 = false;
   bool g_bVar                   = false;
   bool g_bOut                   = false;
   bool g_bArray                 = false;

   bool g_bComments              = false;
   bool g_bDebugMode             = false;
   bool g_bKeepBody              = false;
   bool g_bJavaDoc               = false;
   bool g_bAtEnd                 = false;
   bool g_bInterfaceOnly         = false;
   bool g_bConvertBody           = false;
   bool g_bGenerateSa            = true;
   bool g_bNamespace             = false;
   bool g_bIgnoreImplementation = false;
   bool g_bCallback              = false;

   // Comment/Directive/General block detection
   int g_nCommentCaller  = 0;
   int g_nDirectiveCaller = 0;
   int g_nGeneralCaller  = 0;
   // If you have struct/class definitions (like func_info, param_info, etc.)
   // that are used *directly by the lexer actions*, you might need to
   // define them here or ensure they are included via a header.
   // Otherwise, they belong in pas2dox.cpp or their own dedicated headers.

  // This part is the C++ user code section.
   // It will be appended to the generated pas2dox_lexer.cpp.
   // It should typically contain the implementations of yyFlexLexer's virtual functions
   // if you choose to override them directly in the generated file.

   // It's crucial that yywrap() is defined if %option noyywrap is NOT used,
   // or if you need custom wrap logic. If %option noyywrap is used (as above),
   // you generally don't need to define yywrap(), as the default behavior is disabled.
   // If you remove %option noyywrap, you would need:
   // int PasDoxFlexLexer::yywrap() { return 1; } // Or custom logic}

   /*****************************************************************************
   /// Type of function
   enum func_type { NORMAL,       ///< Normal
                    CONSTRUCTOR,  ///< Constructor function
                    DESTRUCTOR,   ///< Destructor function
                  };

   /// Type Conversion
   struct type_conv
   {
       char m_pascal_type[15];
       char m_c_type[15];
   };

   type_conv type_conversion[] = {
     {"boolean", "bool"},
     {"shortint", "signed char"},
     {"byte","char*"},
     {"smallint","short"},
     {"word","unsigned short"},
     {"integer", "int"},
     {"longint","long"},
     {"single","float"},
     {"double","double"},
     {"self", "this"},
   };

/// Operator Conversion
struct op_conv
{
  char m_pascal_op[4];
  char m_c_op[3];
};

//
//  \note duplicated operators for bitwise and logical operations.
//
//  biwise operators have a higher precedence, than logical, so
//
//  if x > 0 and x < 10 then...  will use bitwise operators
// if (x > 0) and (x < 10) then...  will use logical operators
//
op_conv operator_conversion[] = {
  {".","::"},       ///< Scope Resolution
  {"not","~"},      ///< Bitwise negation
  {"@","&"},        ///< Address
  {"^","*"},        ///< De-Reference
  {"/","/"},        ///< Floating point division
  {"div","/"},      ///< Integer Division
  {"mod","^"},      ///< Modulus
  {"shl","<<"},     ///< Left bitwise shift
  {"shr",">>"},     ///< Right bitwise shift
  {"=", "=="},      ///< Equal to
  {"<>","!="},      ///< Not Equal to
  {"and","&"},      ///< Bitwise AND
  {"or","|"},       ///< Bitwise OR
  {"xor","^"},      ///< Bitwise XOR
  {"not","!"},      ///< Logical NOT
  {"and","&&"},     ///< logical AND
  {"or","||"},      ///< Logical OR
  {":=","="},       ///< Simple Assignment
};

/// Convert from Pascal type to C++ type
void Convert_Type(string& pText)  ///< Type to be converted
{
  type_conv* p;

  string str = pText;
//  std::transform (str.begin(),str.end(), str.begin(), tolower); #@@
	for(int i = 0; i < str.size(); i++)
		str[i] = tolower(str[i]);

  int count = sizeof(type_conversion) / sizeof(type_conv);

  for (int i = 0; i < count; i++)
  {
    p = &type_conversion[i];

    // have we found the pascal type in our table?
    if (str == p->m_pascal_type)
    {
      // convert to c++ type
      pText = p->m_c_type;
    }
  }
}

/// Remove spaces
void RemoveSpaces(string& pText)  ///< String to work on
{
  int pos = string::npos;

  while(true)
  {
    pos = pText.find(" ", 0);

    if (pos == string::npos)
      break;

    pText.replace(pos, 1, "");
  }
}

/// Class Information
class class_info
{
  public:
    class_info();
    class_info(string name, string parent, string parent2, string var, string type);

    string sName;        /// Class Name
    string sParent;      /// Parent Class
    string sParent2;     /// Second Parent Class

    string sVar;         /// Class Variable
    string sType;        /// Class Type
};

class_info::class_info()
{
  sName = "";
  sParent = "";
  sParent2 = "";
  sVar = "";
  sType = "";
}

class_info::class_info(string name,    ///< Class Name
                       string parent,  ///< Parent Class
            		       string parent2, ///< Second Parent Class
                       string var,     ///< Class Variable
		                   string type)    ///< Class Type
{
  sName = name;
  sParent = parent;
  sParent2 = parent2;
  sVar = var;
  sType = type;
}

/// Parameter Information
class param_info
{
  public:
  // Copy constructor
  param_info(const param_info& obj)  { *this = obj; }

  // Assignment operator
  param_info& operator=(const param_info& obj);

  param_info();
  param_info(string type, string name, string comment, string def, bool c, bool v, bool o, bool array);

  string sType;        /// Parameter Name
  string sName;        /// Parameter Type
  string sDefault;     /// Default Value
  string sComment;     /// Associated Comment
  bool bConst;         /// Const Parameter
  bool bVar;           /// Var Parameter
  bool bOut;           /// Out Parameter
  bool bArray;         /// Array Parameter
};

param_info& param_info::operator=(const param_info& obj)
{
  sType = obj.sType;
  sName = obj.sName;
  sDefault = obj.sDefault;
  sComment = obj.sComment;

  bConst = obj.bConst;
  bVar = obj.bVar;
  bOut = obj.bOut;
  bArray = obj.bArray;

  return *this;
}

param_info::param_info()
{
  sType = "";
  sName = "";
  sComment = "";
  bConst = false;
  bVar = false;
  bOut = false;
  bArray = false;
}

param_info::param_info(string type, string name, string comment, string def, bool c, bool v, bool o, bool array)
{
  sType = type;
  sName = name;
  sComment = comment;
  sDefault = def;
  bConst = c;
  bVar = v;
  bOut = o;
  bArray = array;
}

/// Function Information
class func_info
{
  public:
    func_info();
    func_info(string aclass, string name, string ret, func_type ft);

    string sClass;               ///< Associated Class
    string sName;                ///< Function Name
    string sRet;	               ///< Return Type

    func_type eFuncType;         ///< Function type

 	///< Pure virtual function that defines how this function should be formatted in Pascal.
    virtual void pascalize() = 0;
};

func_info::func_info()
{
  sClass = "";
  sName = "";
  sRet = "";

  eFuncType = NORMAL;
}

func_info::func_info(string aclass, string name, string ret, func_type ft)
{
  sClass = aclass;
  sName = name;
  sRet = ret;

  eFuncType = ft;
}

/// Normal Function
class func_info_typedef : public func_info
{
public:
	func_info_typedef(){}
	func_info_typedef(string aclass, string name, string ret, func_type ft)
		: func_info(aclass, name, ret, ft) {}
	virtual void pascalize(){
		fprintf(OUTPUT, "typedef %s (*%s)(", sRet != "" ? sRet.c_str() : "void",
			sName.c_str());
	};
};

class func_info_variable : public func_info
{
public:
	func_info_variable(){}
	func_info_variable(string aclass, string name, string ret, func_type ft)
		: func_info(aclass, name, ret, ft) {}
	virtual void pascalize(){
		fprintf(OUTPUT, "%s (*%s)(", sRet != "" ? sRet.c_str() : "void",
			sName.c_str());
	};
};


/// Property Information
class prop_info
{
  public:
    prop_info();
    prop_info(string name, string type);

    string sName;                /// Property Name
    string sType;                /// Property Type

    string sRead;                /// Read Value
    string sWrite;               /// Write Value
    string sDefault;             /// Default Value

    bool bRead;                  /// Assign read property
};

prop_info::prop_info()
{
  sName = "";
  sType = "";
}

prop_info::prop_info(string name, string type)
{
  sName = name;
  sType = type;
}

/// Array / Set Information
class array_info
{
  public:
    array_info();
    array_info(string name, string type, int size, string def);

    string sName;           ///< Array Name
    string sType;           ///< Array Type
    int nSize;              ///< Array Size
    string sDefault;        ///< Default Values
};

array_info::array_info()
{
  sName = "";
  sType = "";
  nSize = 0;
  sDefault = "";
}

array_info::array_info(string name, string type, int size, string def)
{
  sName = name;
  sType = type;
  nSize = size;
  sDefault = def;
}

func_info* pFunc = NULL;

class_info* pClass = NULL;
prop_info* pProperty = NULL;
array_info* pArray = NULL;

list<param_info> lstParams;        /// List Of Parameters
list<string> g_lstComments;        /// List Of Comments
list<param_info> g_lstParams;      /// List Of Parameters
list<int> g_lstSize;               /// List of Integers for multi dimensional arrays
string g_defaultvalue;
string g_processtype;
list<string> g_lParamType;         /// Parameter Type

unsigned int g_nNamespaces = 0;
unsigned int g_nBegins = 0;

// Sections
bool g_bClassBody = false;	       /// In Class Body
bool g_bFuncBody = false;          /// In Function Body
bool g_bType = false;              /// In Type Section
bool g_bImplementation = false;    /// In Implementation Section
bool g_bIgnoreTypeSection = false; /// Ignoring type section
bool g_bConstSec = false;          /// In a Const Section
bool g_bParams = false;            /// Parameter List
bool g_bEnum = false;              /// Enumeration
bool g_bCase = false;              /// Case Statement
bool g_bInterface = false;         /// Interface
bool g_bStructure = false;         /// Structure
bool g_bVars = false;              /// Vars
bool g_bSet = false;               /// Sets
bool g_bIgnore = false;            /// Ignore Section

bool g_bFom = false;               /// Property Field Or Method
bool g_bConst = false;             /// Parameter is const
bool g_bVar = false;               /// Var Parameter
bool g_bOut = false;               /// Out Parameter
bool g_bArray = false;             /// Array parameter
list<string> g_lArrayStack;        /// For processing "array of array"s

string g_strTypedef = "";          /// array or set
string g_strTypedefType = "";          /// array or set type
string g_strTypedefName = "";          /// array or set name
int g_nSize = 0;                   /// Array size

bool g_bComments = false;	         /// In a Comment Section

bool g_bDebugMode = false;         /// In Debug Mode
bool g_bKeepBody = false;          /// Keep function body
bool g_bJavaDoc = false;           /// JavaDoc Commenting
bool g_bAtEnd = false;             /// At end of function parameters
bool g_bInterfaceOnly = false;     /// Process only 'Interface' section
bool g_bConvertBody = false;       /// Convert function body to c++
bool g_bGenerateSa = true;         /// Generate \sa clauses
bool g_bNamespace = false;         /// Generate enclosing namespace
bool g_bIgnoreImplementation = false; /// Default is to include implementation

int g_nCommentCaller = 0;          /// Inteligent returning from comments
int g_nDirectiveCaller = 0;        /// Inteligent returning from preprocessor directives
int g_nGeneralCaller = 0;          /// Intelligent return
bool g_bCallback = false;          /// In function typedef

//#define OutputLog(s) { if(g_bDebugMode) fprintf(OUTPUT, "%s\n", s); }

/// Let's learn not to use macros in where inline function is more appropriate.
inline void OutputLog(const char *s, bool newline = true)
{
	if(!g_bDebugMode)
		return;
	for(; *s; ++s){
		// Prevent a newline to be unintentionally included in the debug output
		if(*s == '\n')
			fprintf(OUTPUT, "\\n");
		else
			fputc(*s, OUTPUT);
	}
	if(newline)
		fputc('\n', OUTPUT);
}

/// std::string's concatenation can do it, too.
inline void OutputLog2(const char *s, const char *t){
	OutputLog(s,false);
	OutputLog(t);
}

// We leave them at this time
#define OutputLogNum(s, num) { if(g_bDebugMode) fprintf(OUTPUT, "%s %d\n", s, num); }
#define OutputLogHead() { if(g_bDebugMode) fprintf(OUTPUT, "// line %d\n", __LINE__); }

/// Convert Operator
void Convert_Op(string& pText,   ///< Operator to be converted
                bool logical)    ///< Are we a logical operator?
{
  op_conv* p;

  // Return if we're not operating in function body conversion mode
  if (!g_bConvertBody)
    return;

  string str = pText;
//  std::transform (str.begin(),str.end(), str.begin(), tolower); #@@
	for(int i = 0; i < str.size(); i++)
		str[i] = tolower(str[i]);
  int count = sizeof(operator_conversion) / sizeof(op_conv);

  for (int i = 0; i < count; i++)
  {
    p = &operator_conversion[i];

    // have we found a matching operator in our conversion table?
    if (str == p->m_pascal_op)
    {
      //OutputLog2("// original type = ", pText.c_str());
      //OutputLog2("// new type = ", p->m_c_op);
      // convert to a c++ operator
      pText = p->m_c_op;

      if (!logical)
        break;
    }
  }
}

// Display enumerations
void DisplayEnum(void)
{
  list<param_info>::iterator i;

  OutputLog("// DisplayEnum");

  for (i = lstParams.begin(); i != lstParams.end(); ++i)
  {
    string comm;

    fprintf(OUTPUT, "%s%s ",
      i->sName.c_str(),
      distance (i, lstParams.end()) == 1 ? "" : ",");

    while (!g_lstComments.empty())
    {
      comm = g_lstComments.front();

      OutputLog2("//Got = ", comm.c_str());
      if (comm == "*/") // Corrected line
      {
        fprintf(OUTPUT, "%s\n", comm.c_str());
        g_lstComments.pop_front();
        break;
      }

      fprintf(OUTPUT, "%s", comm.c_str());

      g_lstComments.pop_front();
    }

    fprintf(OUTPUT, "\n");

  }
}

//*****************************************************************************
/// Remove Parameters From List
void RemoveParams()
{
  OutputLog("// clearing parameters");

  g_lstParams.clear();
  lstParams.clear();
}

//*****************************************************************************
/// Process Parameter Types
void ProcessParameterType(const char* pText)
{
  std::list<param_info>::iterator i;

  // Loop through the parameters from the last type position
  for (i = lstParams.begin(); i != lstParams.end(); i++)
  {
    // and assign this type to those new parameters
    i->sType = pText;
     param_info p(pText, i->sName, i->sComment, g_defaultvalue, i->bConst, i->bVar, i->bOut, i->bArray);
    g_lstParams.push_back(p);
  }

  g_defaultvalue = "";
  g_processtype = "";

  OutputLog("// Parameters cleared");
  lstParams.clear();
}

//*****************************************************************************
/// Process Comments
void ProcessComment(void)
{
  // If we're not processing parameters at the moment
  // Then just output the text
  if (g_bParams == false)
  {
    OutputLog("// PC1");
    fprintf(OUTPUT, "%s", yytext);
  }
  else
  {
    OutputLog2("// concatenating to lstParams.back().sComment ", yytext);

    // Add comment to the last item in the parameter list.
    if(!lstParams.empty()){
      lstParams.back().sComment += yytext;
    }
  }
}

//*****************************************************************************
/// Process end;
void ProcessEnd(void)
{
  OutputLog("// processing end\n");

  if (g_bCase)
  {
    fprintf(OUTPUT, "end;\n");
    g_bCase = false;
  }
  else
  {
    fprintf(OUTPUT, "};\n");
  }

  // Tidy up if we're ending a class
  if (pClass)
  {
//    delete pClass;
    pClass = NULL;
    g_bClassBody = false;
  }
}

//*****************************************************************************
/// Output Inherited Class Details
void OutputInheritedClass(int ntype)
{
  OutputLogNum("// Inherited Class, type = ", ntype);
  if (g_bInterface)
  {
  	// Treat interface like a class
    fprintf(OUTPUT, "class ");
    g_bInterface = false;
  }
  else
  {
    fprintf(OUTPUT, "class ");
  }

  if (pClass->sParent != "")
  {
    fprintf(OUTPUT, "%s : public %s", pClass->sName.c_str(), pClass->sParent.c_str());
  }
  else
  {
    fprintf(OUTPUT, "%s", pClass->sName.c_str());
  }

  if (pClass->sParent2 != "")
  {
    fprintf(OUTPUT, ",%s", pClass->sParent2.c_str());
  }

  if (ntype == 0)
  {
    fprintf(OUTPUT, "\n{\n");

    // All members default to public
   	fprintf(OUTPUT, "public:\n");

    g_bClassBody = true;
  }
  else
  {
    fprintf(OUTPUT, ";\n");
  }
}

//*****************************************************************************
/// Process Enumerations
void ProcessEnums(void)
{
  fprintf(OUTPUT, "enum %s\n{\n", pClass->sName.c_str());
  g_bParams = true;

  if (pClass)
  {
    delete pClass;
    pClass = NULL;
  }

  g_bEnum = true;
}

//*****************************************************************************
/// Process Array
void ProcessArrays(void)
{
  fprintf(OUTPUT, "%s %s[%d] = { \n", pArray->sType.c_str(), pArray->sName.c_str(),
    g_nSize);

  g_bParams = true;

  if (pArray)
  {
    delete pArray;
    pClass = NULL;
  }

  g_bEnum = true;
}

//*****************************************************************************
void ProcessFunction(func_type ft, bool variable = false)
{
  // Process function or procedure

  if (pFunc)
    delete pFunc;

  pFunc = variable ? (func_info*)new func_info_variable : (func_info*)new func_info_typedef;
  pFunc->sRet = "";

  pFunc->eFuncType = ft;
}

//*****************************************************************************
// Display function params
void DisplayParams(void)
{
  list<param_info>::iterator i;

  // Loop through parameter list
  for (i = g_lstParams.begin(); i != g_lstParams.end(); i++)
  {
/*    string comm;

    if(!g_lstComments.empty())
    {
      comm = g_lstComments.front();
      OutputLog2("// Got comment : ", comm.c_str());
    }*/

    fprintf(OUTPUT, "%s%s%s %s%s%s%s%s %s%s\n",
        i->bConst ? "const " : "",
        i->bOut   ? "/* out */ " : "",
        i->sType.c_str(),
        i->bVar   ? "&" : "",
        i->bArray ? "*" : "",
        i->sName.c_str(),
        i->sDefault != "" ? " = " : "",
        i->sDefault.c_str(),
        distance (i, g_lstParams.end()) == 1 ? "" : ",",
        i->sComment.c_str()
        );

/*    if (!g_lstComments.empty())
    {
      g_lstComments.pop_front();
    }*/
  }

  RemoveParams();
}

//*****************************************************************************
// Display Variables
void DisplayVariables(void)
{
  list<param_info>::iterator i;

  // Loop through parameter list
  for (i = g_lstParams.begin(); i != g_lstParams.end(); i++)
  {
    string comm;

/*
    if(!g_lstComments.empty())
    {
      comm = g_lstComments.front();
    }
	*/

    if (g_bConvertBody)
    {
      Convert_Type(i->sType);
    }

/*
    fprintf(OUTPUT, "%s %s %s;%s\n",
        g_bConstSec ? "const" : "",
        i->sType.c_str(),
        i->sName.c_str(),
        comm.c_str()
        );
		*/

	OutputLog2("// val = ", i->sDefault.c_str());
    if (i->sDefault != "")
	{

      fprintf(OUTPUT, "%s %s %s = %s; ",
        g_bConstSec ? "const" : "",
        i->sType.c_str(),
        i->sName.c_str(),
		i->sDefault.c_str()
        );

	}
	else
	{
      fprintf(OUTPUT, "%s %s %s; ",
          g_bConstSec ? "const" : "",
          i->sType.c_str(),
          i->sName.c_str()
          );
	}

  	while (!g_lstComments.empty())
	{
	  comm = g_lstComments.front();

	  if (comm == "*/")
	  {
	    fprintf(OUTPUT, "%s\n", comm.c_str());
		g_lstComments.pop_front();
	    break;
	  }

	  fprintf(OUTPUT, "%s", comm.c_str());

	  g_lstComments.pop_front();
	}

	fprintf(OUTPUT, "\n");
  }

  RemoveParams();
}

//*****************************************************************************
/// Change between Delphi and C++ strings
void ChangeStrings(void)
{
  for (int i = 0; i < yyleng; ++i)
  {
    if (yytext[i] == '\'')
    {
      // check for double ''
      if (yytext[i+1] == '\'')
      {
        yytext[i] = '\\';
        ++i;  // safe because of trailing '\0'
      }
      else
      {
        yytext[i] = '\"';
      }
    }
    else
    {
      if (yytext[i] == '\"')
        yytext[i] = '\'';
    }
  }
}

//*****************************************************************************
void PushText(const char* txt);
%}

%{
/// Translate incoming text from Shift-JIS (CP932) to UTF-8.
#define YY_INPUT(buf,result,max_size) \
	 { \
	int c = getc(yyin); \
	if(0x81 <= c && c <= 0x9f || 0xE0 <= c && c <= 0xff){\
		char cs[3];\
		cs[0] = c;\
		cs[1] = getc(yyin);\
		cs[2] = '\0';\
		wchar_t wcs[3] = L"";\
		MultiByteToWideChar(932, MB_PRECOMPOSED, cs, 2, wcs, sizeof wcs);\
		result = WideCharToMultiByte(CP_UTF8, 0, wcs, 1, buf, max_size, NULL, NULL);\
	}\
	else\
	 result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
	 }

   // C/C++ block
   // -- PasDoxFlexLexer Class Declaration --
   class PasDoxFlexLexer : public yyFlexLexer { // <--- CHANGE: Inherit from yyFlexLexer
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
       : yyFlexLexer(in, out) // <--- CHANGE: Call yyFlexLexer constructor
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
           // The second argument for yyunput is the buffer pointer (yytext)
           yyunput(txt[i], yytext);
       }
   }

   // -- main function -- --
   int main(int argc, char** argv) {
       // Declare global variables
       const char* versionFile = "Pas2dox_v%d.txt"; // File to store version information
       const char* dateFile = "Pas2dox_v%d.txt"; // File to store date information
       const char* outputFile = "Pas2dox_v%d.out"; // File to output the processed program

       // // Declare global OUTPUT stream
       // FILE* outputFile = NULL; // Global OUTPUT stream // <--- REMOVE OR COMMENT OUT THIS LINE

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


   };

   // -- main function -- --
   // Output program version information
   void version_info(void) {
       fprintf(stdout, "Pas2dox version %s %s\n", version, date);
       fprintf(stdout, "Copyright Darren Bowles 2002,2003\n\n");
       fprintf(stdout, "Modified by Pedro Dias Vicente 2025\n\n");
       fprintf(stdout, "Pas2dox is a Pascal/Delphi/... \n");
   }


%} // End of C/C++ block

   %option noyywrap nounistd case-insensitive
   %option yylineno debug

   %%
   // Your lexer rules will go here
   // Example:
   "BEGIN" { /* action */ }
   "END"   { /* action */ }
   .       { /* default action */ }
   %%



ID      [a-z_A-Z][a-z_A-Z0-9]*
NUM     [0-9]*
S       [ \t\n\r]
KEYWORD ("abstract"|"cdecl"|"register"|"dynamic"|"virtual"|"export"|"external"|"far"|"forward"|"override"|"overload"|"pascal"|"reintroduce"|"safecall"|"stdcall")
OPER    ("."|"not"|"@"|"^"|"/"|"div"|"mod"|"shl"|"shr"|"="|"<>"|"and"|"xor"|"or"|":=")
NON_ID  [^a-z_A-Z0-9]

%x PROGRAM
%x PROGRAM_NAME
%x PROGRAM_BEGIN
%x function
%x func_params
%x DefaultVal
%x param_type
%x func_ret
%x comments
%x ccomments
%x mytype
%x myparent
%x myparent2
%x myparent3
%x callback
%x InPlaceCallback
%x InPlaceCallback2
%x InPlaceCallback3
%x directive
%x directive_define
%x directive_ifdef
%x directive_ifndef

%x Body
%x Func_Body
%x InClass
%x ClassVar
%x ClassPtrVar
%x vars
%x Uses
%x Struct
%x Property
%x prop_type
%x Constants
%x   ConstAssign1
%x   ConstAssign2
%x   ConstAssign3
%x Constant2
%x Constant3
%x Enum
%x Array
%x Initialisation
%x ProcessVariable
%x   ProcessVariableType
%x   ProcessVariableType2
%x   ProcessVariableType3
%x ProcessArray
%x   ProcessArraySize
%x   ProcessArrayType
%x ProcessSet
%x  ProcessSetType1
%x  ProcessSetType2
%x ProcessEnum
%x ProcessEnum1
%x IfThen
%x ForLoop

%%

<*>\x0d { OutputLog("// We've got a program or unit"); BEGIN (PROGRAM); }
<*>"{$" { OutputLog("// Got a compiler directive"); if (YY_START != directive) g_nDirectiveCaller = YY_START; BEGIN(directive); }
<*>{KEYWORD} { OutputLog2("// got keyword ", YYText()); if(g_bComments || YY_START == function) { fprintf(OUTPUT, "%s", YYText()); } }
<directive>"}" { OutputLog("// Finished Directive"); BEGIN(g_nDirectiveCaller); g_nDirectiveCaller = 0; }
<directive>"define" { OutputLog("// Got a define"); BEGIN(directive_define); }
<directive>"else" { OutputLog("// Got Else"); fprintf(OUTPUT, "#else\n"); BEGIN(directive); }
<directive>"endif" { OutputLog("// Got EndIf"); fprintf(OUTPUT, "#endif\n"); BEGIN(directive); }
<directive>"IfDef" { OutputLog("// Got IfDef"); fprintf(OUTPUT, "#ifdef "); BEGIN(directive_ifdef); }
<directive>"IfNDef" { OutputLog("// Got IfNDef"); fprintf(OUTPUT, "#ifndef "); BEGIN(directive_ifndef); }
<directive>[^}] { }
<directive_define>{ID} { fprintf(OUTPUT, "#define %s", YYText()); }
<directive_ifdef>{ID} { fprintf(OUTPUT, "%s\n", YYText()); }
<directive_ifndef>{ID} { fprintf(OUTPUT, "%s\n", YYText()); }
<PROGRAM>"unit" { OutputLog("// got a unit"); fprintf(OUTPUT, "namespace %s\n{\n", YYText()); BEGIN(PROGRAM_NAME); }
<PROGRAM>"program" { OutputLog("// Got a program"); fprintf(OUTPUT, "// Program %s", YYText()); BEGIN(PROGRAM_NAME); }
<PROGRAM_NAME>{ID}";" { OutputLog("// Got Program Name"); fprintf(OUTPUT, "// %s\n", YYText()); BEGIN(PROGRAM_BEGIN); }
<PROGRAM_BEGIN>"uses" { OutputLog("// Got uses"); BEGIN(Uses); }
<Uses>{ID} { OutputLog2("// Got unit : ", YYText()); fprintf(OUTPUT, "/// \\sa %s\n", YYText()); }
<Uses>";" { OutputLog("// finished Uses"); BEGIN(Body); }
<Body>"type" { OutputLogHead(); if (g_bIgnoreImplementation && g_bImplementation) { OutputLog("// Ignore type Section"); fprintf(OUTPUT, "#ifdef INCLUDE_TYPE_SECTION\n"); g_bIgnoreTypeSection = true; g_bType = true; g_bConstSec = false; BEGIN(mytype); } else { OutputLog("// Not Ignoring type Section"); g_bType = true; g_bConstSec = false; BEGIN(mytype); } }
<Body,Constants>^(const|resourcestring) { OutputLogHead(); if (g_bIgnoreImplementation && g_bImplementation) { OutputLog("// Ignore const Section"); fprintf(OUTPUT, "/*\n//const\n"); g_bVars = true; g_bComments = true; g_bIgnore = true; BEGIN(vars); } else { OutputLog("// Not Ignoring const Section"); g_bType = false; g_bClassBody = false; g_bConstSec = true; BEGIN(Constants); } }
<Body>^var|{S}*var { if (g_bImplementation) { OutputLog("// Ignore var Section"); g_bVars = true; fprintf(OUTPUT, "/*\nvar\n"); g_bComments = true; BEGIN(vars); } else { OutputLog("// in a type varsection"); g_bType = false; g_bClassBody = false; g_bConstSec = false; BEGIN(vars); } }
<Body>"class" { OutputLog("// got class"); BEGIN(InClass); }
<Body>"interface" { OutputLog("// got interface"); g_bInterface = true; BEGIN(InClass); }
<Body>^procedure|{S}*procedure { OutputLogHead(); OutputLog("// got procedure"); ProcessFunction(NORMAL); BEGIN(function); }
<Body>^function|{S}*function { OutputLogHead(); OutputLog("// got function"); ProcessFunction(NORMAL); BEGIN(function); }
<Body>^constructor|{S}*constructor { OutputLogHead(); OutputLog("// got constructor"); ProcessFunction(CONSTRUCTOR); BEGIN(function); }
<Body>^destructor|{S}*destructor { OutputLogHead(); OutputLog("// got destructor"); ProcessFunction(DESTRUCTOR); BEGIN(function); }
<Body>^property|{S}*property { OutputLogHead(); OutputLog("// got property"); if (pProperty) delete pProperty; pProperty = new prop_info; BEGIN(Property); }
<Body>^begin|{S}*begin { OutputLogHead(); g_nBegins++; if (g_bConvertBody) { fprintf(OUTPUT, "{ "); } else { fprintf(OUTPUT, "begin"); } BEGIN(Func_Body); }
<Body>"{"[^}\n]*"}" { OutputLog("// Found comment"); fprintf(OUTPUT, "%s", YYText()); }
<Body>"//"[^\n]* { OutputLog("// Found comment"); fprintf(OUTPUT, "%s", YYText()); }
<Body>{NUM} { fprintf(OUTPUT, "%s", YYText()); }
<Body>{ID} { fprintf(OUTPUT, "%s", YYText()); }
<Body>[^a-z_A-Z0-9\t\n\r] { fprintf(OUTPUT, "%s", YYText()); }
<Body>";" { fprintf(OUTPUT, "%s", YYText()); }
<Body>"." { fprintf(OUTPUT, "%s", YYText()); }
<Body>{S} { }
<InClass,Func_Body,Property>{ID}{S}*":"{S}*"class of" { OutputLog("// pointer to class - in class/func_body/property"); BEGIN(ClassPtrVar); }
<ClassPtrVar>{ID}";" { OutputLog2("// Got Class Ptr Var : ", YYText()); fprintf(OUTPUT, "typedef %s* %s;", YYText(), pClass->sName.c_str()); BEGIN(g_nGeneralCaller); }
<InClass>{ID} { OutputLogHead(); OutputLog2("// In Class :", YYText()); if (pClass->sName != "") pClass->sName = YYText(); }
<InClass>"(" { OutputLogHead(); OutputLog("// Got a parameter list for this class"); BEGIN(myparent); }
<InClass>";" { OutputLogHead(); OutputLog("// finished class"); OutputInheritedClass(0); BEGIN(InClass); }
<InClass>"end" { OutputLogHead(); OutputLog("// ending class"); ProcessEnd(); BEGIN(Body); }
<function>"." { OutputLogHead(); OutputLog("// Got class dot notation"); string s = YYText(); int pos = s.find('.', 0); int len = s.length(); pFunc->sClass = s.substr(0, pos); pFunc->sName = s.substr(pos + 1, len -(pos+1) ); OutputLog2("// Class = ", pFunc->sClass.c_str()); OutputLog2("// Name = ", pFunc->sName.c_str()); }
<function>{ID} { OutputLogHead(); OutputLog2("// process function name :", YYText()); pFunc->sName = YYText(); }
<function>"(" { OutputLogHead(); OutputLog("// We've got a parameter list for this function"); g_bParams = true; BEGIN(func_params); }
<func_ret>{ID}"."{ID} { string str = YYText(); string strUnit; string strRet; int pos = str.find('.', 0); int len = str.length(); strUnit = str.substr(0, pos); strRet = str.substr(pos + 1, len -(pos+1) ); fprintf(OUTPUT, "/// \\sa %s\n", strUnit.c_str()); pFunc->sRet = strRet; if (g_bCallback) { BEGIN(callback); } else { BEGIN(function); } }
<func_ret>{ID} { OutputLog2("// assign return type :", YYText()); pFunc->sRet = YYText(); if (g_bCallback) { BEGIN(callback); } else { BEGIN(function); } }
<func_params>")"":" { ProcessParameterType(g_processtype.c_str()); OutputLog("// Parameter list has finished - there's a return type to be got"); pFunc->pascalize(); DisplayParams(); fprintf(OUTPUT, ") "); BEGIN(func_ret); }
<func_params>")" { OutputLog("// Parameter list has finished"); ProcessParameterType(g_processtype.c_str()); pFunc->pascalize(); DisplayParams(); fprintf(OUTPUT, ")"); BEGIN(function); }
<func_params>";" { OutputLog("// got a multiple parameter definition"); ProcessParameterType(g_processtype.c_str()); BEGIN(func_params); }
<func_params>":" { OutputLog("// got a parameter type"); BEGIN(param_type); }
<param_type>{ID} { OutputLog2("// got parameter type : ", YYText()); g_processtype = YYText(); BEGIN(func_params); }
<func_params>"="{NUM} { OutputLog2("// got default value : ", YYText()); g_defaultvalue = YYText(); }
<func_params>"=" { OutputLog("// got default value, must be a string"); BEGIN(DefaultVal); }
<DefaultVal>"'"[^']*"'"|"''" { OutputLog2("// got default value string : ", YYText()); g_defaultvalue = YYText(); ChangeStrings(); BEGIN(func_params); }
<func_params>array { OutputLog("// Got array"); g_bArray = true; }
<func_params>const { OutputLog("// Got const"); g_bConst = true; }
<func_params>var { OutputLog("// Got var"); g_bVar = true; }
<func_params>out { OutputLog("// Got out"); g_bOut = true; }
<func_params>{ID} { OutputLog2("// got parameter name :", YYText()); param_info p("", YYText(), "", "", g_bConst, g_bVar, g_bOut, g_bArray); lstParams.push_back(p); g_bConst = false; g_bVar = false; g_bOut = false; g_bArray = false; }
<func_params>"//"[^\n]* { OutputLog("// got comment"); g_lstComments.push_back(YYText()); }
<func_params>"(*" { OutputLog("// got function pointer in param list"); g_bCallback = true; ProcessFunction(NORMAL, true); BEGIN(InPlaceCallback); }
<InPlaceCallback>{ID} { OutputLog2("// In-place function pointer : ", YYText()); pFunc->sName = YYText(); }
<InPlaceCallback>")"{S}*":"{S}*{ID} { OutputLogHead(); OutputLog2("// In-place function pointer return : ", YYText()); string s = YYText(); int pos = s.find(':', 0); string ids = s.substr(pos + 1, s.length() - (pos + 1)); pFunc->sRet = ids; pFunc->pascalize(); fprintf(OUTPUT, "%s)", pFunc->sRet.c_str()); BEGIN(func_params); }
<InPlaceCallback2>":" { OutputLogHead(); OutputLog("// in-place function pointer eats up :"); BEGIN(InPlaceCallback3); }
<InPlaceCallback3>(function|procedure) { OutputLogHead(); OutputLog2("// in-place function pointer type declaration begins, callback = ", g_bCallback ? "true" : "false"); BEGIN(callback); }
<vars>"{*}" { }
<Func_Body>{S}*"inherited"{S}+ { if (g_bConvertBody) { fprintf(OUTPUT, "\n::"); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body,IfThen>"<>" { if (g_bConvertBody) { fprintf(OUTPUT, "!="); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body,IfThen,Body>")"{S}*{OPER}{S}*"(" { string op = YYText(); Convert_Op(op, true); fprintf(OUTPUT, "%s", op.c_str()); }
<Func_Body,IfThen,Body>{OPER} { string op = YYText(); Convert_Op(op, false); fprintf(OUTPUT, "%s", op.c_str()); }
<Func_Body>^if|{S}+if{S}+ { if (g_bConvertBody) { OutputLog("// Processing if "); PushText(YYText()); BEGIN(IfThen); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"for ".*"do" { if (g_bConvertBody) { OutputLog("// Processing for"); PushText(YYText()); BEGIN(ForLoop); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"repeat" { if (g_bConvertBody) { OutputLog("// Processing repeat"); g_nBegins++; fprintf(OUTPUT, "do\n{ "); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"until" { if (g_bConvertBody) { OutputLog("// Processing until"); g_nBegins--; fprintf(OUTPUT, "} while "); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>{ID}"."{ID}"("{NUM}")"";" { OutputLog("// Got a func call with number parameter"); OutputLog2("// text ", YYText()); string str = YYText(); int i = str.find('(', 0); str.replace(i, 3, "();" ); fprintf(OUTPUT, "%s", str.c_str()); }
<Func_Body,IfThen>"Owner." { if (g_bConvertBody) { fprintf(OUTPUT, "::"); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>{ID}{S}*":"{S}+ { if (g_bConvertBody) { fprintf(OUTPUT, "case %s ", YYText()); } else { fprintf(OUTPUT, "%s", YYText()); } }
<IfThen>if { fprintf(OUTPUT, "if ("); }
<IfThen>"then" { fprintf(OUTPUT, ") "); BEGIN(Func_Body); }
<Func_Body>"'"[^']+"'"|"''" { fprintf(OUTPUT, "%s",YYText()); }
<Func_Body>{S}case{S}[^*\n]+ { OutputLogNum("// Got Case - ", g_nBegins); if (!g_bComments) { g_nBegins++; g_bCase = true; } if (g_bConvertBody) { string str = YYText(); int i = str.find("case ", 0); str.erase(i, 5); i = str.find(" of", 0); str.erase(i,3); string str2 = str; for(int i = 0; i < str2.size(); i++) str2[i] = tolower(str2[i]); i = str2.find("self", 0); if (i != string::npos) { str = "this->"; str += str2.substr(i+4, str2.size() - (i+4)); } fprintf(OUTPUT, "switch (%s) {\n", str.c_str()); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"exit" { if (g_bConvertBody) { fprintf(OUTPUT, "return"); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"result" { if (g_bConvertBody) { fprintf(OUTPUT, "return"); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"else" { if (g_bConvertBody) { fprintf(OUTPUT, "} else {"); } else { fprintf(OUTPUT, "%s", YYText()); } }
<Func_Body>"begin" { OutputLogHead(); g_nBegins++; if (g_bConvertBody) { fprintf(OUTPUT, "{ "); } else { fprintf(OUTPUT, "begin"); } }
<Func_Body>"end" { OutputLogHead(); g_nBegins--; if (g_nBegins == 0) { if (g_bConvertBody) { fprintf(OUTPUT, "}"); } else { fprintf(OUTPUT, "end"); } BEGIN(Body); } else { if (g_bConvertBody) { fprintf(OUTPUT, "}"); } else { fprintf(OUTPUT, "end"); } BEGIN(Func_Body); } }
<Body>^finalization { fprintf(OUTPUT, "// %s\n", YYText()); }
<Body>^initialization { fprintf(OUTPUT, "// %s\n", YYText()); fprintf(OUTPUT, "\n#ifndef DOXYGEN_SKIP\n"); BEGIN(Initialisation); }
<Initialisation>"end." { fprintf(OUTPUT, "// finished\n"); fprintf(OUTPUT, "#endif\n"); BEGIN(Body); }
<*>^implementation { if (g_bInterfaceOnly) { OutputLog("// End of 'Interface' section - finish now"); yyterminate(); } OutputLog("// Processing an implementation section"); g_bType = false; g_bConstSec = false; g_bImplementation = true; g_nBegins = 0; BEGIN(Body); if (pClass) { delete pClass; pClass = NULL; g_bClassBody = false; } }
<Constants>^var { OutputLogHead(); OutputLog("// in a type varsection"); g_bType = false; g_bClassBody = false; g_bConstSec = false; BEGIN(Constants); }
<Constants,mytype,Func_Body,InClass,Struct,myparent>{ID}{S}*":"{S}*"("([^)]*)")"{S}*";" { OutputLogHead(); PushText(YYText()); g_nGeneralCaller = YY_START; BEGIN(ProcessEnum); }
<ProcessEnum>{ID} { g_strTypedefName = YYText(); BEGIN(ProcessEnum1); }
<ProcessEnum1>":" {} // eat
<ProcessEnum1>{S}*"("([^)]*)")"{S}* { ChangeStrings(); string str = YYText(); int pos = str.find("(", 0); str = str.replace(pos, str.size() - pos, ""); g_strTypedefType = str; DisplayEnum(); g_bParams = false; OutputLog2("// Typedef type : ", g_strTypedefType.c_str()); OutputLog2("// Typedef Name : ", g_strTypedefName.c_str()); fprintf(OUTPUT, "typedef enum %s %s;", g_strTypedefType.c_str(),g_strTypedefName.c_str()); BEGIN(g_nGeneralCaller); }
<mytype>type { OutputLog("// ignore type keyword"); }
<mytype>(^end;|{S}end;|{S}end{S};) { OutputLog("// Ignore end..."); }
<mytype>{ID} { OutputLog2("// processing types : ", YYText()); if (pClass) delete pClass; pClass = new class_info; pClass->sName = YYText(); BEGIN(myparent); }
<myparent>("="|":"){S}*"class of" { OutputLog("// pointer to class"); BEGIN(ClassPtrVar); }
<myparent>("="|":"){S}*"^" { OutputLog("// pointer to class"); BEGIN(ClassPtrVar); }
<myparent>"="{S}"interface"{S}*"(" { g_bInterface = true; BEGIN(myparent2); }
<myparent>"="{S}*"class"{S}*";" { OutputLog("// got x = class;"); fprintf(OUTPUT, "class %s;\n", pClass->sName.c_str()); BEGIN(mytype); }
<myparent>"="{S}*"class"{S}*"(" { BEGIN(myparent2); }
<myparent2>"," { }
<myparent2>{ID} { if (pClass->sParent != "") { OutputLog2("// Assigning parent2 : ", YYText()); pClass->sParent2 = YYText(); } else { OutputLog2("// Assigning parent : ", YYText()); OutputLog2("// Name parent : ", pClass->sName.c_str()); pClass->sParent = YYText(); } }
<myparent2>")" { OutputLogHead(); OutputLog("// Processed parents"); OutputInheritedClass(0); BEGIN(InClass); }
<myparent3>{S}*";" { OutputLogHead(); OutputLog("// processed struct"); fprintf(OUTPUT, "} %s;\n", pClass->sName.c_str()); pClass->sName = ""; g_bClassBody = false; BEGIN(mytype); }
<Property,prop_type>";" { if (g_bGenerateSa) { if (pProperty->sRead != "" && pProperty->sWrite != "") { fprintf(OUTPUT, "/** \\sa %s For reading \\sa %s For writing */\n", pProperty->sRead.c_str(), pProperty->sWrite.c_str()); } else if (pProperty->sRead != "") { fprintf(OUTPUT, "/** \\sa %s For reading*/\n", pProperty->sRead.c_str()); } else if (pProperty->sWrite != "") { fprintf(OUTPUT, "/** \\sa %s For writing*/\n", pProperty->sWrite.c_str()); } } fprintf(OUTPUT, "%s %s; \n", pProperty->sType.c_str(), pProperty->sName.c_str() ); if (pProperty) { delete pProperty; pProperty = NULL; } g_bFom = false; if (g_bClassBody) { BEGIN(InClass); } else { BEGIN(Body); } }
<Property>{ID} { OutputLog2("// property :", YYText()); pProperty->sName = YYText(); }
<Body,InClass>default;
<*>"(*" { OutputLog("daz2"); if (!g_bComments) { if (!g_bIgnore) { if (g_bJavaDoc) fprintf(OUTPUT, "/**"); else fprintf(OUTPUT, "/***"); } g_bComments = true; } if (YY_START != comments) g_nCommentCaller = YY_START; BEGIN(comments); }
<comments>"*)" { OutputLog("daz3"); if (g_nCommentCaller != 0) BEGIN(g_nCommentCaller); else BEGIN(INITIAL); g_nCommentCaller = 0; g_bComments = false; if (!g_bIgnore) { if (g_bJavaDoc) fprintf(OUTPUT, " */\n"); else fprintf(OUTPUT, " ***/\n"); } }
<comments>"\n" { if (!g_bIgnore) { if (g_bJavaDoc) fprintf(OUTPUT, "\n * "); else fprintf(OUTPUT, "\n * "); } }
<comments>. { if (!g_bIgnore) { fprintf(OUTPUT, "%s", YYText()); } }
<comments>"/*" { OutputLog("daz"); if (!g_bComments) { g_bComments = true; } BEGIN(ccomments); }
<ccomments>"*/" { OutputLog("daz1"); BEGIN(comments); }
<ccomments>. { }
<ccomments>"\n" { }
<InClass,Struct,Constants,mytype,Func_Body>{ID}{S}*":"{S}*"array"{S}*"[" { OutputLogHead(); PushText(YYText()); g_nGeneralCaller = YY_START; BEGIN(ProcessArray); }
<ProcessArray>{ID} { pArray = new array_info(YYText(), "", 0, ""); BEGIN(ProcessArraySize); }
<ProcessArraySize>(":"|"="){S}*"array"{S}*"[" {} // eat up
<ProcessArraySize>"]" {} // eat up
<ProcessArraySize>"byte" { OutputLog("// got byte, so size = 255"); g_nSize = 255; g_lstSize.push_back(g_nSize); }
<ProcessArraySize>"-"*{NUM} { g_nSize = atoi(YYText()); OutputLogNum("// got size = ", g_nSize); g_lstSize.push_back(g_nSize); }
<ProcessArraySize>"of" { OutputLog("// got of"); BEGIN(ProcessArrayType); }
<ProcessArrayType>{ID} { OutputLog2("// got array type : ", YYText()); pArray->sType = YYText(); g_strTypedef = "Array"; g_strTypedefType = pArray->sType; g_strTypedefName = pArray->sName; ProcessArrays(); g_bParams = false; BEGIN(g_nGeneralCaller); }
%%