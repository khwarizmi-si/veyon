/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QCloseEvent>
#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHostAddress>
#include <QMenu>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include "AboutDialog.h"
#include "DesktopTheme.h"
#include "AccessControlProvider.h"
#include "MainWindow.h"
#include "BuiltinFeatures.h"
#include "AuthenticationCredentials.h"
#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"
#include "ComputerManager.h"
#include "ComputerSelectPanel.h"
#include "ScreenshotManagementPanel.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "LicenseService.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformUserFunctions.h"
#include "SlideshowPanel.h"
#include "SpotlightPanel.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"
#include "UserConfig.h"

#include "ui_MainWindow.h"

namespace
{

QString localizedFeatureLabel( const Feature& feature )
{
	const auto featureName = feature.name();
	const auto displayName = feature.displayName();

	if( featureName == QStringLiteral("MonitoringMode") )
	{
		return QStringLiteral("Pantau");
	}
	if( featureName == QStringLiteral("Demo") || displayName == QStringLiteral("Demo") )
	{
		return QStringLiteral("Peraga");
	}
	if( featureName == QStringLiteral("LockScreen") || displayName == QStringLiteral("Lock") )
	{
		return QStringLiteral("Kunci");
	}
	if( featureName == QStringLiteral("InternetAccessControl") )
	{
		return QStringLiteral("Blokir net");
	}
	if( featureName == QStringLiteral("ScreenRecorder") )
	{
		return QStringLiteral("Rekam layar");
	}
	if( featureName == QStringLiteral("RemoteView") )
	{
		return QStringLiteral("Lihat layar");
	}
	if( featureName == QStringLiteral("RemoteControl") )
	{
		return QStringLiteral("Kendali");
	}
	if( featureName == QStringLiteral("Chat") || displayName == QStringLiteral("Chat") )
	{
		return QStringLiteral("Chat");
	}
	if( featureName == QStringLiteral("PowerOn") )
	{
		return QStringLiteral("Nyalakan");
	}
	if( featureName == QStringLiteral("Reboot") || displayName == QStringLiteral("Restart") )
	{
		return QStringLiteral("Restart");
	}
	if( featureName == QStringLiteral("PowerDown") )
	{
		return QStringLiteral("Matikan");
	}
	if( featureName == QStringLiteral("UserLogin") )
	{
		return QStringLiteral("Masuk");
	}
	if( featureName == QStringLiteral("UserLogoff") )
	{
		return QStringLiteral("Keluar");
	}
	if( featureName == QStringLiteral("TextMessage") )
	{
		return QStringLiteral("Pesan");
	}
	if( featureName == QStringLiteral("StartApp") )
	{
		return QStringLiteral("Buka app");
	}
	if( featureName == QStringLiteral("OpenWebsite") )
	{
		return QStringLiteral("Buka web");
	}
	if( featureName == QStringLiteral("DistributeFiles") )
	{
		return QStringLiteral("Kirim file");
	}
	if( featureName == QStringLiteral("FileCollect") )
	{
		return QStringLiteral("Ambil file");
	}
	if( featureName == QStringLiteral("Screenshot") || displayName == QStringLiteral("Screenshot") )
	{
		return QStringLiteral("Tangkapan");
	}
	if( featureName == QStringLiteral("MuteAudio") || displayName == QStringLiteral("Mute audio") )
	{
		return QStringLiteral("Bisukan audio");
	}
	if( featureName == QStringLiteral("BlockUsbStorage") || displayName == QStringLiteral("Block USB storage") )
	{
		return QStringLiteral("Blokir USB");
	}
	if( featureName == QStringLiteral("DisableWebcam") || displayName == QStringLiteral("Disable webcam") )
	{
		return QStringLiteral("Matikan webcam");
	}

	return displayName;
}

QString localizedFeatureActiveLabel( const Feature& feature )
{
	if( feature.displayNameActive().isEmpty() )
	{
		return {};
	}
	const auto featureName = feature.name();
	const auto activeName = feature.displayNameActive();

	if( featureName == QStringLiteral("Demo") )
	{
		return QStringLiteral("Stop peraga");
	}
	if( featureName == QStringLiteral("ScreenLock") )
	{
		return QStringLiteral("Buka kunci");
	}
	if( featureName == QStringLiteral("ScreenRecorder") )
	{
		return QStringLiteral("Stop rekam");
	}
	if( featureName == QStringLiteral("MuteAudio") || activeName == QStringLiteral("Unmute audio") )
	{
		return QStringLiteral("Aktifkan audio");
	}
	if( featureName == QStringLiteral("BlockUsbStorage") || activeName == QStringLiteral("Allow USB storage") )
	{
		return QStringLiteral("Izinkan USB");
	}
	if( featureName == QStringLiteral("DisableWebcam") || activeName == QStringLiteral("Enable webcam") )
	{
		return QStringLiteral("Aktifkan webcam");
	}

	return localizedFeatureLabel( feature );
}

QString modernFeatureIconUrl( const Feature& feature )
{
	if( feature.name() == QStringLiteral("MonitoringMode") )
	{
		return QStringLiteral(":/core/ui-icons/monitor.png");
	}
	if( feature.name() == QStringLiteral("ScreenRecorder") )
	{
		return QStringLiteral(":/core/media-record.png");
	}
	if( feature.name() == QStringLiteral("RemoteView") )
	{
		return QStringLiteral(":/core/ui-icons/scan.png");
	}
	if( feature.name() == QStringLiteral("RemoteControl") )
	{
		return QStringLiteral(":/core/ui-icons/mouse-pointer-2.png");
	}
	if( feature.name() == QStringLiteral("TextMessage") )
	{
		return QStringLiteral(":/core/dialog-ok-apply.png");
	}
	if( feature.name() == QStringLiteral("StartApp") )
	{
		return QStringLiteral(":/core/media-playback-start.png");
	}
	if( feature.name() == QStringLiteral("Screenshot") )
	{
		return QStringLiteral(":/core/ui-icons/camera.png");
	}
	if( feature.name() == QStringLiteral("Chat") )
	{
		return QStringLiteral(":/core/ui-icons/messages-square.png");
	}

	return feature.iconUrl();
}

}


