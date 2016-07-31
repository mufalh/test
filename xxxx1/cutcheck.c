/* Automatically generated: DO NOT MODIFY. */
/*
 * libcut.inc
 * CUT 2.1
 *
 * Copyright (c) 2001-2002 Samuel A. Falvo II, William D. Tanksley
 * See LICENSE.TXT for details.
 *
 * Based on WDT's 'TestAssert' package.
 *
 * $log$
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "cut.h"

#ifndef BOOL		/* Just in case -- helps in portability */
#define BOOL int
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef struct NameStackItem   NameStackItem;
typedef struct NameStackItem  *NameStack;

struct NameStackItem
{
  NameStackItem *      next;
  char *               name;
  CUTTakedownFunction *takedown;
};

static int            breakpoint = 0;
static int            count = 0;
static BOOL           test_hit_error = FALSE;
static NameStack      nameStack;

static void traceback( void );
static void cut_exit( void );

/* I/O Functions */

static void print_string( char *string )
{
  printf( "%s", string );
  fflush( stdout );
}

static void print_string_as_error( char *filename, int lineNumber, char *string )
{
  printf( "%s(%d): %s", filename, lineNumber, string );
  fflush( stdout );
}

static void print_integer_as_expected( int i )
{
  printf( "(signed) %d (unsigned) %u (hex) 0x%08X", i, i, i );
}

static void print_integer( int i )
{
  printf( "%d", i );
  fflush( stdout );
}

static void print_integer_in_field( int i, int width )
{
  printf( "%*d", width, i );
  fflush( stdout );
}

static void new_line( void )
{
  printf( "\n" );
  fflush( stdout );
}

static void print_character( char ch )
{
  printf( "%c", ch );
  fflush( stdout );
}

static void dot( void )
{
  print_character( '.' );
}

static void space( void )
{
  print_character( ' ' );
}

/* Name Stack Functions */

static NameStackItem *stack_topOf( NameStack *stack )
{
  return *stack;
}

static BOOL stack_isEmpty( NameStack *stack )
{
  return stack_topOf( stack ) == NULL;
}

static BOOL stack_isNotEmpty( NameStack *stack )
{
  return !( stack_isEmpty( stack ) );
}

static void stack_push( NameStack *stack, char *name, CUTTakedownFunction *tdfunc )
{
  NameStackItem *item;

  item = (NameStackItem *)( malloc( sizeof( NameStackItem ) ) );
  if( item != NULL )
  {
    item -> next = stack_topOf( stack );
    item -> name = name;
    item -> takedown = tdfunc;

    *stack = item;
  }
}

static void stack_drop( NameStack *stack )
{
  NameStackItem *oldItem;

  if( stack_isNotEmpty( stack ) )
  {
    oldItem = stack_topOf( stack );
    *stack = oldItem -> next;

    free( oldItem );
  }
}

/* CUT Initialization and Takedown  Functions */

void cut_init( int brkpoint )
{
  breakpoint = brkpoint;
  count = 0;
  test_hit_error = FALSE;
  nameStack = NULL;

  if( brkpoint >= 0 )
  {
    print_string( "Breakpoint at test " );
    print_integer( brkpoint );
    new_line();
  }
}

void cut_exit( void )
{
  exit( test_hit_error != FALSE );
}

/* User Interface functions */

static void print_group( int position, int base, int leftover )
{
  if( !leftover )
    return;

  print_integer_in_field( base, position );
  while( --leftover )
    dot();
}

static void print_recap( int count )
{
  int countsOnLastLine = count % 50;
  int groupsOnLastLine = countsOnLastLine / 10;
  int dotsLeftOver = countsOnLastLine % 10;
  int lastGroupLocation =
     countsOnLastLine - dotsLeftOver + ( 4 * groupsOnLastLine ) + 5;

  if( dotsLeftOver == 0 )
  {
    if( countsOnLastLine == 0 )
      lastGroupLocation = 61;
    else
      lastGroupLocation -= 14;

    print_group( lastGroupLocation, countsOnLastLine-10, 10);
  }
  else
  {
    print_group(
                lastGroupLocation,
                countsOnLastLine - dotsLeftOver,
                dotsLeftOver
               );
  }
}

void cut_break_formatting( void ) // DEPRECATED: Do not use in future software
{
  new_line();
}

void cut_resume_formatting( void )
{
  new_line();
  print_recap( count );
}

void cut_interject( const char *comment, ... )
{
  va_list marker;
  va_start(marker,comment);
  
  cut_break_formatting();
  vprintf(comment,marker);
  cut_resume_formatting();
  
  va_end(marker);
}

/* Test Progress Accounting functions */

