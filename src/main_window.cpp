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
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QNetworkCookie>
#include <QUrl>
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
#include <QMessageBox>
#include <QApplication>
#include <QThread>


#include "string_utils.h"
#include "request_tools.h"
#include "connection.h"
#include "curl_tools.h"
#include "schema_keys.h"

#include "json_tools.h"
#include "json_util.h"

#include "main_window.h"
#include "results_window.h"
#include "filesystem_utils.h"

#include "qt_client_data.h"
#include "sso_login_dialog.h"
#include "ui_utils.h"
#include "user_details.h"


/** How long, in seconds, to wait for a server to send its list of services */
static const long S_FETCH_SERVICES_TIMEOUT = 30L;

/** The HTTP status code for when a server requires authentication */
static const long S_HTTP_UNAUTHORIZED = 401L;

/** The authentication methods that we can use with a username and password */
static const long S_PASSWORD_AUTH_METHODS = CURLAUTH_BASIC | CURLAUTH_DIGEST | CURLAUTH_DIGEST_IE | CURLAUTH_NTLM;


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

	mw_fetch_thread_p = nullptr;
	mw_fetch_response_p = nullptr;
	mw_pending_connection_p = nullptr;
	mw_sso_retry_flag = false;

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
			else
				{
					QMessageBox :: warning (this, "Error running service", "Failed to get parameter values");
				}
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
	if (IsFetchingServices ())
		{
			/*
			 * The fetch thread is using the Connection so we can't
			 * close until it has finished.
			 */
			QMessageBox :: information (this, tr ("Getting services"), tr ("Please wait until the services have been received from the server before closing"));
			event_p -> ignore ();
			return;
		}

	event_p -> accept ();
	emit Closed ();
}



void MainWindow :: AddActions ()
{
	QMenuBar *menu_bar_p = menuBar ();
	QToolBar *main_toolbar_p = new QToolBar (tr ("Main"));
	main_toolbar_p -> setToolButtonStyle (Qt :: ToolButtonFollowStyle);
	mw_toolbar_p = main_toolbar_p;

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
	action_p = new QAction (tr ("Connect to server..."), this);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: ConnectToServer);
	menu_p -> addAction (action_p);
	mw_connect_action_p = action_p;


	// Operations

	// Refresh services
	action_p = new QAction (tr ("Refresh services"), this);
	action_p -> setShortcut (QKeySequence :: Refresh);
	connect (action_p, &QAction :: triggered, 	this, &MainWindow :: RefreshServices);
	menu_p -> addAction (action_p);
	mw_refresh_action_p = action_p;

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


void MainWindow :: RefreshServices ()
{
	if (!IsFetchingServices ())
		{
			FetchServices (mw_client_data_p -> qcd_base_data.cd_connection_p);
		}
}


void MainWindow :: GetInterestedServices ()
{

}


void MainWindow :: GetNamedServices ()
{

}



void MainWindow :: ConnectToServer ()
{
	if (IsFetchingServices ())
		{
			return;
		}

	const QString current_url (mw_client_data_p -> qcd_server_url_s);
	bool ok_flag = false;
	QString url = QInputDialog :: getText (this, tr ("Connect to server"), tr ("Grassroots server URL:"), QLineEdit :: Normal, current_url, &ok_flag).trimmed ();

	if (ok_flag && (!url.isEmpty ()) && (url != current_url))
		{
			if (mw_client_data_p -> qcd_progress_p -> HasProgressItems ())
				{
					if (QMessageBox :: question (this, tr ("Connect to server"), tr ("The jobs from %1 will be removed from the progress window. Do you want to continue?").arg (current_url)) != QMessageBox :: Yes)
						{
							return;
						}
				}

			QByteArray url_ba = url.toUtf8 ();
			Connection *new_connection_p = AllocateWebServerConnection (url_ba.constData (), CM_MEMORY);

			if (new_connection_p)
				{
					if (new_connection_p -> co_type == CT_WEB)
						{
							SetCurlToolVerbose (reinterpret_cast <WebConnection *> (new_connection_p) -> wc_curl_p, mw_client_data_p -> qcd_verbose_flag);
						}

					/* We only switch to the new server once it has sent us its services */
					mw_pending_connection_p = new_connection_p;
					mw_sso_retry_flag = false;
					FetchServices (new_connection_p);
				}
			else
				{
					QMessageBox :: warning (this, tr ("Connect to server"), tr ("Failed to create a connection to %1").arg (url));
				}
		}
}