MainWindow::MainWindow( VeyonMaster &masterCore, QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_master( masterCore ),
	m_modeGroup( new QButtonGroup( this ) )
{
	ui->setupUi( this );
	setObjectName(QStringLiteral("masterWindow"));
	m_licenseBanner = new QLabel(ui->centralWidget);
	m_licenseBanner->setWordWrap(true);
	m_licenseBanner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	m_licenseBanner->setContentsMargins(12, 8, 12, 8);
	m_licenseBanner->setStyleSheet(QStringLiteral("background: #fff4d6; color: #35312a; border-bottom: 1px solid #e3cf91;"));
	ui->centralLayout->insertWidget(0, m_licenseBanner);
	connect(&m_master, &VeyonMaster::licenseStateChanged, this, &MainWindow::updateLicenseBanner);
	updateLicenseBanner(m_master.licenseState());
	connect(&m_master.computerControlListModel(), &ComputerControlListModel::modelAboutToBeReset,
			this, [this]() { m_connectedComputers.clear(); });
	connect(&m_master.computerControlListModel(), &ComputerControlListModel::stateChanged,
			this, [this](const QModelIndex& index) {
				const auto controlInterface = m_master.computerControlListModel().computerControlInterface(index);
				if (controlInterface.isNull())
				{
					return;
				}
				auto* computer = controlInterface.data();
				if (controlInterface->state() == ComputerControlInterface::State::Connected)
				{
					m_connectedComputers.insert(computer);
				}
				else if (m_connectedComputers.remove(computer))
				{
					ui->statusBar->showMessage(tr("Komputer %1 terputus").arg(controlInterface->computerName()), 15000);
				}
			});

	restoreState( QByteArray::fromBase64( m_master.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_master.userConfig().windowGeometry().toUtf8() ) );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->panelButtons );
	ui->statusBar->addWidget( ui->spacerLabel1 );
	ui->filterPoweredOnComputersButton->hide();
	auto* connectionFilter = new QComboBox(ui->statusBar);
	connectionFilter->addItems({tr("Semua PC"), tr("Aktif"), tr("Tidak aktif")});
	connectionFilter->setToolTip(tr("Filter berdasarkan koneksi komputer"));
	auto workspaceHeader = new QWidget(ui->centralWidget);
	workspaceHeader->setObjectName(QStringLiteral("workspaceHeader"));
	workspaceHeader->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	auto headerLayout = new QHBoxLayout(workspaceHeader);
	headerLayout->setContentsMargins(20, 16, 20, 16);
	headerLayout->setSpacing(12);
	auto workspaceTitle = new QLabel(tr("Ruang kelas"), workspaceHeader);
	workspaceTitle->setObjectName(QStringLiteral("workspaceTitle"));
	headerLayout->addWidget(workspaceTitle);
	headerLayout->addStretch();
	ui->filterLineEdit->setClearButtonEnabled(true);
	ui->filterLineEdit->setMinimumWidth(160);
	ui->filterLineEdit->setMaximumWidth(320);
	headerLayout->addWidget(ui->filterLineEdit, 1);
	headerLayout->addWidget(connectionFilter);
	ui->centralLayout->insertWidget(1, workspaceHeader);
	auto targetBar = new QWidget(ui->centralWidget);
	targetBar->setObjectName(QStringLiteral("targetBar"));
	targetBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	auto targetLayout = new QHBoxLayout(targetBar);
	targetLayout->setContentsMargins(20, 8, 20, 8);
	auto targetSummary = new QLabel(targetBar);
	targetSummary->setWordWrap(true);
	targetLayout->addWidget(targetSummary, 1);
	auto selectedActions = new QToolButton(targetBar);
	selectedActions->setText(tr("Tindakan pilihan"));
	selectedActions->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	selectedActions->setIcon(DesktopTheme::icon(QIcon(QStringLiteral(":/core/ui-icons/ellipsis.png"))));
	selectedActions->setToolTip(tr("Buka tindakan untuk komputer yang dipilih"));
	targetLayout->addWidget(selectedActions);
	connect(selectedActions, &QToolButton::clicked, this, [this, selectedActions]() {
		ui->computerMonitoringWidget->showContextMenu(selectedActions->mapToGlobal(QPoint(0, selectedActions->height())));
	});
	auto updateTargets = [this, targetSummary, selectedActions]() {
		const auto model = ui->computerMonitoringWidget->model();
		const auto visible = model->rowCount();
		int connected = 0;
		for (int row = 0; row < visible; ++row)
		{
			const auto computer = model->data(model->index(row, 0), ComputerControlListModel::ControlInterfaceRole)
				.value<ComputerControlInterface::Pointer>();
			if (computer && computer->state() == ComputerControlInterface::State::Connected)
			{
				++connected;
			}
		}
		const auto selected = ui->computerMonitoringWidget->selectionModel()->selectedIndexes().size();
		targetSummary->setText(tr("%1 tampil  |  %2 aktif  |  %3 tidak terhubung  |  %4 dipilih")
			.arg(visible).arg(connected).arg(visible - connected).arg(selected));
		selectedActions->setEnabled(selected > 0);
		for (const auto& feature : m_master.features())
		{
			if (feature.testFlag(Feature::Flag::Mode))
			{
				if (auto button = findChild<ToolButton*>(feature.name()))
				{
					button->setEnabled(connected > 0 || feature.name() == QStringLiteral("MonitoringMode"));
				}
			}
		}
	};
	connect(ui->computerMonitoringWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, updateTargets);
	connect(ui->computerMonitoringWidget->model(), &QAbstractItemModel::modelReset, this, updateTargets);
	connect(ui->computerMonitoringWidget->model(), &QAbstractItemModel::rowsInserted, this, updateTargets);
	connect(ui->computerMonitoringWidget->model(), &QAbstractItemModel::rowsRemoved, this, updateTargets);
	connect(ui->computerMonitoringWidget->model(), &QAbstractItemModel::layoutChanged, this, updateTargets);
	connect(&m_master.computerControlListModel(), &ComputerControlListModel::stateChanged, this, updateTargets);
	updateTargets();
	ui->centralLayout->insertWidget(2, targetBar);
	ui->statusBar->addWidget( ui->filterComputersWithLoggedOnUsersButton );
	ui->statusBar->addWidget( ui->spacerLabel2, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoAdjustComputerIconSizeButton );
	ui->statusBar->addWidget( ui->spacerLabel3 );
	ui->statusBar->addWidget( ui->useCustomComputerPositionsButton );
	ui->statusBar->addWidget( ui->alignComputersButton );
	ui->statusBar->addWidget( ui->spacerLabel4 );
	ui->statusBar->addWidget( ui->aboutButton );
	ui->statusBar->setSizeGripEnabled( false );
	ui->computerSelectPanelButton->setIcon(QIcon(QStringLiteral(":/core/ui-icons/monitor.png")));
	ui->screenshotManagementPanelButton->setIcon(QIcon(QStringLiteral(":/core/ui-icons/camera.png")));
	ui->slideshowPanelButton->setIcon(QIcon(QStringLiteral(":/core/ui-icons/presentation.png")));
	ui->spotlightPanelButton->setIcon(QIcon(QStringLiteral(":/core/ui-icons/focus.png")));
	for (auto* button : ui->statusBar->findChildren<QToolButton*>())
	{
		button->setIcon(DesktopTheme::icon(button->icon()));
	}
	ui->filterLineEdit->setMinimumWidth( 160 );
	ui->gridSizeSlider->setMinimumWidth( 100 );
	ui->panelButtons->layout()->setSpacing( 4 );
	ui->computerSelectPanelButton->setText( tr( "Lokasi dan komputer" ) );
	ui->screenshotManagementPanelButton->setText( tr( "Tangkapan" ) );
	ui->slideshowPanelButton->setText( tr( "Tayangan" ) );
	ui->spotlightPanelButton->setText( tr( "Sorotan" ) );
	for (auto* button : { ui->computerSelectPanelButton, ui->screenshotManagementPanelButton,
						  ui->slideshowPanelButton, ui->spotlightPanelButton })
	{
		button->setToolTip(button->text());
	}
	ui->filterLineEdit->setPlaceholderText( tr( "Cari pengguna dan komputer" ) );

	// create all views
	auto mainSplitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	mainSplitter->setChildrenCollapsible( false );
	mainSplitter->setObjectName( QStringLiteral("MainSplitter") );

	auto monitoringSplitter = new QSplitter( Qt::Vertical, mainSplitter );
	monitoringSplitter->setChildrenCollapsible( false );
	monitoringSplitter->setObjectName( QStringLiteral("MonitoringSplitter") );

	auto slideshowSpotlightSplitter = new QSplitter( Qt::Horizontal, monitoringSplitter );
	slideshowSpotlightSplitter->setChildrenCollapsible( false );
	slideshowSpotlightSplitter->setObjectName( QStringLiteral("SlideshowSpotlightSplitter") );

	auto computerSelectPanel = new ComputerSelectPanel( m_master.computerManager() );
	auto screenshotManagementPanel = new ScreenshotManagementPanel();
	auto slideshowPanel = new SlideshowPanel( m_master.userConfig(), ui->computerMonitoringWidget );
	auto spotlightPanel = new SpotlightPanel( m_master.userConfig(), ui->computerMonitoringWidget );

	slideshowSpotlightSplitter->addWidget( slideshowPanel );
	slideshowSpotlightSplitter->addWidget( spotlightPanel );
	slideshowSpotlightSplitter->setStretchFactor( slideshowSpotlightSplitter->indexOf(slideshowPanel), 1 );
	slideshowSpotlightSplitter->setStretchFactor( slideshowSpotlightSplitter->indexOf(spotlightPanel), 1 );

	monitoringSplitter->addWidget( slideshowSpotlightSplitter );
	monitoringSplitter->addWidget( ui->computerMonitoringWidget );
	monitoringSplitter->setStretchFactor( monitoringSplitter->indexOf(slideshowSpotlightSplitter), 1 );
	monitoringSplitter->setStretchFactor( monitoringSplitter->indexOf(ui->computerMonitoringWidget), 1 );

	mainSplitter->addWidget( computerSelectPanel );
	mainSplitter->addWidget( screenshotManagementPanel );
	mainSplitter->addWidget( monitoringSplitter );

	mainSplitter->setStretchFactor( mainSplitter->indexOf(monitoringSplitter), 1 );


	static const QHash<QWidget *, QAbstractButton *> panelButtons{
		{ computerSelectPanel, ui->computerSelectPanelButton },
		{ screenshotManagementPanel, ui->screenshotManagementPanelButton },
		{ slideshowPanel, ui->slideshowPanelButton },
		{ spotlightPanel, ui->spotlightPanelButton }
	};

	for( auto it = panelButtons.constBegin(), end = panelButtons.constEnd(); it != end; ++it )
	{
		it.key()->hide();
		it.key()->installEventFilter( this );
		connect( *it, &QAbstractButton::toggled, it.key(), &QWidget::setVisible );
	}

	QList<int> splitterSizes;
	for( auto* splitter : { slideshowSpotlightSplitter, monitoringSplitter, mainSplitter } )
	{
		splitter->setHandleWidth( 1 );

		splitter->installEventFilter( this );

		int index = 0;

		const auto splitterStates = m_master.userConfig().splitterStates()[splitter->objectName()].toArray();
		splitterSizes.clear();
		splitterSizes.reserve( splitterStates.size() );

		for( const auto& sizeObject : splitterStates )
		{
			auto size = sizeObject.toInt();
			const auto widget = splitter->widget( index );
			const auto button = panelButtons.value( widget );

			if( widget )
			{
				if( button )
				{
					widget->setVisible( size > 0 );
					button->setChecked( size > 0 );
				}
				size = qAbs( size );
				if( splitter->orientation() == Qt::Horizontal )
				{
					widget->resize( size, widget->height() );
				}
				else
				{
					widget->resize( widget->width(), size );
				}

				widget->setProperty( originalSizePropertyName(), widget->size() );
			}
			splitterSizes.append( size );
			++index;
		}
		splitter->setSizes( splitterSizes );
	}

	const auto SplitterContentBaseSize = 500;

	if( spotlightPanel->property( originalSizePropertyName() ).isNull() ||
		slideshowPanel->property( originalSizePropertyName() ).isNull() )
	{
		slideshowSpotlightSplitter->setSizes( { SplitterContentBaseSize, SplitterContentBaseSize } );
	}

	if( slideshowSpotlightSplitter->property( originalSizePropertyName() ).isNull() ||
		ui->computerMonitoringWidget->property( originalSizePropertyName() ).isNull() )
	{
		monitoringSplitter->setSizes( { SplitterContentBaseSize, SplitterContentBaseSize } );
	}

	ui->centralLayout->addWidget( mainSplitter );

	if( VeyonCore::config().autoOpenComputerSelectPanel() ||
		!m_master.userConfig().splitterStates().contains(mainSplitter->objectName()) )
	{
		ui->computerSelectPanelButton->setChecked( true );
	}

	// initialize filter controls
	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 this, [this]( const QString& filter ) { ui->computerMonitoringWidget->setSearchFilter( filter ); } );
	connect(connectionFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](int index) {
				auto* model = m_master.computerMonitoringModel();
				model->setStateFilter(index == 1 ? ComputerControlInterface::State::Connected
											   : ComputerControlInterface::State::None);
				model->setHideConnectedComputers(index == 2);
				if(index == 2)
				{
					ui->filterComputersWithLoggedOnUsersButton->setChecked(false);
				}
			});
	connect( ui->filterComputersWithLoggedOnUsersButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) { ui->computerMonitoringWidget->setFilterComputersWithLoggedOnUsers( enabled ); } );
	connectionFilter->setCurrentIndex(m_master.userConfig().filterPoweredOnComputers() ? 1 : 0);
	ui->filterComputersWithLoggedOnUsersButton->setChecked(m_master.userConfig().filterComputersWithLoggedOnUsers());

	// initialize monitoring screen size slider
	ui->gridSizeSlider->setMinimum( ComputerMonitoringWidget::MinimumComputerScreenSize );
	ui->gridSizeSlider->setMaximum( ComputerMonitoringWidget::MaximumComputerScreenSize );

	ui->autoAdjustComputerIconSizeButton->setChecked( ui->computerMonitoringWidget->autoAdjustIconSize() );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 this, [this]( int size ) { ui->computerMonitoringWidget->setComputerScreenSize( size ); } );
	connect( ui->computerMonitoringWidget, &ComputerMonitoringWidget::computerScreenSizeAdjusted,
			 ui->gridSizeSlider, &QSlider::setValue );
	connect( ui->autoAdjustComputerIconSizeButton, &QToolButton::toggled,
			 this, [this]( bool enabled ) {
				 ui->computerMonitoringWidget->setAutoAdjustIconSize( enabled );
				 m_master.userConfig().setAutoAdjustMonitoringIconSize( enabled );
			 } );

	int size = ComputerMonitoringWidget::DefaultComputerScreenSize;
	if( m_master.userConfig().monitoringScreenSize() >= ComputerMonitoringWidget::MinimumComputerScreenSize )
	{
		size = m_master.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );
	ui->computerMonitoringWidget->setComputerScreenSize( size );

	// initialize computer placement controls
	auto customComputerPositionsControlMenu = new QMenu;
	const auto darkSuffix = VeyonCore::useDarkMode() ? QStringLiteral("-dark") : QString();
	customComputerPositionsControlMenu->addAction(QIcon(QStringLiteral(":/core/document-open%1.png").arg(darkSuffix)),
													tr("Muat posisi komputer"),
													this, &MainWindow::loadComputerPositions);
	customComputerPositionsControlMenu->addAction(QIcon(QStringLiteral(":/core/document-save%1.png").arg(darkSuffix)),
													tr("Simpan posisi komputer"),
													this, &MainWindow::saveComputerPositions);
	ui->useCustomComputerPositionsButton->setMenu(customComputerPositionsControlMenu);

	ui->useCustomComputerPositionsButton->setChecked(m_master.userConfig().useCustomComputerPositions());
	connect(ui->useCustomComputerPositionsButton, &QToolButton::toggled,
			ui->computerMonitoringWidget, &ComputerMonitoringWidget::setUseCustomComputerPositions);
	connect(ui->alignComputersButton, &QToolButton::clicked,
			ui->computerMonitoringWidget, &ComputerMonitoringWidget::alignComputers);


	const auto toolButtons = findChildren<QToolButton*>();
	for(auto* btn : toolButtons)
	{
		btn->setIconSize(QSize(18, 18));
		btn->setMinimumHeight( 30 );
	}

	if (VeyonCore::useDarkMode())
	{
		ui->aboutButton->setIcon(QIcon(QStringLiteral(":/core/help-about-dark.png")));
		ui->filterComputersWithLoggedOnUsersButton->setIcon(QIcon(QStringLiteral(":/core/user-group-new-dark.png")));
		ui->autoAdjustComputerIconSizeButton->setIcon(QIcon(QStringLiteral(":/master/zoom-fit-best-dark.png")));
		ui->alignComputersButton->setIcon(QIcon(QStringLiteral(":/master/align-grid-dark.png")));
		ui->useCustomComputerPositionsButton->setIcon(QIcon(QStringLiteral(":/master/exchange-positions-zorder-dark.png")));
		ui->filterPoweredOnComputersButton->setIcon(QIcon(QStringLiteral(":/master/powered-on-dark.png")));
	}

	// create the main toolbar
	ui->toolBar->setMovable( false );
	ui->toolBar->setFloatable( false );
	ui->toolBar->layout()->setSpacing( 6 );
	ui->toolBar->toggleViewAction()->setEnabled( false );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	addFeaturesToToolBar();
	updateTargets();
	reloadSubFeatures();
	updateToolButtonStyles();

	m_modeGroup->button(int(qHash(VeyonCore::builtinFeatures().monitoringMode().feature().uid())))->setChecked(true); // clazy:exclude=qt6-qhash-signature
}



