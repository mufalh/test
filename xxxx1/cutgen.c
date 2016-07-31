/*
 * CUT 2.3
 * Copyright (c) 2001-2002 Samuel A. Falvo II, William D. Tanksley
 * See LICENSE.TXT for details.
 *
 * $Log: cutgen.c,v $
 * Revision 1.4  2003/03/18 05:53:50  sfalvo
 * ADD: cutgen.c: cut_exit() -- common exit point; returns proper error code
 * at all times.
 *
 * FIX: cutgen.c: Factored all instances of exit() to invoke cut_exit()
 * instead.  This fixes the bug #703793.
 *
 * Revision 1.3  2003/03/13 04:27:54  sfalvo
 * ADD: LICENSE.TXT -- zlib license
 *
 * ADD: README cut.h cutgen.c -- Changelog token for CVS
 *
 * FIX: test/bringup-failure -- reflects new usage for bringups and
 * teardowns in CUT 2.2.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define DO_NOT_PROCESS        "..."

#define SEARCH_TOKEN_TEST     "__CUT__"
#define SEARCH_TOKEN_BRINGUP  "__CUT_BRINGUP__"
#define SEARCH_TOKEN_TAKEDOWN "__CUT_TAKEDOWN__"

#define MAX_SYMBOL_LENGTH 256  /* arbitrary */
#define MAX_LINE_LENGTH   1024 /* arbitrary */

#define SEARCH_TOKEN_TEST_LENGTH       sizeof( SEARCH_TOKEN_TEST )-1
#define SEARCH_TOKEN_BRINGUP_LENGTH    sizeof( SEARCH_TOKEN_BRINGUP )-1
#define SEARCH_TOKEN_TAKEDOWN_LENGTH   sizeof( SEARCH_TOKEN_TAKEDOWN )-1

typedef enum TestType {
   TYPE_TEST = 0,
   TYPE_BRINGUP = 1,
   TYPE_TAKEDOWN = 2
} TestType;

typedef struct TestItem {
   char name[MAX_SYMBOL_LENGTH];
   enum TestType type;
   struct TestItem *next;
} TestItem;

/* globals */

TestItem *testList = 0;
FILE *outfile;
extern char *libCUT[]; /* defined at the end of this file */

static int g_count, g_ready, g_index;  /* Used by filename globbing support for windows */
static char **g_wildcards, g_fileName[MAX_LINE_LENGTH];

TestItem *FindFirstMatch( TestItem *current, char *basis_name, int basis_type )
{
   while ( current )
   {
      if ( !strcmp(current->name,basis_name) && current->type == basis_type )
         return current;
      current = current->next;
   }
   return 0;
}

int NameAndTypeInTestList( char *name, TestType type )
{
   return 0 != FindFirstMatch(testList,name,type);
}

void AppendToTestList( char *name, TestType type )
{
   struct TestItem *current = testList;
   if ( !current )
      current = testList = malloc( sizeof( *testList) );
   else
   {
      while ( current->next ) current = current->next;
      current->next = malloc(sizeof( *testList));
      current = current->next;
   }
   
   current->next = 0;
   strcpy(current->name, name);
   current->type = type;
}

void InsertNameAndTypeIntoTestList( char *name, TestType type )
{
    if ( !NameAndTypeInTestList( name, type ) )
        AppendToTestList( name, type );
}

int CharacterIsDigit(char ch)
{
    return ( ( ch >= '0') && ( ch <= '9' ) );
}

int CharacterIsUppercase(char ch)
{
    return ( ( ch >= 'A' ) && ( ch <= 'Z' ) );
}

int CharacterIsLowercase(char ch)
{
    return ( ( ch >= 'a' ) && ( ch <= 'z' ) );
}

int CharacterIsAlphabetic(char ch)
{
    return CharacterIsUppercase(ch) || CharacterIsLowercase(ch) || ( ch == '_' );
}

int CharacterIsAlphanumeric( char ch )
{
    return CharacterIsDigit(ch) || CharacterIsAlphabetic(ch);
}