bool MainWindow :: IsFetchingServices () const
{
	return (mw_fetch_thread_p != nullptr);
}


void MainWindow :: FetchServices (Connection *connection_p)
{
	json_t *req_p = GetAvailableServicesRequest (nullptr, mw_client_data_p -> qcd_base_data.cd_schema_p);

	if (req_p)
		{
			CurlTool *curl_tool_p = nullptr;
			QString url;

			if (connection_p -> co_type == CT_WEB)
				{
					WebConnection *web_connection_p = reinterpret_cast <WebConnection *> (connection_p);

					curl_tool_p = web_connection_p -> wc_curl_p;
					url = web_connection_p -> wc_uri_s;

					/* Don't let an unresponsive server block us for too long */
					SetCurlToolTimeout (curl_tool_p, S_FETCH_SERVICES_TIMEOUT);
				}

			/*
			 * Only make the network call on the fetch thread, the widgets
			 * for the services must be created on the main thread.
			 */
			mw_fetch_response_p = nullptr;
			mw_fetch_status = FetchStatus ();
			mw_fetch_thread_p = QThread :: create ([this, req_p, connection_p, curl_tool_p] ()
				{
					mw_fetch_response_p = MakeRemoteJsonCall (req_p, connection_p);

					/* Keep the details needed to see if the server requires authentication */
					if (curl_tool_p)
						{
							CURL *curl_p = curl_tool_p -> ct_curl_p;
							char *url_s = nullptr;

							curl_easy_getinfo (curl_p, CURLINFO_RESPONSE_CODE, &mw_fetch_status.fs_http_status);
							curl_easy_getinfo (curl_p, CURLINFO_HTTPAUTH_AVAIL, &mw_fetch_status.fs_auth_available);
							curl_easy_getinfo (curl_p, CURLINFO_REDIRECT_COUNT, &mw_fetch_status.fs_redirect_count);

							if ((curl_easy_getinfo (curl_p, CURLINFO_EFFECTIVE_URL, &url_s) == CURLE_OK) && url_s)
								{
									mw_fetch_status.fs_effective_url = QString :: fromUtf8 (url_s);
								}

							url_s = nullptr;
							if ((curl_easy_getinfo (curl_p, CURLINFO_REDIRECT_URL, &url_s) == CURLE_OK) && url_s)
								{
									mw_fetch_status.fs_redirect_url = QString :: fromUtf8 (url_s);
								}
						}

					json_decref (req_p);
				});

			connect (mw_fetch_thread_p, &QThread :: finished, this, [this, connection_p, curl_tool_p] ()
				{
					json_t *response_p = mw_fetch_response_p;
					const FetchStatus status = mw_fetch_status;

					if (curl_tool_p)
						{
							SetCurlToolTimeout (curl_tool_p, 0L);
						}

					mw_fetch_thread_p -> deleteLater ();
					mw_fetch_thread_p = nullptr;
					mw_fetch_response_p = nullptr;

					SetFetchingServices (false);
					ProcessFetchedServices (response_p, connection_p, status);
				});

			SetFetchingServices (true, url);
			mw_fetch_thread_p -> start ();
		}
	else
		{
			QMessageBox :: warning (this, tr ("Getting services"), tr ("Failed to create the request for the list of services"));

			if (connection_p == mw_pending_connection_p)
				{
					FreeConnection (mw_pending_connection_p);
					mw_pending_connection_p = nullptr;
				}
		}
}