MainWindow::~MainWindow()
{
	ui->computerMonitoringWidget->saveConfiguration();

	delete ui;
}



bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() )
	{
		return true;
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		QMessageBox::information(nullptr,
								 tr("Autentikasi tidak tersedia"),
								 tr("Berkas kunci autentikasi tidak ditemukan atau sudah usang. "
									"Buat kunci baru di Khwarizmi Configurator, atau atur "
									"autentikasi masuk di sana. Tanpa autentikasi, komputer "
									"tidak dapat diakses."));
	}

	return false;
}



bool MainWindow::initAccessControl()
{
	if (VeyonCore::config().accessControlForMasterEnabled())
	{
		const auto accessingUser = VeyonCore::authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::UserLogon) ?
									   VeyonCore::authenticationCredentials().logonUsername() :
									   VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
		const auto accessControlResult = VeyonCore::builtinFeatures().accessControlProvider()
										 .checkAccess(accessingUser,
													  QHostAddress(QHostAddress::LocalHost).toString(),
													  {});
		if( accessControlResult.access == AccessControlProvider::Access::Deny )
		{
			vWarning() << "user" << VeyonCore::authenticationCredentials().logonUsername()
					   << "is not allowed to access computers";
			QMessageBox::critical( nullptr, tr( "Access denied" ),
								   tr( "According to the local configuration you're not allowed "
									   "to access computers in the network. Please log in with a different "
									   "account or let your system administrator check the local configuration." ) );
			return false;
		}
	}

	return true;
}



