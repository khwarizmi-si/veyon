include(BuildVeyonPlugin)

build_veyon_plugin(auvidus
	NAME Auvidus
	SOURCES
	AuvidusFeaturePlugin.cpp
	DeviceController.cpp
	AuvidusFeaturePlugin.h
	DeviceController.h
	auvidus.qrc
	)

if(WIN32)
	# Core Audio (CoCreateInstance / IAudioEndpointVolume) needs the COM runtime
	target_link_libraries(auvidus PRIVATE ole32)
endif()