void __cut_mark_point( char *filename, int lineNumber )
{
  if( ( count % 10 ) == 0 )
  {
    if( ( count % 50 ) == 0 )
      new_line();

    print_integer_in_field( count, 5 );
  }
  else
    dot();

  count++;
  if( count == breakpoint )
  {
    print_string_as_error( filename, lineNumber, "Breakpoint hit" );
    new_line();
    traceback();
    cut_exit();
  }
}

void __cut_assert_equals( // DEPRECATED: Do not use in future software
                         char *filename,
                         int   lineNumber,
                         char *message,
                         char *expression,
                         BOOL  success,
                         int   expected
                        )
{
  __cut_mark_point( filename, lineNumber );
  
  if( success != FALSE )
    return;
  
  cut_break_formatting();
  print_string_as_error( filename, lineNumber, message );
  new_line();
  print_string_as_error( filename, lineNumber, "Failed expression: " );
  print_string( expression );
  new_line();
  print_string_as_error( filename, lineNumber, "Actual value: " );
  print_integer_as_expected( expected );
  new_line();

  test_hit_error = TRUE;
  cut_resume_formatting();
}


void __cut_assert(
                  char *filename,
                  int   lineNumber,
                  char *message,
                  char *expression,
                  BOOL  success
                 )
{
  __cut_mark_point( filename, lineNumber );
  
  if( success != FALSE )
    return;
  
  cut_break_formatting();
  print_string_as_error( filename, lineNumber, message );
  new_line();
  print_string_as_error( filename, lineNumber, "Failed expression: " );
  print_string( expression );
  new_line();

  test_hit_error = TRUE;
  cut_resume_formatting();
}


/* Test Delineation and Teardown Support Functions */

static void traceback()
{
  if( stack_isNotEmpty( &nameStack ) )
    print_string( "Traceback" );
  else
    print_string( "(No traceback available.)" );

  while( stack_isNotEmpty( &nameStack ) )
  {
    print_string( ": " );
    print_string( stack_topOf( &nameStack ) -> name );

    if( stack_topOf( &nameStack ) -> takedown != NULL )
    {
      print_string( "(taking down)" );
      stack_topOf( &nameStack ) -> takedown();
    }

    stack_drop( &nameStack );

    if( stack_isNotEmpty( &nameStack ) )
      space();
  }

  new_line();
}

void cut_start( char *name, CUTTakedownFunction *takedownFunction )
{
  stack_push( &nameStack, name, takedownFunction );
}

int __cut_check_errors( char *filename, int lineNumber )
{
  if( test_hit_error || stack_isEmpty( &nameStack ) )
  {
    cut_break_formatting();
    if( stack_isEmpty( &nameStack ) )
      print_string_as_error( filename, lineNumber, "Missing cut_start(); no traceback possible." );
    else
      traceback();

    cut_exit();
  } else return 1;
}

void __cut_end( char *filename, int lineNumber, char *closingFrame )
{
  if( test_hit_error || stack_isEmpty( &nameStack ) )
  {
    cut_break_formatting();
    if( stack_isEmpty( &nameStack ) )
      print_string_as_error( filename, lineNumber, "Missing cut_start(); no traceback possible." );
    else
      traceback();

    cut_exit();
  }
  else
  {
    if( strcmp( stack_topOf( &nameStack ) -> name, closingFrame ) == 0 )
      stack_drop( &nameStack );
    else
    {
      print_string_as_error( filename, lineNumber, "Mismatched cut_end()." );
      traceback();
      cut_exit();
    }
  }
}



extern void __CUT_BRINGUP__Pass( void );
extern void __CUT__TestA( void );
extern void __CUT__TestB( void );
extern void __CUT__TestC( void );
extern void __CUT__TestD( void );
extern void __CUT_TAKEDOWN__Pass( void );


int main( int argc, char *argv[] )
{
  if ( argc == 1 )
    cut_init( -1 );
  else cut_init( atoi( argv[1] ) );


  cut_start( "group-Pass", __CUT_TAKEDOWN__Pass );
  __CUT_BRINGUP__Pass();
  cut_check_errors();
    cut_start( "TestA", 0 );
    __CUT__TestA();
    cut_end( "TestA" );
    cut_start( "TestB", 0 );
    __CUT__TestB();
    cut_end( "TestB" );
    cut_start( "TestC", 0 );
    __CUT__TestC();
    cut_end( "TestC" );
    cut_start( "TestD", 0 );
    __CUT__TestD();
    cut_end( "TestD" );
  cut_end( "group-Pass" );
  __CUT_TAKEDOWN__Pass();


  cut_break_formatting();
  printf("Done.");
  return 0;
}

