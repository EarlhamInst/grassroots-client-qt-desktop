/*
** Copyright 2014-2016 The Earlham Institute
** 
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** 
**     http://www.apache.org/licenses/LICENSE-2.0
** 
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#include <QAction>
#include <QActionGroup>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPushButton>
#include <QPluginLoader>
#include <QTextStream>
#include <QGuiApplication>
#include <QUrl>
#include <QTabWidget>
#include <QToolBar>
#include <QScreen>
#include <QWidget>
#include <QStatusBar>


#include "string_utils.h"
#include "request_tools.h"

#include "json_tools.h"
#include "json_util.h"

#include "main_window.h"
#include "results_window.h"
#include "filesystem_utils.h"

#include "qt_client_data.h"
#include "ui_utils.h"
#include "user_details.h"


#ifdef _DEBUG
	#define MAIN_WINDOW_DEBUG	(STM_LEVEL_FINE)
#else
	#define MAIN_WINDOW_DEBUG	(STM_LEVEL_NONE)
#endif


MainWindow :: MainWindow (QTClientData *data_p)
: mw_client_data_p (data_p)
{
	setAcceptDrops (true);

	mw_prefs_widget_p = new PrefsWidget (this, PL_SIMPLE, false, data_p);
	connect (mw_prefs_widget_p, &PrefsWidget :: RunServices, this, &MainWindow :: RunServices);

	setCentralWidget (mw_prefs_widget_p);

	setWindowTitle (data_p -> qcd_dummy_arg_s);
	setWindowIcon (QIcon ("images/app"));

	mw_current_level = PL_SIMPLE;

	AddActions ();

	QStatusBar *status_bar_p = statusBar ();
	QPixmap user_img ("images/user");
	QLabel *user_img_p = new QLabel;

	user_img_p -> setPixmap (user_img);
	status_bar_p -> addWidget (user_img_p);

	mw_user_label_p = new QLabel;
	status_bar_p -> addWidget (mw_user_label_p);


	QScreen *screen_p = QGuiApplication :: primaryScreen ();
	QSize screen_size = screen_p -> availableGeometry ().size ();
	resize (screen_size * 0.5);
	move (static_cast <int> (screen_size.width () * 0.25), static_cast <int> (screen_size.height () * 0.25));
}


MainWindow :: ~MainWindow ()
{
}


void MainWindow :: GetIndexingData ()
{
	qDebug () << "GetIndexingData" << Qt :: endl;

	QVector <const char *> *active_services_p = mw_prefs_widget_p -> GetActiveServices ();

	if (active_services_p)
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					bool success_flag = true;
					const int last_index = active_services_p -> size () - 1;

					qDebug () << "last_index: " << last_index << Qt :: endl;

					if (last_index >= 0)
						{
							for (int i = 0; i < last_index; ++ i)
								{
									const char *name_s = active_services_p -> takeFirst ();

									qDebug () << "i: " << i << " name: " << name_s << Qt :: endl;

									if (!AppendStringsToByteBuffer (buffer_p, name_s, ",", NULL))
										{
											success_flag = false;
											i = last_index;
										}
								}

							if (success_flag)
								{
									const char *name_s = active_services_p -> takeFirst ();

									if (AppendStringToByteBuffer (buffer_p, name_s))
										{
											const char *services_s = GetByteBufferData (buffer_p);

											if (services_s)
												{
													json_t *req_p = GetNamedServicesIndexingDataRequest (nullptr, services_s, mw_client_data_p -> qcd_base_data.cd_schema_p);

													if (req_p)
														{
															if (mw_client_data_p -> qcd_verbose_flag)
																{
																	PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, req_p, "Client sending: ");
																}

															setCursor (Qt :: BusyCursor);
															json_t *services_json_p = CallServices (req_p, nullptr, mw_client_data_p -> qcd_base_data.cd_connection_p);
															setCursor (Qt :: ArrowCursor);

															if (services_json_p)
																{
																	json_decref (services_json_p);
																}

															json_decref (req_p);
														}

												}
										}
								}
						}

					FreeByteBuffer (buffer_p);
				}
			else
				{
					qDebug () << "No buffer" << Qt :: endl;
				}

			delete active_services_p;
		}
	else
		{
			qDebug () << "No services" << Qt :: endl;
		}
}

void MainWindow :: Run ()
{
	RunServices (true);
}


void MainWindow :: Reject ()
{
	RunServices (false);
}


void MainWindow :: SelectService (const char *service_name_s, const json_t *params_json_p)
{	
	mw_prefs_widget_p -> SelectService (service_name_s, params_json_p);
	mw_tabs_p -> setCurrentWidget (mw_prefs_widget_p);
}



void MainWindow :: ProcessResults (json_t *results_json_p)
{
	if (mw_client_data_p -> qcd_verbose_flag)
		{
			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, results_json_p, "\n\nDATA:\n");
		}

	json_t *services_json_p = json_object_get (results_json_p, SERVICE_RESULTS_S);

	if (json_is_array (services_json_p))
		{
			ProgressWindow *progress_p = mw_client_data_p -> qcd_progress_p;
			bool show_progress_flag = false;
			ResultsWindow *results_p = nullptr;
			bool show_results_flag = false;

			size_t i;
			json_t *job_p;

			json_array_foreach (services_json_p, i, job_p)
				{
					const char *service_name_s = GetJSONString (job_p, JOB_SERVICE_S);

					if (service_name_s)
						{
							bool run_service_flag = false;

							GetJSONBoolean (job_p, SERVICE_RUN_S, &run_service_flag);

							if (run_service_flag)
								{
									if (!results_p)
										{
											results_p = GetNewResultsWindow (mw_client_data_p);
										}

									if (results_p)
										{
											if (results_p -> AddInterestedService (job_p, service_name_s))
												{
													if (!show_results_flag)
														{
															show_results_flag = true;
														}
												}
										}
									else
										{

										}

								}		/* if (run_service_flag) */
							else
								{
									const char *service_description_s = GetJSONString (job_p, OPERATION_DESCRIPTION_S);
									const char *service_uri_s =  GetJSONString (job_p, OPERATION_INFORMATION_URI_S);

									/* Get the job status */
									OperationStatus status = OS_ERROR;
									const char *value_s = GetJSONString (job_p, SERVICE_STATUS_S);

									if (value_s)
										{
											status = GetOperationStatusFromString (value_s);
										}
									else
										{
											json_int_t i;
											/* Get the job status */

											if (GetJSONInteger (job_p, SERVICE_STATUS_VALUE_S, &i))
												{
													if ((i > OS_LOWER_LIMIT) && (i < OS_UPPER_LIMIT))
														{
															status = static_cast <OperationStatus> (i);
														}
												}
										}

									if (status != OS_ERROR)
										{
											json_t *errors_p = nullptr;

											if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
												{
													if (!results_p)
														{
															results_p = GetNewResultsWindow (mw_client_data_p);
														}

													if (results_p)
														{
															results_p -> AddResultsPageFromJSON (job_p, service_name_s, service_description_s, service_uri_s);
														}
													show_results_flag = true;
												}
											else
												{
													progress_p -> AddProgressItemFromJSON (job_p, service_name_s, service_description_s, service_uri_s);
													show_progress_flag = true;
												}

											errors_p = json_object_get (job_p, "errors");

											if (errors_p)
												{
													if (service_name_s)
														{
															mw_prefs_widget_p -> SetServiceErrors (service_name_s, errors_p);
														}
												}

										}		/* if (status != OS_ERROR) */

								}		/* if (run_service_flag) else */

						}		/* if (service_name_s) */

				}		/* json_array_foreach (services_json_p, i, job_p) */

			if (show_progress_flag)
				{
					progress_p -> show ();
					progress_p -> raise ();
				}

			if (show_results_flag)
				{
					results_p -> show ();
					results_p -> raise ();
				}

		}		/* if (json_is_array (services_json_p)) */

}



