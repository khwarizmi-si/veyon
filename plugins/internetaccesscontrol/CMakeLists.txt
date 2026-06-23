include(BuildVeyonPlugin)

build_veyon_plugin(internetaccesscontrol
	NAME InternetAccessControl
	SOURCES
	InternetAccessControlPlugin.cpp
	FirewallController.cpp
	InternetAccessControlPlugin.h
	FirewallController.h
	internetaccesscontrol.qrc
	)
