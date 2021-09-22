#define VSTR_VERSION_C
/*
 *  Copyright (C) 1999, 2000, 2001, 2002, 2003  James Antill
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  email: james@and.org
 */

/* prints the version and some info out when you run the library */

#include "main.h"


#ifndef  USE_SYSCALL_MAIN
# define USE_SYSCALL_MAIN 0 /* use for coverage, does "normal" syscalls */
#endif

/* syscall on Linux x86 doesn't work in a shared library as the PIC code
 * uses %bx. If you are compiling for static only, the this should work.
 * However then you can't run the library anyway.
 *
 * Code does inline asm for i386 Linux */
#if (defined(HAVE_SYSCALL_H) && defined(USE_SYSCALL))
#include <syscall.h>

# define write(x, y, z) vstr__sys_write(x, y, z)
static _syscall3(int, write, int, fd, const void *, buf, int, count)
# define exit(x) vstr__sys_exit(x)
static _syscall1(int, exit, int, ret)
#endif

void vstr_version_func(void)
{
  int fd = 1;
  const char *const msg = "\n"
       "Vstr library release version -- 1.0.15 --, by James Antill.\n"
       "Copyright (C) 1999, 2000, 2001, 2002, 2003 James Antill.\n"
       "This is free software; see the source for copying conditions.\n"
       "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n"
       "PARTICULAR PURPOSE.\n\n"
       "Built as follows:\n"
       "  Compiled on " __DATE__ " at " __TIME__ ".\n"
#ifdef __VERSION__
       "  Compiled by CC version " __VERSION__ ".\n"
#endif

#ifndef NDEBUG
 "  Debugging enabled (CFLAGS = -g -O2).\n"
#else
 "  No debugging (CFLAGS = -g -O2)\n"
#endif

#ifdef HAVE_POSIX_HOST
 "  Running on a POSIX host.\n"
#else
 "  No POSIX host (iovec is manually defined, and vstr_sc_* are stubs)\n"
#endif

#ifdef USE_RESTRICTED_HEADERS
 "  Compiled with restricted functionality in lib C.\n"
#endif

#if defined(VSTR_AUTOCONF_FMT_DBL_glibc)
 "  Formatting floats using -- glibc -- code.\n"
#elif defined(VSTR_AUTOCONF_FMT_DBL_none)
 "  Formatting floats using -- none -- code (all floats are zero).\n"
#elif defined(VSTR_AUTOCONF_FMT_DBL_host)
 "  Formatting floats using -- host -- code.\n"
#else
# error "Please configure properly..."
#endif


 "  Compiler supports attributes:\n    "
#ifdef HAVE_ATTRIB_DEPRECATED
 " deprecated"
#endif

#ifdef HAVE_ATTRIB_MALLOC
 " malloc"
#endif

#ifdef HAVE_ATTRIB_NONNULL
 " nonnull"
#endif

#ifdef HAVE_ATTRIB_PURE
 " pure"
#endif

#ifdef HAVE_ATTRIB_PURE
 " const"
#endif

#if VSTR__USE_INTERNAL_SYMBOLS
 "\n  Internal functions can be restricted for speed and API purity.\n"
#else
 "\n  Internal functions are exported.\n"
#endif

#ifdef HAVE_INLINE
 "  Functions can be inlined for speed.\n"
#else
 "  Functions cannot be inlined.\n"
#endif

#ifdef HAVE_LINKER_SCRIPT
 "  Linker script enabled.\n"
#endif


 "\n"
"Information can be found at:\t\t\t\thttp://www.and.org/vstr/"
       "\n"
"Bug reports should be sent to:\t\t\t    James Antill <james@and.org>"
       "\n\n"
    ;

  const char *scan = NULL;
  int len = 0;

#if ((!USE_SYSCALL_MAIN) && defined(USE_SYSCALL_ASM) && \
     (defined(__linux__) && defined(__i386__)))
 /* this is sick, but works. */

 /* write syscall for Linux x86 */
#  define VSTR__SYS_WRITE(ret, fd, msg, len) \
  __asm__ __volatile__ ("\
mov     $4, %%eax\n\t\
mov     %1, %%ebx\n\t\
mov     %2, %%ecx\n\t\
mov     %3, %%edx\n\t\
int     $0x80\n\t\
" : "=a"(ret) : "m"(fd), "m"(msg), "m"(len))

  /* SUCCESS exit syscall for Linux x86 */
#  define VSTR__SYS_EXIT_S() do { \
  __asm__ __volatile__ ("\
mov     $1, %%eax\n\t\
mov     $0, %%ebx\n\t\
int     $0x80\n\t\
" : : ); } while (FALSE)
  /* FAILURE exit syscall for Linux x86 */
#  define VSTR__SYS_EXIT_F() do { \
  __asm__ __volatile__ ("\
mov     $1, %%eax\n\t\
mov     $1, %%ebx\n\t\
int     $0x80\n\t\
" : : ); } while (FALSE)

#elif (!USE_SYSCALL_MAIN) && (defined(HAVE_SYSCALL_H) && defined(USE_SYSCALL))
 /* syscall code for write/exit */
 /* See above for actual syscall implimentations */

#  define VSTR__SYS_WRITE(ret, fd, msg, len) ret = vstr__sys_write(fd, msg, len)
#  define VSTR__SYS_EXIT_S() vstr__sys_exit(EXIT_SUCCESS)
#  define VSTR__SYS_EXIT_F() vstr__sys_exit(EXIT_FAILURE)

#else
#  include "main.h"

  /* this is obvious, well apart from the fact that it doesn't work unless
   * run via. ld.so */
#  define VSTR__SYS_WRITE(ret, fd, msg, len) ret = write(fd, msg, len)
#  define VSTR__SYS_EXIT_S() exit(EXIT_SUCCESS)
#  define VSTR__SYS_EXIT_F() exit(EXIT_FAILURE)
#endif

  scan = msg;
  while (*scan) ++scan;
  len = (scan - msg);
  scan = msg;

  while (len > 0)
  {
    int ret = -1;

    VSTR__SYS_WRITE(ret, fd, scan, len);

    if (ret < 0)
      VSTR__SYS_EXIT_F();

    scan += ret;
    len  -= ret;
  }

  VSTR__SYS_EXIT_S();
}