void MainWindow :: RunServices (bool run_flag)
{
	if (run_flag)
		{
			json_t *client_params_p = GetUserValuesAsJSON (false);

			if (client_params_p)
				{
                char *services_json_s = json_dumps (client_params_p, JSON_INDENT (2));

                if (services_json_s)
                {
                    printf ("RunServices () sending: %s\n", services_json_s);
                    free (services_json_s);
                }

					RunService (client_params_p);

					json_decref (client_params_p);
				}		/* if (client_params_p) */
		}
	else
		{
			close ();
		}
}


void MainWindow :: SetUser (User *user_p)
{
	mw_user_label_p -> setText (user_p -> us_email_s);
}



void MainWindow :: RunService (json_t *service_p)
{
	if (service_p)
		{
            //if (mw_client_data_p -> qcd_verbose_flag)
            //	{
            //		PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, service_p, "Client sending: ");
            //	}

			setCursor (Qt :: BusyCursor);
			json_t *services_json_p = CallServices (service_p, nullptr, mw_client_data_p -> qcd_base_data.cd_connection_p);
			setCursor (Qt :: ArrowCursor);

			if (services_json_p)
				{
					if (mw_client_data_p -> qcd_verbose_flag)
						{
							PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, service_p, "Client received: ");
						}

					ProcessResults (services_json_p);
					json_decref (services_json_p);
				}		/* if (services_json_p) */

		}		/* if (service_p) */
}



