/*
 * VeyonCore.cpp - implementation of Veyon Core
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <veyonconfig.h>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QGroupBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLibraryInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QStyleHints>
#include <QSysInfo>
#include <QToolTip>

#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "Filesystem.h"
#include "HostAddress.h"
#include "Logger.h"
#include "NetworkObjectDirectoryManager.h"
#include "PasswordDialog.h"
#include "PlatformPluginManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformSessionFunctions.h"
#include "PluginManager.h"
#include "TranslationLoader.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "VncConnection.h"


VeyonCore* VeyonCore::s_instance = nullptr;


VeyonCore::VeyonCore( QCoreApplication* application, Component component, const QString& appComponentName ) :
	QObject( application ),
	m_filesystem( new Filesystem ),
	m_config( nullptr ),
	m_logger( nullptr ),
	m_authenticationCredentials( nullptr ),
	m_cryptoCore( nullptr ),
	m_pluginManager( nullptr ),
	m_platformPluginManager( nullptr ),
	m_platformPlugin( nullptr ),
	m_builtinFeatures( nullptr ),
	m_userGroupsBackendManager( nullptr ),
	m_networkObjectDirectoryManager( nullptr ),
	m_component( component ),
	m_debugging( false )
{
	Q_ASSERT( application != nullptr );

	s_instance = this;

	initPlatformPlugin();

	initConfiguration();

	initSession();

	initLogging( appComponentName );

	initLocaleAndTranslation();

	initUi();

	initCryptoCore();

	initAuthenticationCredentials();

	initPlugins();

	initManagers();

	initFeatures();

	initSystemInfo();

	Q_EMIT initialized(); // clazy:exclude=incorrect-emit
}



VeyonCore::~VeyonCore()
{
	vDebug();

	delete m_featureManager;
	m_featureManager = nullptr;

	delete m_builtinFeatures;
	m_builtinFeatures = nullptr;

	delete m_userGroupsBackendManager;
	m_userGroupsBackendManager = nullptr;

	delete m_authenticationCredentials;
	m_authenticationCredentials = nullptr;

	delete m_logger;
	m_logger = nullptr;

	delete m_platformPluginManager;
	m_platformPluginManager = nullptr;

	delete m_pluginManager;
	m_pluginManager = nullptr;

	delete m_config;
	m_config = nullptr;

	delete m_filesystem;
	m_filesystem = nullptr;

	delete m_cryptoCore;
	m_cryptoCore = nullptr;

	s_instance = nullptr;
}



VeyonCore* VeyonCore::instance()
{
	return s_instance;
}



QString VeyonCore::versionString()
{
	return QStringLiteral( VEYON_VERSION );
}



QString VeyonCore::pluginDir()
{
	return QStringLiteral( VEYON_PLUGIN_DIR );
}



QString VeyonCore::translationsDirectory()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral(VEYON_TRANSLATIONS_DIR);
}



QString VeyonCore::qtTranslationsDirectory()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const auto path = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
	const auto path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
	if( QDir{path}.exists() )
	{
		return path;
	}

	return translationsDirectory();
}



QString VeyonCore::executableSuffix()
{
	return QStringLiteral( VEYON_EXECUTABLE_SUFFIX ); // clazy:exclude=empty-qstringliteral
}



QString VeyonCore::sharedLibrarySuffix()
{
	return QStringLiteral( VEYON_SHARED_LIBRARY_SUFFIX );
}



QString VeyonCore::applicationsDirectory()
{
	return QStringLiteral(CMAKE_INSTALL_PREFIX "/share/applications");

}



QString VeyonCore::sessionIdEnvironmentVariable()
{
	return QStringLiteral("VEYON_SESSION_ID");
}



void VeyonCore::setupApplicationParameters()
{
	QCoreApplication::setOrganizationName( QStringLiteral( "Sahid" ) );
	QCoreApplication::setOrganizationDomain( QStringLiteral( "veyon.io" ) );
	QCoreApplication::setApplicationName( QStringLiteral( "Sahid Surveillance System" ) );

	QCoreApplication::setAttribute( Qt::AA_ShareOpenGLContexts );

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
#endif
}



bool VeyonCore::initAuthentication()
{
	switch( config().authenticationMethod() )
	{
	case AuthenticationMethod::LogonAuthentication: return initLogonAuthentication();
	case AuthenticationMethod::KeyFileAuthentication: return initKeyFileAuthentication();
	}

	return false;
}



bool VeyonCore::useDarkMode()
{
	const auto colorScheme = config().uiColorScheme();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	if (colorScheme == UiColorScheme::System &&
		QGuiApplication::styleHints() &&
		QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
	{
		return true;
	}
#endif

	if (colorScheme == UiColorScheme::Dark)
	{
		return true;
	}

	return false;
}



bool VeyonCore::isDebugging()
{
	return s_instance && s_instance->m_debugging;
}



QByteArray VeyonCore::shortenFuncinfo( const QByteArray& info )
{
	const auto funcinfo = cleanupFuncinfo( info );

	if( isDebugging() )
	{
		return funcinfo + QByteArrayLiteral( "():" );
	}

	return funcinfo.split( ':' ).first() + QByteArrayLiteral(":");
}



// taken from qtbase/src/corelib/global/qlogging.cpp

QByteArray VeyonCore::cleanupFuncinfo( QByteArray info )
{
	// Strip the function info down to the base function name
	// note that this throws away the template definitions,
	// the parameter types (overloads) and any const/volatile qualifiers.

	if (info.isEmpty())
		return info;

	int pos;

	// Skip trailing [with XXX] for templates (gcc), but make
	// sure to not affect Objective-C message names.
	pos = info.size() - 1;
	if (info.endsWith(']') && !(info.startsWith('+') || info.startsWith('-'))) {
		while (--pos) {
			if (info.at(pos) == '[')
				info.truncate(pos);
		}
	}

	// operator names with '(', ')', '<', '>' in it
	static const char operator_call[] = "operator()";
	static const char operator_lessThan[] = "operator<";
	static const char operator_greaterThan[] = "operator>";
	static const char operator_lessThanEqual[] = "operator<=";
	static const char operator_greaterThanEqual[] = "operator>=";

	// canonize operator names
	info.replace("operator ", "operator");

	// remove argument list
	Q_FOREVER {
		int parencount = 0;
		pos = info.lastIndexOf(')');
		if (pos == -1) {
			// Don't know how to parse this function name
			return info;
		}

		// find the beginning of the argument list
		--pos;
		++parencount;
		while (pos && parencount) {
			if (info.at(pos) == ')')
				++parencount;
			else if (info.at(pos) == '(')
				--parencount;
			--pos;
		}
		if (parencount != 0)
			return info;

		info.truncate(++pos);

		if (info.at(pos - 1) == ')') {
			if (info.indexOf(operator_call) == pos - (int)strlen(operator_call))
				break;

			// this function returns a pointer to a function
			// and we matched the arguments of the return type's parameter list
			// try again
			info.remove(0, info.indexOf('('));
			info.chop(1);
			continue;
		} else {
			break;
		}
	}

	// find the beginning of the function name
	int parencount = 0;
	int templatecount = 0;
	--pos;

	// make sure special characters in operator names are kept
	if (pos > -1) {
		switch (info.at(pos)) {
		case ')':
			if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1)
				pos -= 2;
			break;
		case '<':
			if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1)
				--pos;
			break;
		case '>':
			if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1)
				--pos;
			break;
		case '=': {
			int operatorLength = (int)strlen(operator_lessThanEqual);
			if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1)
				pos -= 2;
			else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1)
				pos -= 2;
			break;
		}
		default:
			break;
		}
	}

	while (pos > -1) {
		if (parencount < 0 || templatecount < 0)
			return info;

		char c = info.at(pos);
		if (c == ')')
			++parencount;
		else if (c == '(')
			--parencount;
		else if (c == '>')
			++templatecount;
		else if (c == '<')
			--templatecount;
		else if (c == ' ' && templatecount == 0 && parencount == 0)
			break;

		--pos;
	}
	info = info.mid(pos + 1);

	// remove trailing '*', '&' that are part of the return argument
	while ((info.at(0) == '*')
		   || (info.at(0) == '&'))
		info = info.mid(1);

	// we have the full function name now.
	// clean up the templates
	while ((pos = info.lastIndexOf('>')) != -1) {
		if (!info.contains('<'))
			break;

		// find the matching close
		int end = pos;
		templatecount = 1;
		--pos;
		while (pos && templatecount) {
			char c = info.at(pos);
			if (c == '>')
				++templatecount;
			else if (c == '<')
				--templatecount;
			--pos;
		}
		++pos;
		info.remove(pos, end - pos + 1);
	}

	return info;
}



QString VeyonCore::stripDomain( const QString& username )
{
	// remove the domain part of username (e.g. "EXAMPLE.COM\Teacher" -> "Teacher")
	int domainSeparator = username.indexOf( QLatin1Char('\\') );
	if( domainSeparator >= 0 )
	{
		return username.mid( domainSeparator + 1 );
	}

	return username;
}



QString VeyonCore::stringify( const QVariantMap& map )
{
	return QString::fromUtf8( QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact) );
}



QString VeyonCore::screenName(const QScreen& screen, int index)
{
	auto screenName = tr("Screen %1").arg(index);

	const auto displayDeviceName = platform().coreFunctions().queryDisplayDeviceName(screen);
	if(displayDeviceName.isEmpty() == false)
	{
		screenName.append(QStringLiteral(" – %1").arg(displayDeviceName));
	}

	return screenName;
}



bool VeyonCore::isAuthenticationKeyNameValid( const QString& authKeyName )
{
	static const QRegularExpression keyNameRX{QStringLiteral("^[\\w-]+$")};
	return keyNameRX.match( authKeyName ).hasMatch();
}



int VeyonCore::exec()
{
	Q_EMIT applicationLoaded();

	vDebug() << "Running";

	const auto result = QCoreApplication::instance()->exec();

	vDebug() << "Exit";

	Q_EMIT exited();

	return result;
}



void VeyonCore::initPlatformPlugin()
{
	// initialize plugin manager and load platform plugins first
	m_pluginManager = new PluginManager( this );
	m_pluginManager->loadPlatformPlugins();

	// initialize platform plugin manager and initialize used platform plugin
	m_platformPluginManager = new PlatformPluginManager( *m_pluginManager );
	m_platformPlugin = m_platformPluginManager->platformPlugin();
}



void VeyonCore::initSession()
{
	if( component() != Component::Service && config().multiSessionModeEnabled() )
	{
		const auto systemEnv = QProcessEnvironment::systemEnvironment();
		if( systemEnv.contains( sessionIdEnvironmentVariable() ) )
		{
			m_sessionId = systemEnv.value( sessionIdEnvironmentVariable() ).toInt();
		}
		else
		{
			const auto sessionId = platform().sessionFunctions().currentSessionId();
			if( sessionId != PlatformSessionFunctions::InvalidSessionId )
			{
				m_sessionId = sessionId;
			}
		}
	}
	else
	{
		m_sessionId = PlatformSessionFunctions::DefaultSessionId;
	}
}


void VeyonCore::initConfiguration()
{
	m_config = new VeyonConfiguration();
	m_config->upgrade();

	if( QUuid( config().installationID() ).isNull() )
	{
		config().setInstallationID(QUuid::createUuid().toString(QUuid::WithoutBraces));
	}
}



void VeyonCore::initLogging( const QString& appComponentName )
{
	const auto currentSessionId = sessionId();

	if( currentSessionId != PlatformSessionFunctions::DefaultSessionId )
	{
		m_logger = new Logger( QStringLiteral("%1-%2").arg( appComponentName ).arg( currentSessionId ) );
	}
	else
	{
		m_logger = new Logger( appComponentName );
	}

	m_debugging = ( m_logger->logLevel() >= Logger::LogLevel::Debug );

	VncConnection::initLogging( isDebugging() );
}



void VeyonCore::initLocaleAndTranslation()
{
	if( TranslationLoader::load( QStringLiteral("qtbase") ) == false )
	{
		TranslationLoader::load( QStringLiteral("qt") );
	}

	TranslationLoader::load( QStringLiteral("veyon") );

	const auto app = qobject_cast<QGuiApplication *>( QCoreApplication::instance() );
	if( app )
	{
		QGuiApplication::setLayoutDirection( QLocale{}.textDirection() );
	}
}



void VeyonCore::initUi()
{
	auto app = qobject_cast<QApplication *>(QCoreApplication::instance());
	if (app)
	{
		if (m_config->uiStyle() == UiStyle::Fusion)
		{
			app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
		}

		// Sahid modern theme: clean operational surfaces with focused brand accents.
		app->setStyleSheet(QStringLiteral(R"(
* { color:#1c2b2f; }
QMainWindow, QDialog, QWidget#centralWidget { background:#f4f7f8; }

QToolBar {
	background:#ffffff;
	border:none;
	border-bottom:1px solid #dce5e7;
	padding:3px 6px;
	spacing:3px;
}
QToolBar::separator {
	background:#d7e1e4;
	width:1px;
	margin:8px 6px;
}
QToolBar QToolButton {
	background:transparent;
	border:1px solid transparent;
	border-radius:8px;
	color:#314145;
	font-weight:600;
	padding:2px 6px;
}
QToolBar QToolButton:hover {
	background:#edf7f5;
	border-color:#c4dfda;
	color:#0f665e;
}
QToolBar QToolButton:pressed {
	background:#d8efeb;
}
QToolBar QToolButton:checked {
	background:#16796f;
	border-color:#16796f;
	color:#ffffff;
}

QStatusBar {
	background:#ffffff;
	border-top:1px solid #dce5e7;
	padding:3px 8px;
}
QStatusBar::item { border:0; }
QStatusBar QToolButton {
	background:#f6f8f9;
	border:1px solid #d6e0e2;
	border-radius:7px;
	color:#334347;
	font-weight:600;
	padding:3px 7px;
}
QStatusBar QToolButton:hover {
	background:#edf7f5;
	border-color:#9bcfc7;
	color:#0f665e;
}
QStatusBar QToolButton:checked {
	background:#fff1e8;
	border-color:#ee9d67;
	color:#9b4718;
}
QStatusBar QToolButton:disabled {
	background:#f3f5f6;
	border-color:#e1e7e8;
	color:#99a6aa;
}
QWidget#panelButtons QToolButton {
	background:transparent;
	border:1px solid transparent;
	padding:3px 8px;
}
QWidget#panelButtons QToolButton:hover {
	background:#edf7f5;
	border-color:#c4dfda;
}
QWidget#panelButtons QToolButton:checked {
	background:#16796f;
	border-color:#16796f;
	color:#ffffff;
}

QMenuBar { background:#ffffff; color:#233236; border-bottom:1px solid #dce5e7; }
QMenuBar::item { background:transparent; padding:7px 12px; }
QMenuBar::item:selected { background:#edf7f5; border-radius:6px; color:#0f665e; }
QMenu {
	background:#ffffff;
	border:1px solid #d7e1e4;
	border-radius:8px;
	padding:6px;
}
QMenu::item { padding:7px 22px; border-radius:6px; }
QMenu::item:selected { background:#16796f; color:#ffffff; }
QMenu::separator { height:1px; background:#e7edef; margin:5px 8px; }

QPushButton {
	background:#ffffff;
	border:1px solid #ccd8db;
	border-radius:7px;
	color:#1f2f33;
	padding:7px 16px;
}
QPushButton:hover { background:#fbfdfd; border-color:#1d847a; }
QPushButton:pressed { background:#eaf3f1; }
QPushButton:default {
	background:#16796f;
	border-color:#16796f;
	color:#ffffff;
	font-weight:600;
}
QPushButton:default:hover { background:#0f665e; }
QPushButton:disabled { color:#9aa7aa; background:#f1f4f5; border-color:#dfe6e8; }

QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
	background:#ffffff;
	border:1px solid #cbd8db;
	border-radius:7px;
	padding:6px 9px;
	selection-background-color:#16796f;
	selection-color:#ffffff;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus, QTextEdit:focus {
	border:1px solid #16796f;
	background:#ffffff;
}
QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled {
	background:#f1f4f5;
	color:#8f9da1;
}

QListView, QTreeView, QTableView {
	background:#f7f9fa;
	border:none;
	outline:none;
	alternate-background-color:#eef3f4;
}
QTreeView::item, QListView::item {
	border-radius:7px;
	padding:3px;
}
QTreeView::item:hover, QListView::item:hover {
	background:#e8f4f2;
}
QTreeView::item:selected, QListView::item:selected {
	background:#d7efeb;
	color:#143b37;
}

QSlider::groove:horizontal {
	background:#d9e4e6;
	border-radius:3px;
	height:6px;
}
QSlider::sub-page:horizontal {
	background:#16796f;
	border-radius:3px;
}
QSlider::handle:horizontal {
	background:#ffffff;
	border:2px solid #16796f;
	border-radius:8px;
	height:16px;
	margin:-6px 0;
	width:16px;
}
QSlider::handle:horizontal:hover {
	border-color:#e56e2d;
}

QScrollBar:vertical { background:transparent; width:11px; margin:2px; }
QScrollBar::handle:vertical { background:#becace; border-radius:5px; min-height:32px; }
QScrollBar::handle:vertical:hover { background:#16796f; }
QScrollBar:horizontal { background:transparent; height:11px; margin:2px; }
QScrollBar::handle:horizontal { background:#becace; border-radius:5px; min-width:32px; }
QScrollBar::handle:horizontal:hover { background:#16796f; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }

QSplitter::handle { background:#e3eaec; }
QSplitter::handle:hover { background:#9bcfc7; }
QDockWidget::title {
	background:#ffffff;
	border-bottom:1px solid #dce5e7;
	padding:8px;
}

QGroupBox {
	border:1px solid #dce5e7;
	border-radius:8px;
	margin-top:12px;
	padding-top:10px;
	background:#ffffff;
}
QGroupBox::title {
	color:#58686c;
	left:10px;
	padding:0 4px;
}
QTabWidget::pane { border:1px solid #dce5e7; background:#ffffff; }
QTabBar::tab { background:transparent; padding:8px 16px; color:#54646a; }
QTabBar::tab:selected { color:#16796f; border-bottom:2px solid #16796f; font-weight:600; }

QToolTip {
	background:#193438;
	border:none;
	border-radius:6px;
	color:#ffffff;
	padding:6px 9px;
}
)"));
		if( VeyonCore::useDarkMode() )
		{
			app->setStyleSheet(QStringLiteral(R"(
* { color:#e8eef0; }
QMainWindow, QDialog, QWidget#centralWidget { background:#1f272a; }
QToolBar {
	background:#273134;
	border:none;
	border-bottom:1px solid #344145;
	padding:3px 6px;
	spacing:3px;
}
QToolBar::separator { background:#445257; width:1px; margin:8px 6px; }
QToolBar QToolButton {
	background:transparent;
	border:1px solid transparent;
	border-radius:8px;
	color:#e8eef0;
	font-weight:600;
	padding:2px 6px;
}
QToolBar QToolButton:hover { background:#263f3c; border-color:#3d756e; color:#ffffff; }
QToolBar QToolButton:pressed { background:#1f5d56; }
QToolBar QToolButton:checked { background:#1d847a; border-color:#1d847a; color:#ffffff; }
QStatusBar {
	background:#273134;
	border-top:1px solid #344145;
	padding:3px 8px;
}
QStatusBar::item { border:0; }
QStatusBar QToolButton {
	background:#222b2e;
	border:1px solid #39474b;
	border-radius:7px;
	color:#e8eef0;
	font-weight:600;
	padding:3px 7px;
}
QStatusBar QToolButton:hover { background:#263f3c; border-color:#3d756e; color:#ffffff; }
QStatusBar QToolButton:checked { background:#57321f; border-color:#d97738; color:#ffffff; }
QStatusBar QToolButton:disabled { background:#22282a; border-color:#323c40; color:#7d8a8f; }
QWidget#panelButtons QToolButton {
	background:transparent;
	border:1px solid transparent;
	padding:3px 8px;
}
QWidget#panelButtons QToolButton:hover { background:#263f3c; border-color:#3d756e; }
QWidget#panelButtons QToolButton:checked { background:#1d847a; border-color:#1d847a; color:#ffffff; }
QMenuBar { background:#273134; color:#e8eef0; border-bottom:1px solid #344145; }
QMenuBar::item { background:transparent; padding:7px 12px; }
QMenuBar::item:selected { background:#263f3c; border-radius:6px; color:#ffffff; }
QMenu {
	background:#273134;
	border:1px solid #3b494d;
	border-radius:8px;
	padding:6px;
}
QMenu::item { padding:7px 22px; border-radius:6px; color:#e8eef0; }
QMenu::item:selected { background:#1d847a; color:#ffffff; }
QMenu::separator { height:1px; background:#3b494d; margin:5px 8px; }
QPushButton {
	background:#273134;
	border:1px solid #445257;
	border-radius:7px;
	color:#e8eef0;
	padding:7px 16px;
}
QPushButton:hover { background:#2d383b; border-color:#1d847a; }
QPushButton:pressed { background:#22312f; }
QPushButton:default { background:#1d847a; border-color:#1d847a; color:#ffffff; font-weight:600; }
QPushButton:disabled { color:#7d8a8f; background:#22282a; border-color:#323c40; }
QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
	background:#20282b;
	border:1px solid #445257;
	border-radius:7px;
	padding:6px 9px;
	color:#e8eef0;
	selection-background-color:#1d847a;
	selection-color:#ffffff;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus, QTextEdit:focus {
	border:1px solid #1d847a;
	background:#20282b;
}
QListView, QTreeView, QTableView {
	background:#20282b;
	border:none;
	outline:none;
	alternate-background-color:#273134;
}
QTreeView::item, QListView::item { border-radius:7px; padding:3px; }
QTreeView::item:hover, QListView::item:hover { background:#263f3c; }
QTreeView::item:selected, QListView::item:selected { background:#1d5f58; color:#ffffff; }
QSlider::groove:horizontal { background:#39474b; border-radius:3px; height:6px; }
QSlider::sub-page:horizontal { background:#1d847a; border-radius:3px; }
QSlider::handle:horizontal {
	background:#273134;
	border:2px solid #1d847a;
	border-radius:8px;
	height:16px;
	margin:-6px 0;
	width:16px;
}
QScrollBar:vertical { background:transparent; width:11px; margin:2px; }
QScrollBar::handle:vertical { background:#4b5b60; border-radius:5px; min-height:32px; }
QScrollBar::handle:vertical:hover { background:#1d847a; }
QScrollBar:horizontal { background:transparent; height:11px; margin:2px; }
QScrollBar::handle:horizontal { background:#4b5b60; border-radius:5px; min-width:32px; }
QScrollBar::handle:horizontal:hover { background:#1d847a; }
QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }
QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }
QSplitter::handle { background:#344145; }
QSplitter::handle:hover { background:#1d847a; }
QDockWidget::title { background:#273134; border-bottom:1px solid #344145; padding:8px; }
QTabWidget::pane { border:1px solid #344145; background:#20282b; }
QTabBar::tab { background:transparent; padding:8px 16px; color:#aebabe; }
QTabBar::tab:selected { color:#ffffff; border-bottom:2px solid #1d847a; font-weight:600; }
QToolTip { background:#102326; border:none; border-radius:6px; color:#ffffff; padding:6px 9px; }
)"));
		}

		// recolor selections/highlights throughout the UI with the brand teal
		auto appPalette = app->palette();
		if( VeyonCore::useDarkMode() )
		{
			appPalette.setColor(QPalette::Window, QColor(0x1f, 0x27, 0x2a));
			appPalette.setColor(QPalette::Base, QColor(0x20, 0x28, 0x2b));
			appPalette.setColor(QPalette::AlternateBase, QColor(0x27, 0x31, 0x34));
			appPalette.setColor(QPalette::Text, QColor(0xe8, 0xee, 0xf0));
			appPalette.setColor(QPalette::Button, QColor(0x27, 0x31, 0x34));
			appPalette.setColor(QPalette::ButtonText, QColor(0xe8, 0xee, 0xf0));
		}
		else
		{
			appPalette.setColor(QPalette::Window, QColor(0xf4, 0xf7, 0xf8));
			appPalette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
			appPalette.setColor(QPalette::AlternateBase, QColor(0xee, 0xf3, 0xf4));
			appPalette.setColor(QPalette::Text, QColor(0x1c, 0x2b, 0x2f));
			appPalette.setColor(QPalette::Button, QColor(0xff, 0xff, 0xff));
			appPalette.setColor(QPalette::ButtonText, QColor(0x1c, 0x2b, 0x2f));
		}
		appPalette.setColor(QPalette::Highlight, QColor(0x16, 0x79, 0x6f));
		appPalette.setColor(QPalette::HighlightedText, Qt::white);
		app->setPalette(appPalette);

		auto toolTipPalette = QToolTip::palette();
		static const char* toolTipBackgroundColor = "#193438";
		toolTipPalette.setColor(QPalette::Window, toolTipBackgroundColor);
		toolTipPalette.setColor(QPalette::ToolTipBase, toolTipBackgroundColor);
		toolTipPalette.setColor(QPalette::ToolTipText, Qt::white);
		QToolTip::setPalette(toolTipPalette);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
		switch (config().uiColorScheme())
		{
		case UiColorScheme::Dark:
			QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
			break;
		case UiColorScheme::Light:
			QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
			break;
		default:
			break;
		}
#endif
	}
}



void VeyonCore::initCryptoCore()
{
	m_cryptoCore = new CryptoCore;
}



void VeyonCore::initAuthenticationCredentials()
{
	if( m_authenticationCredentials )
	{
		delete m_authenticationCredentials;
		m_authenticationCredentials = nullptr;
	}

	m_authenticationCredentials = new AuthenticationCredentials;
}



void VeyonCore::initPlugins()
{
	// load all other (non-platform) plugins
	m_pluginManager->loadPlugins();
	m_pluginManager->upgradePlugins();
}



void VeyonCore::initManagers()
{
	m_userGroupsBackendManager = new UserGroupsBackendManager( this );
	m_networkObjectDirectoryManager = new NetworkObjectDirectoryManager( this );
}



void VeyonCore::initFeatures()
{
	m_builtinFeatures = new BuiltinFeatures();
	m_featureManager = new FeatureManager(this);
}



bool VeyonCore::initLogonAuthentication()
{
	if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
	{
		PasswordDialog dlg( QApplication::activeWindow() );
		if( dlg.exec() &&
			dlg.credentials().hasCredentials( AuthenticationCredentials::Type::UserLogon ) )
		{
			m_authenticationCredentials->setLogonUsername( dlg.username() );
			m_authenticationCredentials->setLogonPassword( dlg.password() );

			return true;
		}
	}

	return false;
}



bool VeyonCore::initKeyFileAuthentication()
{
	auto authKeyName = QProcessEnvironment::systemEnvironment().value( QStringLiteral("VEYON_AUTH_KEY_NAME") );

	if( authKeyName.isEmpty() == false )
	{
		if( isAuthenticationKeyNameValid( authKeyName ) &&
				m_authenticationCredentials->loadPrivateKey( VeyonCore::filesystem().privateKeyPath( authKeyName ) ) )
		{
			m_authenticationCredentials->setAuthenticationKeyName( authKeyName );
			return true;
		}
	}
	else
	{
		// try to auto-detect private key file by searching for readable file
		const auto privateKeyBaseDir = VeyonCore::filesystem().expandPath( VeyonCore::config().privateKeyBaseDir() );
		const auto privateKeyDirs = QDir( privateKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

		for( const auto& privateKeyDir : privateKeyDirs )
		{
			if( m_authenticationCredentials->loadPrivateKey( VeyonCore::filesystem().privateKeyPath( privateKeyDir ) ) )
			{
				m_authenticationCredentials->setAuthenticationKeyName( privateKeyDir );
				return true;
			}
		}
	}

	return false;
}



void VeyonCore::initSystemInfo()
{
	vDebug() << versionString() << HostAddress::localFQDN()
			 << QSysInfo::kernelType() << QSysInfo::kernelVersion()
			 << QSysInfo::prettyProductName() << QSysInfo::productType() << QSysInfo::productVersion();
}
