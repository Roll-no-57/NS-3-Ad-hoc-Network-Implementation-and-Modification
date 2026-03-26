build_example(
  NAME test-ff-aodv-p2
  SOURCE_FILES test-ff-aodv-p2.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME scratch-simulator
  SOURCE_FILES scratch-simulator.cc
  LIBRARIES_TO_LINK ${libcore}
)