//bool MainWindow :: AddResults (const json_t *service_results_p)
//{
//	bool success_flag = false;
//	json_t *jobs_p = json_object_get (service_results_p, SERVICE_JOBS_S);

//	if (jobs_p)
//		{
//			const char *service_name_s = GetJSONString (service_results_p, SERVICE_NAME_S);
//			const char *service_description_s = GetJSONString (service_results_p, SERVICES_DESCRIPTION_S);
//			const char *service_uri_s = NULL;

//			if (json_is_array (jobs_p))
//				{
//					size_t i;
//					json_t *job_p;

//					json_array_foreach (jobs_p, i, job_p)
//						{
//							if (mw_client_data_p -> qcd_results_p -> AddResultsPageFromJSON (job_p, service_name_s,  service_description_s, service_uri_s))
//								{
//									success_flag = true;
//								}
//						}
//				}
//			else
//				{
//					success_flag = mw_client_data_p -> qcd_results_p -> AddResultsPageFromJSON (jobs_p, service_name_s,  service_description_s, service_uri_s);
//				}
//		}

//	return success_flag;
//}


void MainWindow :: CreateAndAddServicePage (const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p)
{
	mw_prefs_widget_p -> CreateAndAddServicePage (service_name_s, service_description_s, service_info_uri_s, service_icon_uri_s, provider_p, params_p, metadata_p, mw_current_level);
}


json_t *MainWindow :: GetUserValuesAsJSON (bool all_flag) const
{
	return mw_prefs_widget_p -> GetUserValuesAsJSON (all_flag, mw_current_level);
}


void MainWindow :: LoadConfiguration ()
{
	QString filename = QFileDialog :: getOpenFileName (this, tr ("Save Configuration"), "wheatis_config.json", tr ("JSON (*.json)"));

	if (! (filename.isNull () || filename.isEmpty ()))
		{
			LoadConfigurationFile (filename);
		}		/* if (! (filename.isNull () || filename.isEmpty ())) */

}


void MainWindow :: SetParams (json_t *config_p)
{

	if (json_is_array (config_p))
		{
			const size_t num_services = json_array_size (config_p);
			size_t i = 0;

			for (i = 0; i < num_services; ++ i)
				{
					json_t *service_json_p = json_array_get (config_p, i);

				}		/* for (i = 0; i < num_services; ++ i) */

		}		/* if (json_is_array (config_p) */
	else
		{

		}
}


