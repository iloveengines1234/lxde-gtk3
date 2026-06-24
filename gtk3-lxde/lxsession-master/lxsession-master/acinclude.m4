# acinclude.m4
# 
# Checks the location of the XML Catalog
# Usage:
#   JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# Defines XMLCATALOG and XML_CATALOG_FILE substitutions
# Updated by iloveengines1234 to port completely to GTK 3 / Modern Autoconf

AC_DEFUN([JH_PATH_XML_CATALOG],
[
  # Check for the custom presence or location override of the XML catalog
  AC_ARG_WITH([xml-catalog],
              [AS_HELP_STRING([--with-xml-catalog=CATALOG],
                              [path to xml catalog to use (default: /etc/xml/catalog)])],
              [with_xml_catalog=$withval],
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

  # Check for the xmlcatalog validation utility binary program
  AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
  if test "x$XMLCATALOG" = xno; then
    jh_found_xmlcatalog=false
  fi

  if test "x$jh_found_xmlcatalog" = xtrue; then
    m4_ifval([$1], [$1], [:])
  else
    m4_ifval([$2], [$2], [AC_MSG_ERROR([could not find valid XML catalog setup])])
  fi
])

# Checks if a particular URI reference schema appears in the XML catalog maps
# Usage:
#   JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
  # Modern Fix: Coerce absolute sequence resolution by requiring baseline paths safely
  AC_REQUIRE([JH_PATH_XML_CATALOG])dnl
  
  AC_MSG_CHECKING([for m4_default([$2], [$1]) in XML catalog])
  
  jh_uri_found=false
  if test "x$jh_found_xmlcatalog" = xtrue; then
    # Modern Fix: Replaced dangerous AC_RUN_LOG internal tracking macro with standard 
    # shell redirection to ensure cross-compilation environments evaluate natively.
    if "$XMLCATALOG" "$XML_CATALOG_FILE" "$1" >/dev/null 2>&1; then
      jh_uri_found=true
    fi
  fi
  
  if test "x$jh_uri_found" = xtrue; then
    AC_MSG_RESULT([found])
    m4_ifval([$3], [$3])dnl
  else
    AC_MSG_RESULT([not found])
    m4_ifval([$4], [$4],
       [AC_MSG_ERROR([could not locate required schema entry m4_default([$2], [$1]) inside the XML catalog])])
  fi
])