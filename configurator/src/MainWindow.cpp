/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2010-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QActionGroup>
#include <QDir>
#include <QProcess>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

#include "Configuration/JsonStore.h"
#include "Configuration/UiMapping.h"

#include "AboutDialog.h"
#include "DesktopTheme.h"
#include "ConfigurationPagePluginInterface.h"
#include "ConfigurationManager.h"
#include "LicensePage.h"
#include "VeyonConfiguration.h"
#include "MainWindow.h"
#include "PluginManager.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_configChanged( false )
{
	ui->setupUi( this );
	setObjectName(QStringLiteral("configuratorWindow"));
	ui->gridLayout->setContentsMargins(0, 0, 0, 12);
	ui->gridLayout->setHorizontalSpacing(0);
	ui->verticalLayout->setContentsMargins(24, 12, 24, 16);
	ui->buttonBox->setCenterButtons(false);
	ui->buttonBox->setContentsMargins(12, 0, 20, 0);
	ui->buttonBox->button(QDialogButtonBox::Apply)->setDefault(true);
	ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Terapkan"));
	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Batalkan perubahan"));
	ui->buttonBox->button(QDialogButtonBox::Apply)->setIcon(QIcon());
	ui->buttonBox->button(QDialogButtonBox::Reset)->setIcon(QIcon());
	ui->pageSelector->setIconSize(QSize(20, 20));
	ui->pageSelector->setSpacing(1);
	ui->pageSelector->setMinimumWidth(210);
	ui->pageSelector->setMaximumWidth(240);
	ui->pageSelector->setTextElideMode(Qt::ElideRight);

	setWindowTitle(tr("Khwarizmi Configurator %1").arg(VeyonCore::versionString()));

	loadConfigurationPagePlugins();
	for (auto combo : findChildren<QComboBox*>())
	{
		combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		combo->setMinimumContentsLength(12);
	}
	const QMap<QString, QString> pageTitles{
		{QStringLiteral("Authentication keys"), tr("Kunci autentikasi")},
		{QStringLiteral("Locations & computers"), tr("Lokasi dan komputer")},
		{QStringLiteral("Applications & websites"), tr("Aplikasi dan situs")},
		{QStringLiteral("File transfer"), tr("Transfer berkas")}
	};
	const QList<QColor> categoryColors{
		QColor(QStringLiteral("#63718a")), QColor(QStringLiteral("#7656bd")),
		QColor(QStringLiteral("#2878c8")), QColor(QStringLiteral("#278453")),
		QColor(QStringLiteral("#b66b16")), QColor(QStringLiteral("#26877b"))
	};
	for( int i = 0; i < ui->pageSelector->count(); ++i )
	{
		auto* item = ui->pageSelector->item(i);
		item->setText(pageTitles.value(item->text(), item->text()));
		item->setToolTip(item->text());
		auto tint = categoryColors.at(i % categoryColors.size());
		if (VeyonCore::useDarkMode())
		{
			tint = tint.lighter(155);
		}
		item->setIcon(DesktopTheme::icon(item->icon(), tint));
	}
	auto navigation = new QWidget(ui->centralwidget);
	auto navigationLayout = new QVBoxLayout(navigation);
	navigationLayout->setContentsMargins(0, 16, 0, 0);
	navigationLayout->setSpacing(12);
	auto navigationTitle = new QLabel(tr("Pengaturan"), navigation);
	navigationTitle->setObjectName(QStringLiteral("navigationTitle"));
	navigationLayout->addWidget(navigationTitle);
	auto navigationSearch = new QLineEdit(navigation);
	navigationSearch->setPlaceholderText(tr("Cari kategori..."));
	navigationSearch->setClearButtonEnabled(true);
	navigationLayout->addWidget(navigationSearch);
	ui->gridLayout->removeWidget(ui->pageSelector);
	navigationLayout->addWidget(ui->pageSelector, 1);
	ui->gridLayout->addWidget(navigation, 0, 0);
	connect(navigationSearch, &QLineEdit::textChanged, this, [this](const QString& query) {
		for (int i = 0; i < ui->pageSelector->count(); ++i)
		{
			auto item = ui->pageSelector->item(i);
			item->setHidden(!item->text().contains(query, Qt::CaseInsensitive));
		}
	});
	auto content = new QWidget(ui->centralwidget);
	auto contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(0);
	auto heading = new QLabel(ui->pageSelector->currentItem()->text(), content);
	heading->setObjectName(QStringLiteral("settingsHeading"));
	heading->setContentsMargins(24, 16, 24, 16);
	contentLayout->addWidget(heading);
	ui->gridLayout->removeWidget(ui->scrollArea);
	contentLayout->addWidget(ui->scrollArea, 1);
	ui->gridLayout->addWidget(content, 0, 1);
	connect(ui->pageSelector, &QListWidget::currentRowChanged, heading, [this, heading](int row) {
		if (const auto item = ui->pageSelector->item(row))
		{
			heading->setText(item->text());
		}
	});
	auto brand = new QLabel(tr("Khwarizmi - Pengaturan"), ui->centralwidget);
	brand->setObjectName(QStringLiteral("settingsBrand"));
	ui->gridLayout->addWidget(brand, 2, 0);

	// reset all widget's values to current configuration
	reset( true );

	// if local configuration is incomplete, re-enable the apply button
	if( VeyonConfiguration().data().size() < VeyonCore::config().data().size() )
	{
		configurationChanged();
	}

	const auto pages = findChildren<ConfigurationPage *>();
	for( auto page : pages )
	{
		page->connectWidgetsToProperties();
		connect( page, &ConfigurationPage::widgetsChanged, this, &MainWindow::updateView );
	}

	connect( ui->buttonBox, &QDialogButtonBox::clicked, this, &MainWindow::resetOrApply );

	connect( ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettingsFromFile );
	connect( ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettingsToFile );

	auto viewModeGroup = new QActionGroup( this );
	viewModeGroup->addAction( ui->viewModeStandard );
	viewModeGroup->addAction( ui->viewModeAdvanced );
	viewModeGroup->setExclusive( true );
	ui->viewModeStandard->setChecked( true );

	connect( viewModeGroup, &QActionGroup::triggered, this, &MainWindow::updateView );

	connect( ui->actionAboutQt, &QAction::triggered, QApplication::instance(), &QApplication::aboutQt );

	connect( &VeyonCore::config(), &VeyonConfiguration::configurationChanged, this, &MainWindow::configurationChanged );

	connect(ui->configPages, &QStackedWidget::currentChanged, this, [this]() {
		updateSizes();
		ui->scrollArea->verticalScrollBar()->setValue(0);
		ui->scrollArea->horizontalScrollBar()->setValue(0);
	});

	resize(1040, 740);

	updateView();
}



