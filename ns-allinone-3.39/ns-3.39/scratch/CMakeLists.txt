build_example(
  NAME hello-simulator
  SOURCE_FILES tutorial/hello-simulator.cc
  LIBRARIES_TO_LINK ${libcore}
)

build_example(
  NAME first
  SOURCE_FILES tutorial/first.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)

build_example(
  NAME second
  SOURCE_FILES tutorial/second.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libcsma}
    ${libinternet}
    ${libapplications}
)

build_example(
  NAME third
  SOURCE_FILES tutorial/third.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libcsma}
    ${libwifi}
    ${libinternet}
    ${libapplications}
)

build_example(
  NAME fourth
  SOURCE_FILES tutorial/fourth.cc
  LIBRARIES_TO_LINK ${libcore}
)

build_example(
  NAME fifth
  SOURCE_FILES tutorial/fifth.cc
               tutorial/tutorial-app.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)

build_example(
  NAME sixth
  SOURCE_FILES tutorial/sixth.cc
               tutorial/tutorial-app.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)

build_example(
  NAME seventh
  SOURCE_FILES tutorial/seventh.cc
               tutorial/tutorial-app.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libstats}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
)