void ProcessGenericFunction( char *line, int position, 
                            TestType type, int tokenDisplacement )
{
    char name[MAX_SYMBOL_LENGTH] = "";
    int maxLength = strlen( line ) - 1, offset=0;
    position = position + tokenDisplacement;
    
    while ( CharacterIsAlphanumeric(line[position]) 
       && (position<maxLength) && (offset<MAX_SYMBOL_LENGTH) )
    {
        name[offset++] = line[position++];
        name[offset] = 0;
    }

    InsertNameAndTypeIntoTestList( name, type );
}

void ProcessBringupFunction( char *line, int position )
{
    ProcessGenericFunction( line, position, TYPE_BRINGUP, SEARCH_TOKEN_BRINGUP_LENGTH );
}

void ProcessTestFunction( char *line, int position )
{
    ProcessGenericFunction( line, position, TYPE_TEST, SEARCH_TOKEN_TEST_LENGTH );
}

void ProcessTakedownFunction( char *line, int position )
{
    ProcessGenericFunction( line, position, TYPE_TAKEDOWN, SEARCH_TOKEN_TAKEDOWN_LENGTH );
}


int OffsetOfSubstring( char *line, char *token )
{
    char *inset = strstr(line,token);
    
    if ( !inset ) return -1;
    else return inset - line;
}

void CallIfSubstringFound( char *line, char *token, void (*function)(char*,int) )
{
    int index = OffsetOfSubstring( line, token );
    if ( index != -1 )
        function( line, index );
}

void ProcessSourceFile( char *filename )
{
   FILE *source;
   char line[MAX_LINE_LENGTH];
  
   if( strcmp( filename, DO_NOT_PROCESS ) != 0 )
   {

      source = fopen(filename,"r");
      
      while ( fgets(line,MAX_LINE_LENGTH,source) )
      {
         CallIfSubstringFound( line, SEARCH_TOKEN_BRINGUP, ProcessBringupFunction );
         CallIfSubstringFound( line, SEARCH_TOKEN_TEST, ProcessTestFunction );
         CallIfSubstringFound( line, SEARCH_TOKEN_TAKEDOWN, ProcessTakedownFunction );
      }
      
      fclose(source);
   }
}

void EmitExternDeclarationFor( char *name, char *prefix )
{
    fprintf( outfile, "extern void %s%s( void );\n", prefix, name );
}

void Emit(char *text)
{
   fprintf(outfile, "%s\n", text);
}

void BlankLine()
{
    Emit( "" );
}

void ListExternalFunctions()
{
   TestItem *current = testList;
   while ( current )
   {
      if (current->type == TYPE_TEST)
         EmitExternDeclarationFor( current->name, SEARCH_TOKEN_TEST );
      else if (current->type == TYPE_BRINGUP)
         EmitExternDeclarationFor( current->name, SEARCH_TOKEN_BRINGUP );
      else if (current->type == TYPE_TAKEDOWN)
         EmitExternDeclarationFor( current->name, SEARCH_TOKEN_TAKEDOWN );
      current = current->next;
   }
   
   BlankLine();
}

void EmitLibrary(void)
{
   int i=0;
   while ( libCUT[i] )
      Emit(libCUT[i++]);
}

void ListHeaderFiles(void)
{
    EmitLibrary();
    BlankLine();
    BlankLine();
}

void EmitIndented(int indent,char *format, ...)
{
   va_list v;
   /* Print two spaces per level of indentation. */
   fprintf( outfile, "%*s", indent*2, "" );
   
   va_start(v,format);
   vfprintf( outfile, format, v );
   va_end(v);
   
   fprintf( outfile, "\n" );
}

void EmitBringup(int indent,char *name)
{
    BlankLine();
    EmitIndented(indent, "cut_start( \"group-%s\", __CUT_TAKEDOWN__%s );", 
           name, name );
    EmitIndented(indent, "__CUT_BRINGUP__%s();", name );
    EmitIndented(indent, "cut_check_errors();");
}

void EmitTest(int indent,char *name)
{
    EmitIndented(indent, "cut_start( \"%s\", 0 );", name );
    EmitIndented(indent, "__CUT__%s();", name );
    EmitIndented(indent, "cut_end( \"%s\" );", name );
}

void EmitTakedown(int indent,char *name)
{
    EmitIndented(indent, "cut_end( \"group-%s\" );", name );
    EmitIndented(indent, "__CUT_TAKEDOWN__%s();", name );
    BlankLine();
}