void MainWindow :: LoadConfigurationFile (QString &filename_r)
{
	char c = GetFileSeparatorChar ();
	if (c != '/')
		{
			filename_r.replace('/', c);
		}

	QByteArray ba = filename_r.toLocal8Bit ();
	const char * const filename_s = ba.constData ();

	json_error_t error;
	json_t *config_p = json_load_file (filename_s, 0, &error);

	if (config_p)
		{
			mw_prefs_widget_p -> SetServiceParams (config_p);
		}		/* if (config_p) */
	else
		{

		}
}


void MainWindow :: SaveConfiguration ()
{
	QString filename = QFileDialog :: getSaveFileName (this, tr ("Save Configuration"), "grassroots_client_config.json", tr ("JSON (*.json)"));

	if (! (filename.isNull () || filename.isEmpty ()))
		{
			QByteArray ba = filename.toLocal8Bit ();
			const char * const filename_s = ba.constData ();

			json_t *res_p = GetUserValuesAsJSON (true);

			if (res_p)
				{
					int res = json_dump_file (res_p, filename_s, JSON_INDENT (2) | JSON_PRESERVE_ORDER);

					if (res != 0)
						{

						}

					json_decref (res_p);
				}

		}		/* if (! (filename.isNull () || filename.isEmpty ())) */

}



void MainWindow :: dropEvent (QDropEvent *event_p)
{
	QList <QUrl> urls = event_p -> mimeData () -> urls ();

	if (! (urls.isEmpty ()))
		{
			QString filename = urls.first ().toLocalFile ();

			if (! ((filename.isEmpty ()) || (filename.isNull ())))
				{
					LoadConfigurationFile (filename);
				}		/* if (! (filename.isEmpty ())) */

		}		/* if (! (urls.isEmpty ())) */

}


void MainWindow :: dragEnterEvent (QDragEnterEvent *event_p)
{
	event_p -> acceptProposedAction ();
}


void MainWindow :: closeEvent (QCloseEvent *event_p)
{
	event_p -> accept ();
	emit Closed ();
}



