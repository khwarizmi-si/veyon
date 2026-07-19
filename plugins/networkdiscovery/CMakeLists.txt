include(BuildVeyonPlugin)

build_veyon_plugin(networkdiscovery
	NAME NetworkDiscovery
	SOURCES
	NetworkDiscoveryPlugin.cpp
	NetworkDiscoveryDirectory.cpp
	NetworkScanner.cpp
	NetworkDiscoveryPlugin.h
	NetworkDiscoveryDirectory.h
	NetworkScanner.h
	)