MainWindow::~MainWindow()
{
	delete ui;
}



void MainWindow::reset( bool onlyUI )
{
	if( onlyUI == false )
	{
		VeyonCore::config().clear();
		VeyonCore::config().reloadFromStore();
	}

	const auto pages = findChildren<ConfigurationPage *>();
	for( auto page : pages )
	{
		page->resetWidgets();
	}

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::apply()
{
	const auto showError = [this](const ConfigurationManager& configurationManager) {
		vCritical() << configurationManager.errorString().toUtf8().constData();

		QMessageBox::critical(this, tr("Khwarizmi Configurator"), configurationManager.errorString());
	};

	ConfigurationManager configurationManager;
	if (configurationManager.saveConfiguration())
	{
		if (configurationManager.applyConfiguration() == false)
		{
			showError(configurationManager);
		}

		const auto pages = findChildren<ConfigurationPage *>();
		for( auto page : pages )
		{
			page->applyConfiguration();
		}

		ui->buttonBox->setEnabled( false );
		m_configChanged = false;
	}
	else
	{
		showError(configurationManager);
	}
}



void MainWindow::resizeEvent( QResizeEvent* event )
{
	QMainWindow::resizeEvent( event );

	updateSizes();
}




void MainWindow::configurationChanged()
{
	ui->buttonBox->setEnabled( true );
	m_configChanged = true;
}




void MainWindow::resetOrApply( QAbstractButton *btn )
{
	if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Apply )
	{
		apply();
	}
	else if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Reset )
	{
		reset();
	}
}



void MainWindow::loadSettingsFromFile()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr( "Load settings from file" ),
													 QDir::homePath(), tr( "JSON files (*.json)" ) );
	if( !fileName.isEmpty() )
	{
		// write current configuration to output file
		Configuration::JsonStore(Configuration::JsonStore::Scope::System, fileName).load(&VeyonCore::config());
		reset( true );
		configurationChanged();	// give user a chance to apply possible changes
	}
}




void MainWindow::saveSettingsToFile()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save settings to file" ),
													 QDir::homePath(), tr( "JSON files (*.json)" ) );
	if( !fileName.isEmpty() )
	{
		if( !fileName.endsWith( QStringLiteral(".json"), Qt::CaseInsensitive ) )
		{
			fileName += QStringLiteral(".json");
		}

		bool configChangedPrevious = m_configChanged;

		// write current configuration to output file
		Configuration::JsonStore(Configuration::JsonStore::Scope::System, fileName).flush(&VeyonCore::config());

		m_configChanged = configChangedPrevious;
		ui->buttonBox->setEnabled( m_configChanged );
	}
}