void EmitUnitTesterBody()
{
    int indent=0;
    TestItem *test;
    Emit( "int main1( int argc, char *argv[] )\n{" );
    Emit( "  if ( argc == 1 )" );
    Emit( "    cut_init( -1 );" );
    Emit( "  else cut_init( atoi( argv[1] ) );" );
    BlankLine();
    
    indent = 1;
    test = testList;
    while ( test )
    {
      if (test->type == TYPE_BRINGUP)
      {
         EmitBringup(indent,test->name);
         indent ++;
      }

      if (test->type == TYPE_TEST)
         EmitTest(indent,test->name);

      if (test->type == TYPE_TAKEDOWN)
      {
         indent --;
         EmitTakedown(indent,test->name);
      }
      test = test->next;
    }
    
    BlankLine();
    Emit( "  cut_break_formatting();" );
    Emit( "  printf(\"Done.\");" );
    Emit( "  return 0;\n}\n" );
}

void EmitCutCheck()
{
    Emit( "/* Automatically generated: DO NOT MODIFY. */" );
    ListHeaderFiles();
    BlankLine();
    ListExternalFunctions();
    BlankLine();
    EmitUnitTesterBody();
}

//#if defined(_MSC_VER) // version tested for MS Visual C++ 6.0
//#include <io.h>
//
//void FileName(char *base)
//{
//   char *pos = strrchr(g_wildcards[g_index],'/');
//   char *pos2 = strrchr(g_wildcards[g_index],'\\');
//   if ( !pos || pos2 > pos )
//      pos = pos2;
//
//   if ( pos )
//   {
//      size_t length = 1 + pos - g_wildcards[g_index];
//      strncpy(g_fileName,g_wildcards[g_index],length);
//      g_fileName[length] = 0;
//   }
//   else g_fileName[0] = 0;
//   strcat(g_fileName,base);
//}
//
//int LoadArgument(void)
//{
//   static struct _finddata_t fd;
//   static long handle = -1;
//
//   if ( g_index>=g_count )
//      return 0;
//
//   if ( -1 == handle )
//   {
//      handle = _findfirst(g_wildcards[g_index], &fd);
//      if ( -1 == handle ) // there MUST be at least a first match for each wildcard.
//         return 0;
//      FileName(fd.name);
//      return 1;
//   }
//   else if ( 0 == _findnext(handle,&fd) )
//   { // there's a 'next' filename
//      FileName(fd.name);
//      return 1;
//   }
//   else
//   { // this wasn't the first filename, and there isn't a next.
//      _findclose(handle);
//      handle = -1;
//      g_index++;
//      return LoadArgument();
//   }
//}
//
//#endif
//#if defined(__LINUX__)

void FileName( char *base )
{
    strncpy( g_fileName, base, MAX_LINE_LENGTH );
    g_fileName[ MAX_LINE_LENGTH - 1 ] = 0;
}

int LoadArgument( void )
{
   if ( g_index >= g_count )
      return 0;

   FileName( g_wildcards[g_index] );
   g_index++;  /* MUST come after FileName() call; bad code smell */
   return 1;
}

//#endif

void StartArguments( int starting, int count, char *array[] )
{
   g_index = starting;
   g_count = count;
   g_wildcards = array;

   g_ready = LoadArgument();
}

int NextArgument( void )
{
   if( g_ready )
      g_ready = LoadArgument();

   return g_ready;
}

char *GetArgument( void )
{
   if( g_ready )
      return g_fileName;

   return NULL;
}

void EstablishOutputFile( int argc, char *argv[] )
{
   int i;

   i = 0;
   while( i < argc )
   {
      if( ( argv[i+1] != NULL ) && ( strcmp( argv[i], "-o" ) == 0 ) )
      {
         outfile = fopen( argv[i+1], "wb+" );
         if( outfile == NULL )
            fprintf( stderr, "ERROR: Can't open %s for writing.\n", argv[i+1] );
         
         argv[i] = argv[i+1] = DO_NOT_PROCESS;

         return;
      }

      i++;
   }

   outfile = stdout;
}

