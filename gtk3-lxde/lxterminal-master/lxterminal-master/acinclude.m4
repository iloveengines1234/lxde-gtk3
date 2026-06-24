dnl JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Checks for the path of the XML Catalog on the host system.
dnl Defines XMLCATALOG and XML_CATALOG_FILE substitutions.
AC_DEFUN([JH_PATH_XML_CATALOG],
[
  # Check for the custom path argument or fallback to default standard locations
  AC_ARG_WITH([xml-catalog],
              AS_HELP_STRING([--with-xml-catalog=CATALOG],
                             [path to xml catalog to use @<:@default=/etc/xml/catalog@:>@]),
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

  # Locate the xmlcatalog binary companion infrastructure
  AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
  if test "x$XMLCATALOG" = "xno"; then
    jh_found_xmlcatalog=false
  fi

  # Route layout evaluation hooks based on final confirmation state
  if $jh_found_xmlcatalog; then
    m4_ifval([$1], [$1], [:])
  else
    m4_ifval([$2], [$2], [AC_MSG_ERROR([could not find valid XML catalog setup])])
  fi
])

dnl JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Checks if a specific schema, DTD, or URI namespace matches inside the catalog tracker.
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
  AC_REQUIRE([JH_PATH_XML_CATALOG])dnl
  AC_MSG_CHECKING([for m4_default([$2], [$1]) in XML catalog])
  
  if $jh_found_xmlcatalog && AC_RUN_LOG([$XMLCATALOG --noout "$XML_CATALOG_FILE" "$1" >&2]); then
    AC_MSG_RESULT([found])
    m4_ifval([$3], [$3
])dnl
  else
    AC_MSG_RESULT([not found])
    m4_ifval([$4], [$4], [AC_MSG_ERROR([could not find m4_default([$2], [$1]) in XML catalog])])
  fi
])