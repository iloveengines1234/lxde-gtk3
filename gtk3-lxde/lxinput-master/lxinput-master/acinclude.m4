# Checks the location of the XML Catalog
# Usage:
#   JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# Defines XMLCATALOG and XML_CATALOG_FILE substitutions
# Updated by iloveengines1234 to port completely to GTK 3 / Modern Autoconf

AC_DEFUN([JH_PATH_XML_CATALOG],
[
  # check for the presence of the XML catalog
  AC_ARG_WITH([xml-catalog],
              [AS_HELP_STRING([--with-xml-catalog=CATALOG],
                              [path to xml catalog to use])],,
              [with_xml_catalog=/etc/xml/catalog])
  jh_found_xmlcatalog=true
  XML_CATALOG_FILE="$with_xml_catalog"
  AC_SUBST([XML_CATALOG_FILE])
  AC_MSG_CHECKING([for XML catalog ($XML_CATALOG_FILE)])
  if test -f "$XML_CATALOG_FILE"; then
    AC_MSG_RESULT([found])
  else
    jh_found_xmlcatalog=false
    AC_MSG_RESULT([not found])
  fi

  # check for the xmlcatalog program
  AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
  if test "x$XMLCATALOG" = xno; then
    jh_found_xmlcatalog=false
  fi

  if $jh_found_xmlcatalog; then
    m4_ifval([$1], [$1], [:])
  else
    m4_ifval([$2], [$2], [AC_MSG_ERROR([could not find XML catalog])])
  fi
])

# Checks if a particular URI appears in the XML catalog
# Usage:
#   JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
  # Modern Fix: Swapped out the old Autoconf validation framework macro AS_RUN_LOG.
  # Modern autoconf engines utilize AS_IF alongside generic system verification loops 
  # or standard evaluation commands to guarantee robust macro expansions.
  AC_REQUIRE([JH_PATH_XML_CATALOG])dnl
  AC_MSG_CHECKING([for m4_default([$2], [$1]) in XML catalog])
  
  if $jh_found_xmlcatalog && \
     $XMLCATALOG --noout "$XML_CATALOG_FILE" "$1" >&2; then
    AC_MSG_RESULT([found])
    m4_ifval([$3], [$3])dnl
  else
    AC_MSG_RESULT([not found])
    m4_ifval([$4], [$4],
       [AC_MSG_ERROR([could not find m4_default([$2], [$1]) in XML catalog])])
  fi
])