void MainWindow::resetConfiguration()
{
	if( QMessageBox::warning( this, tr( "Reset configuration" ),
							  tr( "Do you really want to reset the local configuration and revert "
								  "all settings to their defaults?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) ==
		QMessageBox::Yes )
	{
		ConfigurationManager().clearConfiguration();
		reset( false );
	}
}



void MainWindow::aboutAlKhwarizmi()
{
	AboutDialog( this ).exec();
}



void MainWindow::updateSizes()
{
	// Hidden pages must not impose their size hint on the active page.
	ui->configPages->setMinimumSize(0, 0);
	ui->configPages->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	for (int i = 0; i < ui->configPages->count(); ++i)
	{
		ui->configPages->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	}
	ui->configPages->currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	const auto pageSize = ui->configPages->currentWidget()->minimumSizeHint();
	const auto margins = ui->verticalLayout->contentsMargins();
	const auto availableWidth = ui->scrollArea->viewport()->width() - margins.left() - margins.right();
	const auto heading = ui->scrollAreaWidgetContents->findChild<QLabel*>(QStringLiteral("settingsHeading"));
	const auto availableHeight = ui->scrollArea->viewport()->height() - margins.top() - margins.bottom() -
		(heading ? heading->sizeHint().height() : 0) - ui->verticalLayout->spacing();
	ui->configPages->setFixedSize(qMax(availableWidth, pageSize.width()), qMax(availableHeight, pageSize.height()));
}



void MainWindow::updateView()
{
	if( ui->viewModeAdvanced->isChecked() )
	{
		switchToAdvancedView();
	}
	else
	{
		switchToStandardView();
	}

	QTimer::singleShot( 0, this, &MainWindow::updateSizes );
}



void MainWindow::switchToStandardView()
{
	const auto widgets = findChildren<QWidget *>();

	// hide widgets with advanced property flag
	for( auto widget : widgets )
	{
		const auto flags = Configuration::UiMapping::flags( widget );
		if( flags & Configuration::Property::Flag::Hidden )
		{
			widget->hide();
		}
		else if( flags & Configuration::Property::Flag::Advanced )
		{
			widget->hide();
		}
	}

	// hide page selector items for advanced pages
	for( int i = 0; i < ui->pageSelector->count(); ++i )
	{
		auto item = ui->pageSelector->item( i );
		auto page = item->data( Qt::UserRole ).value<ConfigurationPage *>();
		if( page )
		{
			const auto flags = Configuration::UiMapping::flags( page );

			ui->pageSelector->setRowHidden( i, flags & Configuration::Property::Flag::Advanced );
			ui->configPages->widget( i )->setVisible( i == ui->pageSelector->currentRow() );
			if( flags & Configuration::Property::Flag::Advanced &&
				i == ui->pageSelector->currentRow() )
			{
				ui->pageSelector->setCurrentRow( 0 );
			}
		}
	}
}



void MainWindow::switchToAdvancedView()
{
	const auto widgets = findChildren<QWidget *>();
	for( auto widget : widgets )
	{
		const auto flags = Configuration::UiMapping::flags( widget );
		if( flags & Configuration::Property::Flag::Advanced )
		{
			widget->show();
		}
	}

	for( int i = 0; i < ui->pageSelector->count(); ++i )
	{
		ui->pageSelector->setRowHidden( i, false );
		ui->configPages->widget( i )->setVisible( i == ui->pageSelector->currentRow() );
	}
}



void MainWindow::loadConfigurationPagePlugins()
{
	for( auto pluginObject : std::as_const( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto configurationPagePluginInterface = qobject_cast<ConfigurationPagePluginInterface *>( pluginObject );

		if( pluginInterface && configurationPagePluginInterface )
		{
			auto page = configurationPagePluginInterface->createConfigurationPage();
			if( page )
			{
				ui->configPages->addWidget( page );

				auto item = new QListWidgetItem( page->windowIcon(), page->windowTitle() );
				item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
				item->setData( Qt::UserRole, QVariant::fromValue( page ) );
				ui->pageSelector->addItem( item );
			}
		}
	}

	// The licence page is built in, not a plugin: deleting a plugin must not
	// be a way to hide the licence state.
	auto licensePage = new LicensePage;
	ui->configPages->addWidget( licensePage );
	auto licenseItem = new QListWidgetItem( licensePage->windowIcon(), licensePage->windowTitle() );
	licenseItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
	licenseItem->setData( Qt::UserRole, QVariant::fromValue( static_cast<ConfigurationPage *>( licensePage ) ) );
	ui->pageSelector->addItem( licenseItem );

	// adjust minimum size
	ui->pageSelector->setMinimumWidth(210);
}



void MainWindow::closeEvent( QCloseEvent *closeEvent )
{
	if( m_configChanged &&
		QMessageBox::question( this, tr( "Unsaved settings" ),
							   tr( "There are unsaved settings. "
								   "Quit anyway?" ),
							   QMessageBox::Yes | QMessageBox::No ) !=
		QMessageBox::Yes )
	{
		closeEvent->ignore();
		return;
	}

	closeEvent->accept();
	QMainWindow::closeEvent( closeEvent );
}
