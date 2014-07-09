#pragma once


typedef float vec4_t[4];

typedef struct  
{
	void* data;
	int type;
} menuStatement_t;

#define STATEMENT_RAW 0

typedef struct  
{
	int numStatements;
	menuStatement_t** statements;
} menuScript_t;

typedef struct
{
	char* name;
	vec4_t rect;
	char rectAlignX;
	char rectAlignY;
	// 22
	short unknown8;
	vec4_t rect2;
	char pad[8];
	// 48
	int style;
	int border;
	int ownerDraw;
	int ownerDrawFlags;
	float borderSize;
	// 68
	int flags1;
	int flags2;
	int unknown1;
	// 80
	vec4_t foreColor;
	vec4_t backColor;
	vec4_t borderColor;
	vec4_t outlineColor;
	// 144
	vec4_t unknownColor;
	// 160
	char** background; // not actually, but implicitly
} windowDef_t;

typedef windowDef_t Window;

#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s {
	int pos;
	int width;
	int maxChars;
	int unknown;
} columnInfo_t;

typedef struct listBoxDef_s {
	int startPos;
	int endPos;
	int drawPadding;
	int cursorPos;
	float elementWidth;
	float elementHeight;
	int elementStyle;
	int numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char *doubleClick;
	bool notselectable;
} listBoxDef_t;

typedef struct  
{
	Window window;
	// 164
	char pad1[24];
	// 188
	int type;
	// 192
	char pad2[116];
	// 308
	void* typeData;
} itemDef_t;

typedef struct
{
	Window window;
	int unknown2;
	int fullscreen;
	int numItems;
	int unknown3;
	int unknown4;
	int fadeCycle;
	float fadeClamp;
	float fadeAmount;
	float fadeInAmount;
	float blurWorld;
	menuScript_t* onOpen;
	menuScript_t* onRequestClose;
	menuScript_t* onClose;
	menuScript_t* onESC;
	void* execKeys;
	void* visibleExp;
	void* allowedBinding;
	void* soundLoop;
	int unknown5;
	vec4_t focusColor;
	void* rectXExp;
	void* rectYExp;
	void* rectHExp;
	void* rectWExp;
	void* openSoundExp;
	void* closeSoundExp;
	itemDef_t** items;
} menuDef_t;

typedef struct  
{
	char* filename;
	int count;
	menuDef_t** menuFiles;
} menuFile_t;

typedef struct  
{
	int type;
	int id;
	void* extraData;
} menuExpToken_t;

typedef struct  
{
	int count;
	menuExpToken_t* tokens;
} menuExpression_t;

// Q3TA precompiler code

//undef if binary numbers of the form 0b... or 0B... are not allowed
#define BINARYNUMBERS
//undef if not using the token.intvalue and token.floatvalue
#define NUMBERVALUE
//use dollar sign also as punctuation
#define DOLLAR

//maximum token length
#define MAX_TOKEN					1024

//punctuation
typedef struct punctuation_s
{
	char *p;						//punctuation character(s)
	int n;							//punctuation indication
	struct punctuation_s *next;		//next punctuation
} punctuation_t;

//token
typedef struct token_s
{
	char string[MAX_TOKEN];			//available token
	int type;						//last read token type
	int subtype;					//last read token sub type
#ifdef NUMBERVALUE
	unsigned long int intvalue;	//integer value
	long double floatvalue;			//floating point value
#endif //NUMBERVALUE
	char *whitespace_p;				//start of white space before token
	char *endwhitespace_p;			//start of white space before token
	int line;						//line the token was on
	int linescrossed;				//lines crossed in white space
	struct token_s *next;			//next token in chain
} token_t;

//script file
typedef struct script_s
{
	char filename[1024];			//file name of the script
	char *buffer;					//buffer containing the script
	char *script_p;					//current pointer in the script
	char *end_p;					//pointer to the end of the script
	char *lastscript_p;				//script pointer before reading token
	char *whitespace_p;				//begin of the white space
	char *endwhitespace_p;			//end of the white space
	int length;						//length of the script in bytes
	int line;						//current line in script
	int lastline;					//line before reading token
	int tokenavailable;				//set by UnreadLastToken
	int flags;						//several script flags
	punctuation_t *punctuations;	//the punctuations used in the script
	punctuation_t **punctuationtable;
	token_t token;					//available token
	struct script_s *next;			//next script in a chain
} script_t;

//macro definitions
typedef struct define_s
{
	char *name;							//define name
	int flags;							//define flags
	int builtin;						// > 0 if builtin define
	int numparms;						//number of define parameters
	token_t *parms;						//define parameters
	token_t *tokens;					//macro tokens (possibly containing parm tokens)
	struct define_s *next;				//next defined macro in a list
	struct define_s *hashnext;			//next define in the hash chain
} define_t;

//indents
//used for conditional compilation directives:
//#if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s
{
	int type;								//indent type
	int skip;								//true if skipping current indent
	script_t *script;						//script the indent was in
	struct indent_s *next;					//next indent on the indent stack
} indent_t;

//source file
typedef struct source_s
{
	char filename[64];					//file name of the script
	char includepath[64];					//path to include files
	punctuation_t *punctuations;			//punctuations to use
	script_t *scriptstack;					//stack with scripts of the source
	token_t *tokens;						//tokens to read first
	define_t *defines;						//list with macro definitions
	define_t **definehash;					//hash chain with defines
	indent_t *indentstack;					//stack with indents
	int skip;								// > 0 if skipping conditional code
	token_t token;							//last read token
} source_t;

#define MAX_TOKENLENGTH		1024

typedef struct pc_token_s
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;

//token types
#define TT_STRING						1			// string
#define TT_LITERAL					2			// literal
#define TT_NUMBER						3			// number
#define TT_NAME						4			// name
#define TT_PUNCTUATION				5			// punctuation

//typedef int menuDef_t;
//typedef int itemDef_t;

#define KEYWORDHASH_SIZE	512

typedef struct keywordHash_s
{
	char *keyword;
	bool (*func)(itemDef_t *item, int handle);
	//struct keywordHash_s *next;
} keywordHash_t;

menuDef_t* UI_ParseMenu(const char *menuFile);
bool Menu_Parse(int handle, menuDef_t *menu);
int PC_LoadSource(const char *filename);
int PC_FreeSource(int handle);