include(BuildVeyonPlugin)

build_veyon_plugin(screenrecorder
	NAME ScreenRecorder
	SOURCES
	ScreenRecorderFeaturePlugin.cpp
	ScreenRecorderSession.cpp
	ScreenRecorderFeaturePlugin.h
	ScreenRecorderSession.h
	screenrecorder.qrc
	)