void MainWindow::reloadSubFeatures()
{
	for( const auto& feature : m_master.features() )
	{
		auto button = findChild<QToolButton *>( feature.name() );
		if( button )
		{
			addSubFeaturesToToolButton( button, feature );
		}
	}
}



ComputerControlInterfaceList MainWindow::selectedComputerControlInterfaces() const
{
	return ui->computerMonitoringWidget->selectedComputerControlInterfaces();
}



void MainWindow::resizeEvent( QResizeEvent* event )
{
	QMainWindow::resizeEvent(event);
	updateToolButtonStyles();
}



void MainWindow::updateToolButtonStyles()
{
	const auto compactPanels = width() < 1250;
	const auto compactToolbar = width() < 900;
	for (auto* button : { ui->computerSelectPanelButton, ui->screenshotManagementPanelButton,
						  ui->slideshowPanelButton, ui->spotlightPanelButton })
	{
		button->setToolButtonStyle(compactPanels || m_master.userConfig().toolButtonIconOnlyMode() ?
			Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	}
	for (auto* button : findChildren<ToolButton*>())
	{
		button->setToolButtonStyle(compactToolbar || m_master.userConfig().toolButtonIconOnlyMode() ?
			Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	}
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	if( m_master.currentMode() != VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
	{
		const Feature& activeFeature = VeyonCore::featureManager().feature( m_master.currentMode() );

		QMessageBox::information(this, tr("Fitur masih aktif"),
								 tr("Fitur \"%1\" masih aktif. Hentikan sebelum menutup Khwarizmi.")
								 .arg(activeFeature.displayName()));
		event->ignore();
		return;
	}

	QJsonObject splitterStates;
	const auto splitters = findChildren<QSplitter *>();
	for( const auto* splitter : splitters )
	{
		const auto sizes = splitter->sizes();
		QJsonArray splitterSizes;
		int i = 0;
		int hiddenSize = 0;
		for( auto size : sizes )
		{
			auto widget = splitter->widget(i);
			const auto originalSize = widget->property( originalSizePropertyName() ).toSize();
			if( widget->size().isEmpty() && originalSize.isEmpty() == false )
			{
				size = splitter->orientation() == Qt::Horizontal ? -originalSize.width() : -originalSize.height();
				hiddenSize += qAbs(size);
			}
			else
			{
				size -= hiddenSize;
			}

			splitterSizes.append( size );
			++i;
		}
		splitterStates[splitter->objectName()] = splitterSizes;
	}

	m_master.userConfig().setSplitterStates( splitterStates );

	m_master.userConfig().setWindowState( QString::fromLatin1( saveState().toBase64() ) );
	m_master.userConfig().setWindowGeometry( QString::fromLatin1( saveGeometry().toBase64() ) );

	QMainWindow::closeEvent( event );
}



bool MainWindow::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::Resize )
	{
		const auto widget = qobject_cast<QWidget *>( object );
		const auto resizeEvent = static_cast<QResizeEvent *>( event );

		if( resizeEvent->oldSize().isEmpty() == false )
		{
			widget->setProperty( originalSizePropertyName(), resizeEvent->oldSize() );
		}
	}

	return QMainWindow::eventFilter( object, event );
}



void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() )
	{
	case Qt::Key_F5:
		VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();
		m_master.computerControlListModel().reload();
		event->accept();
		break;
	case Qt::Key_F11:
		QWidget::setWindowState( QWidget::windowState() ^ Qt::WindowFullScreen );
		event->accept();
		break;
	default:
		QMainWindow::keyPressEvent( event );
		break;
	}
}



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
}



