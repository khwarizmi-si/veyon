/*
 * EntraIdConfigurationPage.cpp - configuration page for Microsoft Entra ID connector
 *
 * Copyright (c) 2026 Al-Khwarizmi
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

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "Configuration/Password.h"
#include "EntraIdConfiguration.h"
#include "EntraIdConfigurationPage.h"
#include "EntraIdGraphClient.h"


EntraIdConfigurationPage::EntraIdConfigurationPage( EntraIdConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	m_configuration( configuration )
{
	setWindowTitle( tr( "Microsoft Entra ID" ) );
	setWindowIcon( QIcon( QStringLiteral(":/core/network-server.png") ) );

	auto rootLayout = new QVBoxLayout( this );

	m_enabled = new QCheckBox( tr( "Enable Microsoft Entra ID connector" ), this );
	rootLayout->addWidget( m_enabled );

	auto authGroup = new QGroupBox( tr( "Microsoft Graph application" ), this );
	auto authLayout = new QFormLayout( authGroup );
	m_tenantId = new QLineEdit( authGroup );
	m_clientId = new QLineEdit( authGroup );
	m_clientSecret = new QLineEdit( authGroup );
	m_clientSecret->setEchoMode( QLineEdit::Password );
	m_authorityHost = new QLineEdit( authGroup );
	m_graphBaseUrl = new QLineEdit( authGroup );
	m_queryTimeout = new QSpinBox( authGroup );
	m_queryTimeout->setRange( 5, 300 );
	m_queryTimeout->setSuffix( tr( " s" ) );

	addTextRow( authLayout, tr( "Tenant ID" ), m_tenantId );
	addTextRow( authLayout, tr( "Client ID" ), m_clientId );
	addTextRow( authLayout, tr( "Client secret" ), m_clientSecret );
	addTextRow( authLayout, tr( "Authority host" ), m_authorityHost );
	addTextRow( authLayout, tr( "Graph base URL" ), m_graphBaseUrl );
	authLayout->addRow( tr( "Timeout" ), m_queryTimeout );
	rootLayout->addWidget( authGroup );

	auto mappingGroup = new QGroupBox( tr( "Device mapping" ), this );
	auto mappingLayout = new QFormLayout( mappingGroup );
	m_deviceDisplayNameField = new QLineEdit( mappingGroup );
	m_deviceHostAddressField = new QLineEdit( mappingGroup );
	m_deviceMacAddressField = new QLineEdit( mappingGroup );
	m_deviceLocationField = new QLineEdit( mappingGroup );
	m_fallbackLocationName = new QLineEdit( mappingGroup );

	addTextRow( mappingLayout, tr( "Display name field" ), m_deviceDisplayNameField );
	addTextRow( mappingLayout, tr( "Host address field" ), m_deviceHostAddressField );
	addTextRow( mappingLayout, tr( "MAC address field" ), m_deviceMacAddressField );
	addTextRow( mappingLayout, tr( "Location field" ), m_deviceLocationField );
	addTextRow( mappingLayout, tr( "Fallback location" ), m_fallbackLocationName );
	rootLayout->addWidget( mappingGroup );

	m_useSecurityGroupsForAccessControl = new QCheckBox( tr( "Use Entra security groups for access control" ), this );
	rootLayout->addWidget( m_useSecurityGroupsForAccessControl );

	auto infoLabel = new QLabel( tr( "Required Microsoft Graph application permissions: Device.Read.All, Group.Read.All, GroupMember.Read.All and User.Read.All. Grant admin consent in Microsoft Entra admin center." ), this );
	infoLabel->setWordWrap( true );
	rootLayout->addWidget( infoLabel );

	auto testButton = new QPushButton( tr( "Test connection" ), this );
	connect( testButton, &QPushButton::clicked, this, &EntraIdConfigurationPage::testConnection );
	rootLayout->addWidget( testButton, 0, Qt::AlignLeft );
	rootLayout->addStretch();
}



void EntraIdConfigurationPage::resetWidgets()
{
	m_enabled->setChecked( m_configuration.enabled() );
	m_tenantId->setText( m_configuration.tenantId() );
	m_clientId->setText( m_configuration.clientId() );
	m_clientSecret->setText( QString::fromUtf8( m_configuration.clientSecret().plainText().toByteArray() ) );
	m_authorityHost->setText( m_configuration.authorityHost() );
	m_graphBaseUrl->setText( m_configuration.graphBaseUrl() );
	m_queryTimeout->setValue( m_configuration.queryTimeout() );
	m_deviceDisplayNameField->setText( m_configuration.deviceDisplayNameField() );
	m_deviceHostAddressField->setText( m_configuration.deviceHostAddressField() );
	m_deviceMacAddressField->setText( m_configuration.deviceMacAddressField() );
	m_deviceLocationField->setText( m_configuration.deviceLocationField() );
	m_fallbackLocationName->setText( m_configuration.fallbackLocationName() );
	m_useSecurityGroupsForAccessControl->setChecked( m_configuration.useSecurityGroupsForAccessControl() );
}



void EntraIdConfigurationPage::connectWidgetsToProperties()
{
	connect( m_enabled, &QCheckBox::toggled, this, [this]( bool value ) {
		m_configuration.setEnabled( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_tenantId, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setTenantId( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_clientId, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setClientId( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_clientSecret, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setClientSecret( Configuration::Password::fromPlainText( value.toUtf8() ) );
		Q_EMIT widgetsChanged();
	} );
	connect( m_authorityHost, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setAuthorityHost( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_graphBaseUrl, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setGraphBaseUrl( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_queryTimeout, QOverload<int>::of( &QSpinBox::valueChanged ), this, [this]( int value ) {
		m_configuration.setQueryTimeout( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_deviceDisplayNameField, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setDeviceDisplayNameField( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_deviceHostAddressField, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setDeviceHostAddressField( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_deviceMacAddressField, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setDeviceMacAddressField( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_deviceLocationField, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setDeviceLocationField( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_fallbackLocationName, &QLineEdit::textChanged, this, [this]( const QString& value ) {
		m_configuration.setFallbackLocationName( value );
		Q_EMIT widgetsChanged();
	} );
	connect( m_useSecurityGroupsForAccessControl, &QCheckBox::toggled, this, [this]( bool value ) {
		m_configuration.setUseSecurityGroupsForAccessControl( value );
		Q_EMIT widgetsChanged();
	} );
}



void EntraIdConfigurationPage::applyConfiguration()
{
}



void EntraIdConfigurationPage::testConnection()
{
	EntraIdGraphClient client( m_configuration, this );
	QString errorString;
	const auto devices = client.devices( &errorString );
	if( errorString.isEmpty() )
	{
		QMessageBox::information( this,
								  tr( "Microsoft Entra ID" ),
								  tr( "Connection successful. %1 devices returned by Microsoft Graph." ).arg( devices.size() ) );
	}
	else
	{
		QMessageBox::critical( this, tr( "Microsoft Entra ID" ), errorString );
	}
}



void EntraIdConfigurationPage::addTextRow( QFormLayout* formLayout, const QString& label, QLineEdit* lineEdit )
{
	lineEdit->setClearButtonEnabled( true );
	formLayout->addRow( label, lineEdit );
}
