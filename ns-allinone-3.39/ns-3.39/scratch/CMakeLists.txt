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
    ${libnetanim}
)

build_example(
  NAME test-ff-aodv-energy-route-demo
  SOURCE_FILES test-ff-aodv-energy-route-demo.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libnetanim}
)

build_example(
  NAME test-ff-aodv
  SOURCE_FILES test-ff-aodv.cc
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
  NAME test-ff-aodv-v2
  SOURCE_FILES test-ff-aodv-v2.cc
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
  NAME experiment-ff-aodv
  SOURCE_FILES experiment-ff-aodv.cc
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
  NAME sim-802-11-static
  SOURCE_FILES sim-802-11-static.cc
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
  NAME sim-802-15-4-static
  SOURCE_FILES sim-802-15-4-static.cc
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
  NAME sim-fanet-bonus
  SOURCE_FILES sim-fanet-bonus.cc
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