void MainWindow::addFeaturesToToolBar()
{
	auto controls = new QToolBar(tr("Kontrol kelas"), this);
	controls->setObjectName(QStringLiteral("classControls"));
	controls->setMovable(false);
	controls->setFloatable(false);
	addToolBarBreak(Qt::TopToolBarArea);
	addToolBar(Qt::TopToolBarArea, controls);
	auto allActionsButton = new QToolButton(ui->toolBar);
	allActionsButton->setText(tr("Semua yang tampil"));
	allActionsButton->setToolTip(tr("Semua tindakan komputer"));
	allActionsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	allActionsButton->setIcon(DesktopTheme::icon(QIcon(QStringLiteral(":/core/ui-icons/ellipsis.png"))));
	connect(allActionsButton, &QToolButton::clicked, this, [this]() {
		QDialog panel(this);
		panel.setWindowTitle(tr("Tindakan untuk semua komputer yang tampil"));
		panel.resize(680, 560);
		auto layout = new QVBoxLayout(&panel);
		auto scroll = new QScrollArea(&panel);
		scroll->setWidgetResizable(true);
		auto content = new QWidget(scroll);
		auto sections = new QVBoxLayout(content);
		QList<QGridLayout*> grids;
		for (const auto& title : {tr("Tampilan dan kontrol"), tr("Tindakan"), tr("Daya dan sesi")})
		{
			auto heading = new QLabel(title, content);
			heading->setObjectName(QStringLiteral("sectionHeading"));
			sections->addWidget(heading);
			auto grid = new QGridLayout;
			grid->setSpacing(10);
			grid->setColumnStretch(0, 1);
			grid->setColumnStretch(1, 1);
			sections->addLayout(grid);
			grids.append(grid);
		}
		int counts[] = {0, 0, 0};
		for (const auto& feature : m_master.features())
		{
			if (feature.testFlag(Feature::Flag::Meta))
			{
				continue;
			}
			auto button = findChild<ToolButton*>(feature.name());
			int section = feature.testFlag(Feature::Flag::Mode) ? 0 : 1;
			if (QStringList{QStringLiteral("PowerOn"), QStringLiteral("PowerDown"), QStringLiteral("Reboot"),
				QStringLiteral("UserLogin"), QStringLiteral("UserLogoff")}.contains(feature.name()))
			{
				section = 2;
			}
			const auto subFeatures = m_master.subFeatures(feature.uid());
			auto addButton = [&, section](const QString& label, const QIcon& icon) {
				auto action = new QPushButton(content);
				action->setAccessibleName(label);
				action->setToolTip(label);
				auto buttonLayout = new QHBoxLayout(action);
				buttonLayout->setContentsMargins(8, 4, 8, 4);
				auto symbol = new QLabel(action);
				symbol->setPixmap(icon.pixmap(20, 20));
				buttonLayout->addWidget(symbol);
				auto text = new QLabel(label, action);
				text->setWordWrap(true);
				text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
				text->setAttribute(Qt::WA_TransparentForMouseEvents);
				symbol->setAttribute(Qt::WA_TransparentForMouseEvents);
				buttonLayout->addWidget(text, 1);
				action->setMinimumHeight(42);
				action->setEnabled(button->isEnabled());
				auto& index = counts[section];
				grids[section]->addWidget(action, index / 2, index % 2);
				++index;
				return action;
			};
			auto action = addButton(button->text(), button->icon());
			connect(action, &QPushButton::clicked, &panel, [this, &panel, feature]() {
					panel.accept();
					m_master.runFeature(feature);
					updateModeButtonGroup();
					if (feature.testFlag(Feature::Flag::Mode))
					{
						reloadSubFeatures();
					}
				});
			if (feature.testFlag(Feature::Flag::Mode))
			{
				action->setCheckable(true);
				action->setChecked(button->isChecked());
			}
			for (const auto& subFeature : subFeatures)
			{
				auto subAction = addButton(tr("%1 : %2").arg(button->text(), localizedFeatureLabel(subFeature)),
					DesktopTheme::icon(QIcon(subFeature.iconUrl())));
				connect(subAction, &QPushButton::clicked, &panel, [this, &panel, button, feature, subFeature]() {
						panel.accept();
						m_master.runFeature(subFeature);
						if (feature.testFlag(Feature::Flag::Mode))
						{
							if (!subFeature.testFlag(Feature::Flag::Option))
							{
								button->setChecked(true);
							}
							reloadSubFeatures();
						}
					});
			}
		}
		sections->addStretch();
		scroll->setWidget(content);
		layout->addWidget(scroll);
		auto close = new QDialogButtonBox(QDialogButtonBox::Close, &panel);
		connect(close, &QDialogButtonBox::rejected, &panel, &QDialog::reject);
		layout->addWidget(close);
		panel.exec();
	});

	for( const auto& feature : m_master.features() )
	{
		if( feature.testFlag( Feature::Flag::Meta ) )
		{
			continue;
		}

		const QMap<QString, QColor> actionColors{
			{QStringLiteral("MonitoringMode"), QColor(QStringLiteral("#15803d"))},
			{QStringLiteral("RemoteView"), QColor(QStringLiteral("#2563eb"))},
			{QStringLiteral("RemoteControl"), QColor(QStringLiteral("#7c3aed"))},
			{QStringLiteral("Chat"), QColor(QStringLiteral("#059669"))},
			{QStringLiteral("Screenshot"), QColor(QStringLiteral("#c2410c"))}
		};
		auto actionColor = actionColors.value(feature.name());
		if (actionColor.isValid() && VeyonCore::useDarkMode())
		{
			actionColor = actionColor.lighter(160);
		}
		auto btn = new ToolButton( DesktopTheme::icon(QIcon( modernFeatureIconUrl( feature ) ), actionColor),
										  localizedFeatureLabel( feature ),
										  localizedFeatureActiveLabel( feature ),
										  feature.description(),
										  feature.shortcut() );
		connect(btn, &QToolButton::clicked, this, [=, this] () {
			m_master.runFeature( feature );
			updateModeButtonGroup();
			if( feature.testFlag( Feature::Flag::Mode ) )
			{
				reloadSubFeatures();
			}
		});
		btn->setObjectName( feature.name() );
		btn->setParent(ui->toolBar);
		const auto quickAction = feature.name() == QStringLiteral("MonitoringMode") ||
			feature.name() == QStringLiteral("RemoteView") ||
			feature.name() == QStringLiteral("RemoteControl") ||
			feature.name() == QStringLiteral("Chat") ||
			feature.name() == QStringLiteral("Screenshot");
		if (quickAction)
		{
			btn->addTo(ui->toolBar);
		}
		else
		{
			if (feature.testFlag(Feature::Flag::Mode))
			{
				btn->addTo(controls);
			}
			else
			{
				btn->hide();
			}
			if (!feature.shortcut().isEmpty())
			{
				auto shortcutAction = new QAction(this);
				shortcutAction->setShortcut(feature.shortcut());
				connect(shortcutAction, &QAction::triggered, this, [this, feature]() {
					m_master.runFeature(feature);
					updateModeButtonGroup();
					if (feature.testFlag(Feature::Flag::Mode))
					{
						reloadSubFeatures();
					}
				});
				addAction(shortcutAction);
			}
		}

		if( feature.testFlag( Feature::Flag::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, buttonId( feature ) );
		}
	}
	ui->toolBar->addSeparator();
	ui->toolBar->addWidget(allActionsButton);
}



