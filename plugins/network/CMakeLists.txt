# probe plugin
set(gammaray_network_srcs
    networksupport.cpp
    networkinterfacemodel.cpp

    cookies/cookieextension.cpp
    cookies/cookiejarmodel.cpp
)
gammaray_add_plugin(gammaray_network JSON gammaray_network.json SOURCES ${gammaray_network_srcs})
target_link_libraries(gammaray_network gammaray_core Qt5::Network)

# ui plugin
if(GAMMARAY_BUILD_UI)
  set(gammaray_network_ui_srcs
    networkwidget.cpp
    networkinterfacewidget.cpp

    cookies/cookietab.cpp
  )
  gammaray_add_plugin(gammaray_network_ui JSON gammaray_network.json SOURCES ${gammaray_network_ui_srcs})
  target_link_libraries(gammaray_network_ui gammaray_ui)
endif()