void MainWindow :: ProcessFetchedServices (json_t *response_p, Connection *connection_p, const FetchStatus &status_r)
{
	const bool switching_flag = (connection_p == mw_pending_connection_p);
	const bool sso_retry_flag = mw_sso_retry_flag;
	const json_t *services_p = response_p ? json_object_get (response_p, SERVICES_NAME_S) : nullptr;
	QString url;


	mw_sso_retry_flag = false;

	if (connection_p -> co_type == CT_WEB)
		{
			url = reinterpret_cast <WebConnection *> (connection_p) -> wc_uri_s;
		}

	/*
	 * The server may require authentication, whether we are connecting to it
	 * at start up, switching to it or our session with it has expired.
	 */
	if (! (services_p && json_is_array (services_p)))
		{
			const AuthenticationType auth_type = GetRequiredAuthentication (status_r, url);

			if (auth_type != AT_NONE)
				{
					if (response_p)
						{
							json_decref (response_p);
						}

					if (auth_type == AT_PASSWORD)
						{
							/*
							 * Ask the user for their details and try again. If they have
							 * already given some, then they were rejected.
							 */
							const char *previous_username_s = reinterpret_cast <WebConnection *> (connection_p) -> wc_curl_p -> ct_username_s;
							const bool retry_flag = (previous_username_s != nullptr);
							QString username (QString :: fromUtf8 (previous_username_s));
							QString password;

							if (GetCredentials (url, retry_flag, username, password))
								{
									QByteArray username_ba = username.toUtf8 ();
									QByteArray password_ba = password.toUtf8 ();

									if (SetConnectionCredentials (connection_p, username_ba.constData (), password_ba.constData ()))
										{
											FetchServices (connection_p);
											return;
										}

									QMessageBox :: warning (this, tr ("Connect to server"), tr ("Failed to set the username and password for %1").arg (url));
								}
						}
					else if (sso_retry_flag)
						{
							/* We already have a session but the server is still asking us to sign in */
							QMessageBox :: warning (this, tr ("Connect to server"), tr ("Signed in to %1 but the server did not accept the session").arg (url));
						}
					else if (SignInWithSingleSignOn (connection_p, url))
						{
							mw_sso_retry_flag = true;
							FetchServices (connection_p);
							return;
						}

					/* Only abandon a server that we were trying to switch to */
					if (switching_flag)
						{
							FreeConnection (connection_p);
							mw_pending_connection_p = nullptr;
						}

					return;
				}
		}

	if (services_p && json_is_array (services_p))
		{
			Client *client_p = mw_client_data_p -> qcd_base_data.cd_client_p;
			const json_t *header_p = json_object_get (response_p, HEADER_S);
			const json_t *user_json_p = json_object_get (response_p, USER_S);


			PrintJSONToLog (STM_LEVEL_INFO, __FILE__, __LINE__, response_p, "Grassroots server sent: ");


			if (switching_flag)
				{
					Connection *old_connection_p = mw_client_data_p -> qcd_base_data.cd_connection_p;

					mw_client_data_p -> qcd_progress_p -> ClearProgressItems ();
					UseConnection (connection_p);
					FreeConnection (old_connection_p);
					mw_pending_connection_p = nullptr;
				}

			/* Get the schema version used by the server */
			if (header_p)
				{
					const json_t *schema_p = json_object_get (header_p, SCHEMA_S);

					if (schema_p)
						{
							SchemaVersion *server_schema_p = GetSchemaVersionFromJSON (schema_p);

							if (server_schema_p)
								{
									SetClientSchema (client_p, server_schema_p);
								}
						}
				}

			if (user_json_p)
				{
					User *user_p = GetUserFromJSON (user_json_p);

					if (user_p)
						{
							SetUser (user_p);
							FreeUser (user_p);
						}
				}
			else
				{

				}

			mw_prefs_widget_p -> ClearServices ();

			json_t *client_results_p = ShowServices (response_p, client_p, nullptr, connection_p);

			if (client_results_p)
				{
					json_decref (client_results_p);
				}
		}
	else
		{
			if (switching_flag)
				{
					const QString current_url (mw_client_data_p -> qcd_server_url_s);

					FreeConnection (connection_p);
					mw_pending_connection_p = nullptr;

					QMessageBox :: warning (this, tr ("Connect to server"), tr ("Failed to get any services from %1, staying connected to %2").arg (url, current_url));
				}
			else
				{
					QMessageBox :: warning (this, tr ("Getting services"), tr ("Failed to get the services from %1").arg (url));
				}
		}

	if (response_p)
		{
			json_decref (response_p);
		}
}