void MainWindow::addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature )
{
	if( button->menu() )
	{
		button->menu()->close();
		button->menu()->deleteLater();
		button->setMenu( nullptr );
	}

	const auto parentFeatureIsMode = parentFeature.testFlag( Feature::Flag::Mode );
	const auto subFeatures = m_master.subFeatures( parentFeature.uid() );

	if( subFeatures.isEmpty() ||
		( parentFeatureIsMode && button->isChecked() ) )
	{
		return;
	}

	auto menu = new QMenu( button );
	menu->setObjectName( parentFeature.name() );
	menu->setToolTipsVisible( true );

	for( const auto& subFeature : subFeatures )
	{
		auto action = menu->addAction(DesktopTheme::icon(QIcon(subFeature.iconUrl())), localizedFeatureLabel( subFeature ),
							   #if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
									  subFeature.shortcut(),
							   #endif
									  this, [=, this]()
		{
			m_master.runFeature( subFeature );
			if( parentFeatureIsMode )
			{
				if( subFeature.testFlag( Feature::Flag::Option ) == false )
				{
					button->setChecked( true );
				}
				reloadSubFeatures();
			}
		}
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
		,subFeature.shortcut()
#endif
		);
		action->setToolTip( subFeature.description() );
		action->setObjectName( subFeature.uid().toString() );

		if( subFeature.testFlag( Feature::Flag::Option ) )
		{
			action->setCheckable( true );
			action->setChecked( subFeature.testFlag( Feature::Flag::Checked ) );
		}
	}

	button->setMenu( menu );
	button->setPopupMode( ToolButton::InstantPopup );
}