int main1( int argc,char *argv[] )
{
   char *filename;

   if ( argc < 2 )
   {
      fprintf(
              stderr,
              "USAGE:\n"
              "   %s [options] <input file> [<input file> [...]]\n"
              "\n"
              "OPTIONS:\n"
              "   -o filename   Specifies output file.\n"
              "\n"
              "NOTES:\n"
              "   If -o is left unspecified, output defaults to stdout.\n",
              argv[0]
             );
      return 3;
   }

   EstablishOutputFile( argc, argv );

   /* Skip the executable's name and the output filename. */
   StartArguments(0,argc,argv);

   /* Consume the rest of the arguments, one at a time. */
   while ( NextArgument() )
   {
      filename = GetArgument();

      if( strcmp( filename, DO_NOT_PROCESS ) )
      {
         fprintf( stderr, "  - parsing '%s'... ", filename);
         ProcessSourceFile( filename );
         fprintf( stderr, "done.\n");
      }
   }
   
   EmitCutCheck();
   fflush(outfile);
   fclose(outfile);
   return 0;
}

char * libCUT[] =
{
   "/*",
   " * libcut.inc",
   " * CUT 2.1",
   " *",
   " * Copyright (c) 2001-2002 Samuel A. Falvo II, William D. Tanksley",
   " * See LICENSE.TXT for details.",
   " *",
   " * Based on WDT's 'TestAssert' package.",
   " *",
   " * $log$",
   " */",
   "",
   "#include <string.h>",
   "#include <stdlib.h>",
   "#include <stdio.h>",
   "#include <stdarg.h>",
   "#include \"cut.h\"",
   "",
   "#ifndef BOOL		/* Just in case -- helps in portability */",
   "#define BOOL int",
   "#endif",
   "",
   "#ifndef FALSE",
   "#define FALSE (0)",
   "#endif",
   "",
   "#ifndef TRUE",
   "#define TRUE 1",
   "#endif",
   "",
   "typedef struct NameStackItem   NameStackItem;",
   "typedef struct NameStackItem  *NameStack;",
   "",
   "struct NameStackItem",
   "{",
   "  NameStackItem *      next;",
   "  char *               name;",
   "  CUTTakedownFunction *takedown;",
   "};",
   "",
   "static int            breakpoint = 0;",
   "static int            count = 0;",
   "static BOOL           test_hit_error = FALSE;",
   "static NameStack      nameStack;",
   "",
   "static void traceback( void );",
   "static void cut_exit( void );",
   "",
   "/* I/O Functions */",
   "",
   "static void print_string( char *string )",
   "{",
   "  printf( \"%s\", string );",
   "  fflush( stdout );",
   "}",
   "",
   "static void print_string_as_error( char *filename, int lineNumber, char *string )",
   "{",
   "  printf( \"%s(%d): %s\", filename, lineNumber, string );",
   "  fflush( stdout );",
   "}",
   "",
   "static void print_integer_as_expected( int i )",
   "{",
   "  printf( \"(signed) %d (unsigned) %u (hex) 0x%08lX\", i, i, i );",
   "}",
   "",
   "static void print_integer( int i )",
   "{",
   "  printf( \"%d\", i );",
   "  fflush( stdout );",
   "}",
   "",
   "static void print_integer_in_field( int i, int width )",
   "{",
   "  printf( \"%*d\", width, i );",
   "  fflush( stdout );",
   "}",
   "",
   "static void new_line( void )",
   "{",
   "  printf( \"\\n\" );",
   "  fflush( stdout );",
   "}",
   "",
   "static void print_character( char ch )",
   "{",
   "  printf( \"%c\", ch );",
   "  fflush( stdout );",
   "}",
   "",
   "static void dot( void )",
   "{",
   "  print_character( '.' );",
   "}",
   "",
   "static void space( void )",
   "{",
   "  print_character( ' ' );",
   "}",
   "",
   "/* Name Stack Functions */",
   "",
   "static NameStackItem *stack_topOf( NameStack *stack )",
   "{",
   "  return *stack;",
   "}",
   "",
   "static BOOL stack_isEmpty( NameStack *stack )",
   "{",
   "  return stack_topOf( stack ) == NULL;",
   "}",
   "",
   "static BOOL stack_isNotEmpty( NameStack *stack )",
   "{",
   "  return !( stack_isEmpty( stack ) );",
   "}",
   "",
   "static void stack_push( NameStack *stack, char *name, CUTTakedownFunction *tdfunc )",
   "{",
   "  NameStackItem *item;",
   "",
   "  item = (NameStackItem *)( malloc( sizeof( NameStackItem ) ) );",
   "  if( item != NULL )",
   "  {",
   "    item -> next = stack_topOf( stack );",
   "    item -> name = name;",
   "    item -> takedown = tdfunc;",
   "",
   "    *stack = item;",
   "  }",
   "}",
   "",
   "static void stack_drop( NameStack *stack )",
   "{",
   "  NameStackItem *oldItem;",
   "",
   "  if( stack_isNotEmpty( stack ) )",
   "  {",
   "    oldItem = stack_topOf( stack );",
   "    *stack = oldItem -> next;",
   "",
   "    free( oldItem );",
   "  }",
   "}",
   "",
   "/* CUT Initialization and Takedown  Functions */",
   "",
   "void cut_init( int brkpoint )",
   "{",
   "  breakpoint = brkpoint;",
   "  count = 0;",
   "  test_hit_error = FALSE;",
   "  nameStack = NULL;",
   "",
   "  if( brkpoint >= 0 )",
   "  {",
   "    print_string( \"Breakpoint at test \" );",
   "    print_integer( brkpoint );",
   "    new_line();",
   "  }",
   "}",
   "",
   "void cut_exit( void )",
   "{",
   "  exit( test_hit_error != FALSE );",
   "}",
   "",
   "/* User Interface functions */",
   "",
   "static void print_group( int position, int base, int leftover )",
   "{",
   "  if( !leftover )",
   "    return;",
   "",
   "  print_integer_in_field( base, position );",
   "  while( --leftover )",
   "    dot();",
   "}",
   "",
   "static void print_recap( int count )",
   "{",
   "  int countsOnLastLine = count % 50;",
   "  int groupsOnLastLine = countsOnLastLine / 10;",
   "  int dotsLeftOver = countsOnLastLine % 10;",
   "  int lastGroupLocation =",
   "     countsOnLastLine - dotsLeftOver + ( 4 * groupsOnLastLine ) + 5;",
   "",
   "  if( dotsLeftOver == 0 )",
   "  {",
   "    if( countsOnLastLine == 0 )",
   "      lastGroupLocation = 61;",
   "    else",
   "      lastGroupLocation -= 14;",
   "",
   "    print_group( lastGroupLocation, countsOnLastLine-10, 10);",
   "  }",
   "  else",
   "  {",
   "    print_group(",
   "                lastGroupLocation,",
   "                countsOnLastLine - dotsLeftOver,",
   "                dotsLeftOver",
   "               );",
   "  }",
   "}",
   "",
   "void cut_break_formatting( void ) // DEPRECATED: Do not use in future software",
   "{",
   "  new_line();",
   "}",
   "",
   "void cut_resume_formatting( void )",
   "{",
   "  new_line();",
   "  print_recap( count );",
   "}",
   "",
   "void cut_interject( const char *comment, ... )",
   "{",
   "  va_list marker;",
   "  va_start(marker,comment);",
   "  ",
   "  cut_break_formatting();",
   "  vprintf(comment,marker);",
   "  cut_resume_formatting();",
   "  ",
   "  va_end(marker);",
   "}",
   "",
   "/* Test Progress Accounting functions */",
   "",
   "void __cut_mark_point( char *filename, int lineNumber )",
   "{",
   "  if( ( count % 10 ) == 0 )",
   "  {",
   "    if( ( count % 50 ) == 0 )",
   "      new_line();",
   "",
   "    print_integer_in_field( count, 5 );",
   "  }",
   "  else",
   "    dot();",
   "",
   "  count++;",
   "  if( count == breakpoint )",
   "  {",
   "    print_string_as_error( filename, lineNumber, \"Breakpoint hit\" );",
   "    new_line();",
   "    traceback();",
   "    cut_exit();",
   "  }",
   "}",
   "",
   "void __cut_assert_equals( // DEPRECATED: Do not use in future software",
   "                         char *filename,",
   "                         int   lineNumber,",
   "                         char *message,",
   "                         char *expression,",
   "                         BOOL  success,",
   "                         int   expected",
   "                        )",
   "{",
   "  __cut_mark_point( filename, lineNumber );",
   "  ",
   "  if( success != FALSE )",
   "    return;",
   "  ",
   "  cut_break_formatting();",
   "  print_string_as_error( filename, lineNumber, message );",
   "  new_line();",
   "  print_string_as_error( filename, lineNumber, \"Failed expression: \" );",
   "  print_string( expression );",
   "  new_line();",
   "  print_string_as_error( filename, lineNumber, \"Actual value: \" );",
   "  print_integer_as_expected( expected );",
   "  new_line();",
   "",
   "  test_hit_error = TRUE;",
   "  cut_resume_formatting();",
   "}",
   "",
   "",
   "void __cut_assert(",
   "                  char *filename,",
   "                  int   lineNumber,",
   "                  char *message,",
   "                  char *expression,",
   "                  BOOL  success",
   "                 )",
   "{",
   "  __cut_mark_point( filename, lineNumber );",
   "  ",
   "  if( success != FALSE )",
   "    return;",
   "  ",
   "  cut_break_formatting();",
   "  print_string_as_error( filename, lineNumber, message );",
   "  new_line();",
   "  print_string_as_error( filename, lineNumber, \"Failed expression: \" );",
   "  print_string( expression );",
   "  new_line();",
   "",
   "  test_hit_error = TRUE;",
   "  cut_resume_formatting();",
   "}",
   "",
   "",
   "/* Test Delineation and Teardown Support Functions */",
   "",
   "static void traceback()",
   "{",
   "  if( stack_isNotEmpty( &nameStack ) )",
   "    print_string( \"Traceback\" );",
   "  else",
   "    print_string( \"(No traceback available.)\" );",
   "",
   "  while( stack_isNotEmpty( &nameStack ) )",
   "  {",
   "    print_string( \": \" );",
   "    print_string( stack_topOf( &nameStack ) -> name );",
   "",
   "    if( stack_topOf( &nameStack ) -> takedown != NULL )",
   "    {",
   "      print_string( \"(taking down)\" );",
   "      stack_topOf( &nameStack ) -> takedown();",
   "    }",
   "",
   "    stack_drop( &nameStack );",
   "",
   "    if( stack_isNotEmpty( &nameStack ) )",
   "      space();",
   "  }",
   "",
   "  new_line();",
   "}",
   "",
   "void cut_start( char *name, CUTTakedownFunction *takedownFunction )",
   "{",
   "  stack_push( &nameStack, name, takedownFunction );",
   "}",
   "",
   "int __cut_check_errors( char *filename, int lineNumber )",
   "{",
   "  if( test_hit_error || stack_isEmpty( &nameStack ) )",
   "  {",
   "    cut_break_formatting();",
   "    if( stack_isEmpty( &nameStack ) )",
   "      print_string_as_error( filename, lineNumber, \"Missing cut_start(); no traceback possible.\" );",
   "    else",
   "      traceback();",
   "",
   "    cut_exit();",
   "  } else return 1;",
   "}",
   "",
   "void __cut_end( char *filename, int lineNumber, char *closingFrame )",
   "{",
   "  if( test_hit_error || stack_isEmpty( &nameStack ) )",
   "  {",
   "    cut_break_formatting();",
   "    if( stack_isEmpty( &nameStack ) )",
   "      print_string_as_error( filename, lineNumber, \"Missing cut_start(); no traceback possible.\" );",
   "    else",
   "      traceback();",
   "",
   "    cut_exit();",
   "  }",
   "  else",
   "  {",
   "    if( strcmp( stack_topOf( &nameStack ) -> name, closingFrame ) == 0 )",
   "      stack_drop( &nameStack );",
   "    else",
   "    {",
   "      print_string_as_error( filename, lineNumber, \"Mismatched cut_end().\" );",
   "      traceback();",
   "      cut_exit();",
   "    }",
   "  }",
   "}",
   0
};

/*
 * vim: tabstop=3 shiftwidth=3 expandtab 
 */