MainWindow :: AuthenticationType MainWindow :: GetRequiredAuthentication (const FetchStatus &status_r, const QString &server_url_r)
{
	if (status_r.fs_http_status == S_HTTP_UNAUTHORIZED)
		{
			/*
			 * If the server doesn't offer any methods that use a username and
			 * password, such as mod_auth_openidc for non-browser requests, then
			 * the user needs to sign in via a browser.
			 */
			return ((status_r.fs_auth_available & S_PASSWORD_AUTH_METHODS) ? AT_PASSWORD : AT_SINGLE_SIGN_ON);
		}

	/* A redirect that wasn't followed, e.g. to an identity provider */
	if ((status_r.fs_http_status >= 300L) && (status_r.fs_http_status < 400L) && (!status_r.fs_redirect_url.isEmpty ()))
		{
			return AT_SINGLE_SIGN_ON;
		}

	/*
	 * We were redirected away from the server, e.g. to an identity provider's
	 * sign in page. This can be on the same host so check the path too, but
	 * ignore just switching from http to https.
	 */
	if (status_r.fs_redirect_count > 0L)
		{
			const QUrl effective_url (status_r.fs_effective_url);
			const QUrl server_url (server_url_r);

			if ((effective_url.host () != server_url.host ()) || (effective_url.path () != server_url.path ()))
				{
					return AT_SINGLE_SIGN_ON;
				}
		}

	return AT_NONE;
}


bool MainWindow :: SignInWithSingleSignOn (Connection *connection_p, const QString &url_r)
{
	const QUrl server_url (url_r);
	SSOLoginDialog dialog (server_url, this);

	if (dialog.exec () == QDialog :: Accepted)
		{
			const QList <QNetworkCookie> cookies = dialog.GetServerCookies ();
			CURL *curl_p = reinterpret_cast <WebConnection *> (connection_p) -> wc_curl_p -> ct_curl_p;
			bool success_flag = true;

			/*
			 * Turn on curl's cookie engine so that the session cookies are sent
			 * with every request to the server and any updates to them that the
			 * server sends back are kept.
			 */
			curl_easy_setopt (curl_p, CURLOPT_COOKIEFILE, "");
			curl_easy_setopt (curl_p, CURLOPT_COOKIELIST, "ALL");

			for (const QNetworkCookie &cookie : cookies)
				{
					/*
					 * Use the Netscape cookie file format so that host-only cookies,
					 * which is what mod_auth_openidc sets by default, stay host-only.
					 * Otherwise curl would treat any updated cookies from the server
					 * as different ones and send both the old and new sessions.
					 */
					QString domain = cookie.domain ();
					const bool tail_match_flag = domain.startsWith ('.');
					const QString path = cookie.path ().isEmpty () ? QString ("/") : cookie.path ();
					const qint64 expiry = cookie.isSessionCookie () ? 0 : cookie.expirationDate ().toSecsSinceEpoch ();
					QByteArray cookie_ba;

					if (domain.isEmpty ())
						{
							domain = server_url.host ();
						}

					if (cookie.isHttpOnly ())
						{
							cookie_ba.append ("#HttpOnly_");
						}

					cookie_ba.append (domain.toUtf8 ());
					cookie_ba.append (tail_match_flag ? "\tTRUE\t" : "\tFALSE\t");
					cookie_ba.append (path.toUtf8 ());
					cookie_ba.append (cookie.isSecure () ? "\tTRUE\t" : "\tFALSE\t");
					cookie_ba.append (QByteArray :: number (expiry));
					cookie_ba.append ('\t');
					cookie_ba.append (cookie.name ());
					cookie_ba.append ('\t');
					cookie_ba.append (cookie.value ());

					const char * const cookie_s = cookie_ba.constData ();




					if (curl_easy_setopt (curl_p, CURLOPT_COOKIELIST, cookie_s) == CURLE_OK)
						{
							qDebug () << "set CURLOPT_COOKIELIST to \"" << cookie_s << "\"" << Qt :: endl;
						}
					else
						{
							success_flag = false;
							qDebug () << "failed to set CURLOPT_COOKIELIST to \"" << cookie_s << "\"" << Qt :: endl;
						}
				}

			if (!success_flag)
				{
					QMessageBox :: warning (this, tr ("Connect to server"), tr ("Failed to store the session for %1").arg (url_r));
				}

			return success_flag;
		}

	return false;
}


