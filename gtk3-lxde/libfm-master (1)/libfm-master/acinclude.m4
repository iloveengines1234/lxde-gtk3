dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

dnl AC_CHECK_LDFLAG(CACHEVAR, FLAGS, APPENDVAR)
dnl Safely tests if the linker/compiler chain supports a given flag.
AC_DEFUN([AC_CHECK_LDFLAG],
[
  AC_CACHE_CHECK([whether the target compiler accepts $2], [$1],
    [
      dnl Save clean compilation states
      save_LDFLAGS="$LDFLAGS"
      LDFLAGS="$2 $LDFLAGS"
      
      dnl Fixed: Use modern compile-link check using exit code validation instead of raw output matching
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([[]], [[return 0;]])],
        [$1=yes],
        [$1=no]
      )
      
      dnl Restore environmental states safely
      LDFLAGS="$save_LDFLAGS"
    ])

  dnl Fixed: Re-architected with standard m4 expansion mapping to avoid variable indirection syntax faults
  AS_IF([test "x$$1" = xyes],
    [
      $3="$2 ${$3}"
    ])
])