void MainWindow :: AddActions ()
{
	QMenuBar *menu_bar_p = menuBar ();
	QToolBar *main_toolbar_p = new QToolBar (tr ("Main"));
	main_toolbar_p -> setToolButtonStyle (Qt :: ToolButtonFollowStyle);

	// File Menu
	QMenu *menu_p = menu_bar_p -> addMenu (tr ("&File"));

	// Load configuration
	QAction *action_p = new QAction (QIcon ("images/open"), tr ("&Load Configuration..."), this);
	action_p -> setShortcuts (QKeySequence :: Open);
	action_p -> setStatusTip (tr ("Load a configuration file."));
	connect (action_p, &QAction :: triggered, this, &MainWindow :: LoadConfiguration);
	menu_p -> addAction (action_p);
	main_toolbar_p -> addAction (action_p);

	// Save configuration
	action_p = new QAction (QIcon ("images/save"), tr ("&Save Configuration..."), this);
	action_p -> setShortcuts (QKeySequence :: Save);
	action_p -> setStatusTip (tr ("Save configuration file."));
	connect (action_p, &QAction :: triggered, this, &MainWindow :: SaveConfiguration);
	menu_p -> addAction (action_p);
	main_toolbar_p -> addAction (action_p);

	main_toolbar_p -> addSeparator ();

	// Run
	action_p = new QAction (QIcon ("images/run"), tr ("Run"), this);
	main_toolbar_p -> addAction (action_p);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: Run);


	// Indexing json
	action_p = new QAction (QIcon ("images/index"), tr ("Get Indexing Data"), this);
	main_toolbar_p -> addAction (action_p);
	connect (action_p, &QAction :: triggered, this, &MainWindow :: GetIndexingData);


	// Cancel
	action_p = new QAction (QIcon ("images/cancel"), tr ("Quit"), this);
	main_toolbar_p -> addAction (action_p);
	connect (action_p, &QAction :: triggered, this, &MainWindow :: Run);


	// Connect Menu
	menu_p = menu_bar_p -> addMenu (tr ("&Connect"));
	action_p = new QAction (tr ("Connect to server"), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: ConnectToServer);
	menu_p -> addAction (action_p);


	// Operations

	// Get all services
	action_p = new QAction (tr ("Get all services"), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: GetAllServices);
	menu_p -> addAction (action_p);

	// Get interested services
	action_p = new QAction (tr ("Get interested services"), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: GetInterestedServices);
	menu_p -> addAction (action_p);

	// Get named services
	action_p = new QAction (tr ("Get named services"), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: GetNamedServices);
	menu_p -> addAction (action_p);


	// View Menu
	menu_p = menu_bar_p -> addMenu (tr ("&View"));

	// View progress window
	action_p = new QAction (tr ("Progress"), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: ShowProgressWindow);
	menu_p -> addAction (action_p);


	// Tools Menu
	menu_p = menu_bar_p -> addMenu (tr ("&Tools"));

	QMenu *sub_menu_p = menu_p -> addMenu (tr ("Level"));
	QActionGroup *interface_levels_p = new QActionGroup (this);

	// Easy Level
	action_p = new QAction (tr ("Basic"), this);
	action_p -> setStatusTip (tr ("Basic"));
	action_p -> setCheckable (true);
	action_p -> setChecked (mw_current_level == PL_SIMPLE);
	connect (action_p, &QAction :: triggered, this, &MainWindow :: SetSimpleInterfaceLevel);
	sub_menu_p -> addAction (action_p);
	interface_levels_p -> addAction (action_p);

	// Advanced Lavel
	action_p = new QAction (tr ("Advanced"), this);
	action_p -> setStatusTip (tr ("Advanced"));
	action_p -> setCheckable (true);
	action_p -> setChecked (mw_current_level == PL_ADVANCED);
	connect (action_p, &QAction :: triggered, this, &MainWindow :: SetAdvancedInterfaceLevel);
	sub_menu_p -> addAction (action_p);
	interface_levels_p -> addAction (action_p);

	// Wizard Level
	action_p = new QAction (tr ("Wizard"), this);
	action_p -> setStatusTip (tr ("Wizard"));
	action_p -> setCheckable (true);
	action_p -> setChecked (mw_current_level == PL_WIZARD);
	connect (action_p, &QAction :: triggered, this, &MainWindow :: SetWizardInterfaceLevel);
	sub_menu_p -> addAction (action_p);
	interface_levels_p -> addAction (action_p);

	addToolBar (main_toolbar_p);
}


void MainWindow :: SetSimpleInterfaceLevel ()
{
    SetParameterLevel (PL_SIMPLE);
}


void MainWindow :: SetAdvancedInterfaceLevel ()
{
    SetParameterLevel (PL_ADVANCED);
}


void MainWindow :: SetWizardInterfaceLevel ()
{
    SetParameterLevel (PL_WIZARD);
}


void MainWindow :: SetParameterLevel (ParameterLevel pl)
{
	mw_current_level = pl;
	mw_prefs_widget_p -> SetInterfaceLevel (mw_current_level);
	update ();
}


void MainWindow :: GetAllServices ()
{
	User *user_p = nullptr;
	Client *client_p = mw_client_data_p -> qcd_base_data.cd_client_p;
	GetAllServicesInClient (client_p, user_p);
}


void MainWindow :: GetInterestedServices ()
{

}


void MainWindow :: GetNamedServices ()
{

}



void MainWindow :: ConnectToServer ()
{

}


void MainWindow :: ShowProgressWindow ()
{
	mw_client_data_p -> qcd_progress_p -> show ();
	mw_client_data_p -> qcd_progress_p -> raise ();
}
