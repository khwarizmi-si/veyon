include(BuildVeyonPlugin)

build_veyon_plugin(chat
	NAME Chat
	SOURCES
	ChatFeaturePlugin.cpp
	ChatMasterDialog.cpp
	ChatClientWindow.cpp
	ChatFeaturePlugin.h
	ChatMasterDialog.h
	ChatClientWindow.h
	chat.qrc
	)
