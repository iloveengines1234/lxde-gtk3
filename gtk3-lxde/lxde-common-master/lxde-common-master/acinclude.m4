dnl JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Modernized verification for the presence of the system XML Catalog layer.
AC_DEFUN([JH_PATH_XML_CATALOG],
[
  # Fixed: Replaced obsolete AC_HELP_STRING with modern AS_HELP_STRING call
  AC_ARG_WITH([xml-catalog],
    [AS_HELP_STRING([--with-xml-catalog=CATALOG], [path to xml catalog to use])],
    [],
    [with_xml_catalog=/etc/xml/catalog]
  )

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

  # Locate the xmlcatalog processing utility executable
  AC_PATH_PROG([XMLCATALOG], [xmlcatalog], [no])
  if test "x$XMLCATALOG" = xno; then
    jh_found_xmlcatalog=false
  fi

  # Handle execution paths depending on find state outcomes
  AS_IF([test "x$jh_found_xmlcatalog" = xtrue],
    [
      m4_ifval([$1], [$1], [:])
    ],
    [
      m4_ifval([$2], [$2], [AC_MSG_ERROR([could not find a functional XML catalog setup])])
    ]
  )
])

dnl JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Checks if a specific URI string footprint successfully manifests inside the active catalog maps.
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
  dnl Safely ensure parent requirements are triggered cleanly
  AC_REQUIRE([JH_PATH_XML_CATALOG])
  
  AC_MSG_CHECKING([for m4_default([$2], [$1]) in XML catalog])
  
  # Fixed: Modernized execution validation logic to keep logging channels completely clean
  if test "x$jh_found_xmlcatalog" = xtrue && $XMLCATALOG "$XML_CATALOG_FILE" "$1" >/dev/null 2>&1; then
    AC_MSG_RESULT([found])
    m4_ifval([$3], [$3])
  else
    AC_MSG_RESULT([not found])
    m4_ifval([$4], [$4], [AC_MSG_ERROR([could not locate m4_default([$2], [$1]) inside system XML catalogs])])
  fi
])