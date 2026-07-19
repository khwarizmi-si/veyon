include(BuildVeyonPlugin)

build_veyon_plugin(entraid
	NAME EntraId
	SOURCES
	EntraIdPlugin.cpp
	EntraIdPlugin.h
	EntraIdConfiguration.cpp
	EntraIdConfiguration.h
	EntraIdGraphClient.cpp
	EntraIdGraphClient.h
	EntraIdNetworkObjectDirectory.cpp
	EntraIdNetworkObjectDirectory.h
	EntraIdConfigurationPage.cpp
	EntraIdConfigurationPage.h
	)