void MainWindow::updateModeButtonGroup()
{
	for (const auto& feature : m_master.features())
	{
		if (feature.testFlag(Feature::Flag::Mode) && feature.uid() == m_master.currentMode())
		{
			if (auto button = m_modeGroup->button(buttonId(feature)))
			{
				button->setChecked(true);
			}
			break;
		}
	}
}



void MainWindow::loadComputerPositions()
{
	if (const auto fileName = QFileDialog::getOpenFileName(this, tr("Muat posisi komputer"),
														   QDir::homePath(), tr("JSON files (*.json)"));
		!fileName.isEmpty())
	{
		if (QFile file(fileName); file.open(QFile::ReadOnly))
		{
			const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();
			const auto config = QJsonDocument::fromJson(file.readAll()).object();
			const auto uiConfig = config[computerPositionsProperty.parentKey()].toObject();
			const auto computerPositions = uiConfig[computerPositionsProperty.key()].toObject()[QStringLiteral("JsonStoreArray")].toArray();

			ui->computerMonitoringWidget->loadPositions(computerPositions);
			ui->computerMonitoringWidget->setUseCustomComputerPositions(true);
			ui->computerMonitoringWidget->doItemsLayout();
		}
	}
}



void MainWindow::saveComputerPositions()
{
	if (const auto fileName = QFileDialog::getSaveFileName(this, tr("Simpan posisi komputer"),
														   QDir::homePath(), tr("JSON files (*.json)"));
		!fileName.isEmpty())
	{
		if (QFile file(fileName); file.open(QFile::WriteOnly | QFile::Truncate))
		{
			const auto& computerPositionsProperty = m_master.userConfig().computerPositionsProperty();

			// create structure identical to UserConfig so file with positions can be used as template for UserConfig
			QJsonObject computerPositions;
			computerPositions[QStringLiteral("JsonStoreArray")] = ui->computerMonitoringWidget->savePositions();
			QJsonObject uiConfig;
			uiConfig[computerPositionsProperty.key()] = computerPositions;
			QJsonObject config;
			config[computerPositionsProperty.parentKey()] = uiConfig;
			file.write(QJsonDocument(config).toJson());
		}
	}
}

void MainWindow::updateLicenseBanner(const LicenseState& state)
{
	const bool visible = licenseNeedsBanner(state);
	m_licenseBanner->setVisible(visible);
	if (visible)
	{
		m_licenseBanner->setText(LicenseService::describe(state));
	}
}