bool MainWindow :: GetCredentials (const QString &url_r, const bool retry_flag, QString &username_r, QString &password_r)
{
	QDialog dialog (this);
	QFormLayout *layout_p = new QFormLayout (&dialog);
	QLineEdit *username_p = new QLineEdit (&dialog);
	QLineEdit *password_p = new QLineEdit (&dialog);
	QDialogButtonBox *buttons_p = new QDialogButtonBox (QDialogButtonBox :: Ok | QDialogButtonBox :: Cancel, &dialog);
	QString message = retry_flag ?
		tr ("The username or password for %1 was not accepted, please try again.").arg (url_r) :
		tr ("%1 requires a username and password.").arg (url_r);
	QLabel *message_p = new QLabel (message, &dialog);

	dialog.setWindowTitle (tr ("Connect to server"));

	message_p -> setWordWrap (true);
	username_p -> setText (username_r);
	password_p -> setEchoMode (QLineEdit :: Password);

	layout_p -> addRow (message_p);
	layout_p -> addRow (tr ("Username:"), username_p);
	layout_p -> addRow (tr ("Password:"), password_p);
	layout_p -> addRow (buttons_p);

	connect (buttons_p, &QDialogButtonBox :: accepted, &dialog, &QDialog :: accept);
	connect (buttons_p, &QDialogButtonBox :: rejected, &dialog, &QDialog :: reject);

	if (dialog.exec () == QDialog :: Accepted)
		{
			username_r = username_p -> text ();
			password_r = password_p -> text ();

			return true;
		}

	return false;
}


void MainWindow :: SetFetchingServices (const bool fetching_flag, const QString &url_r)
{
	const bool enabled_flag = !fetching_flag;

	/* Stop anything else using the Connection whilst the fetch thread is running */
	mw_prefs_widget_p -> setEnabled (enabled_flag);
	mw_toolbar_p -> setEnabled (enabled_flag);
	mw_refresh_action_p -> setEnabled (enabled_flag);
	mw_connect_action_p -> setEnabled (enabled_flag);
	mw_client_data_p -> qcd_progress_p -> setEnabled (enabled_flag);

	if (fetching_flag)
		{
			statusBar () -> showMessage (tr ("Getting services from %1...").arg (url_r));
			QApplication :: setOverrideCursor (Qt :: BusyCursor);
		}
	else
		{
			statusBar () -> clearMessage ();
			QApplication :: restoreOverrideCursor ();
		}
}


void MainWindow :: UseConnection (Connection *connection_p)
{
	mw_client_data_p -> qcd_base_data.cd_connection_p = connection_p;
	mw_client_data_p -> qcd_server_url_s = nullptr;

	if (connection_p -> co_type == CT_WEB)
		{
			mw_client_data_p -> qcd_server_url_s = reinterpret_cast <WebConnection *> (connection_p) -> wc_uri_s;
		}

	QString title (mw_client_data_p -> qcd_dummy_arg_s);

	if (mw_client_data_p -> qcd_server_url_s)
		{
			title.append (" - ");
			title.append (mw_client_data_p -> qcd_server_url_s);
		}

	setWindowTitle (title);
	mw_client_data_p -> qcd_progress_p -> UpdateTitle ();
}


void MainWindow :: ShowProgressWindow ()
{
	mw_client_data_p -> qcd_progress_p -> show ();
	mw_client_data_p -> qcd_progress_p -> raise ();
}
