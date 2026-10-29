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
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QDialog>
#include <QStyleFactory>
#include <QDialogButtonBox>
#include <QPushButton>

#include <exception>

#include "client_ui_api.h"

#include "prefs_widget.h"
#include "results_widget.h"
#include "main_window.h"

#include "memory_allocations.h"
#include "string_utils.h"

#include "results_window.h"

#include "qt_client_data.h"
#include "progress_window.h"
#include "json_util.h"
#include "viewer_widget.h"

#ifdef _DEBUG
	#define CLIENT_UI_API_DEBUG (DEBUG_INFO)
#else
	#define CLIENT_UI_API_DEBUG (DEBUG_NONE)
#endif

static int s_dummy_argc = 1;
static char *s_dummy_args_ss [2];

static QTClientData *AllocateQTClientData (Connection *connection_p, int argc, char *argv []);

static void FreeQTClientData (QTClientData *qt_data_p);

static const char *GetQTClientName (ClientData *client_data_p);
static const char *GetQTClientDescription (ClientData *client_data_p);
static json_t *RunQTClient (ClientData *client_data_p);
static json_t *DisplayResultsInQTClient (ClientData *client_data_p, json_t *response_p);
static int AddServiceToQTClient (ClientData *client_data_p, const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t * const provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p);
static bool SetQTClientUser (ClientData *client_data_p, User *user_p);

Client *GetClient (Connection *connection_p, int argc, char *argv [])
{
	Client *client_p = nullptr;
	QTClientData *data_p = nullptr;

	try {
		data_p = AllocateQTClientData (connection_p, argc, argv);
	} catch (std::exception &e) {
		qDebug () << e.what () << Qt :: endl;
	}

	if (data_p)
		{
			client_p = static_cast <Client *> (AllocMemory (sizeof (Client)));

			if (client_p)
				{
					InitialiseClient (client_p, GetQTClientName, GetQTClientDescription, RunQTClient, DisplayResultsInQTClient, AddServiceToQTClient, SetQTClientUser, ReleaseClient, reinterpret_cast <ClientData *> (data_p), connection_p);
				}
			else
				{
					FreeMemory (data_p);
				}

		}

	return client_p;
}


bool ReleaseClient (Client *client_p)
{
	QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_p -> cl_data_p);

	//FreeClient (client_p);
	FreeQTClientData (qt_data_p);


	return true;
}


static QTClientData *AllocateQTClientData (Connection *connection_p, int argc, char *argv [])
{
	QTClientData *data_p = static_cast <QTClientData *> (AllocMemory (sizeof (QTClientData)));

	if (data_p)
		{
			/*
			 * Before Qt widgets can be created a valid QApplication
			 * must be created. This requires a valid argc, argv pair
			 * that remain in scope for the entire lifetime of the QApplication
			 * object, In addition, argc must be greater than zero and argv must
			 * contain at least one valid character string.
			 */
			data_p -> qcd_dummy_arg_s = EasyCopyToNewString ("Grassroots Client");

			if (data_p -> qcd_dummy_arg_s)
				{
					/*
					 * Ubuntu 12.04 has some theme bugs with various styles giving messages such as
					 *
					 * (client:1574): Gtk-CRITICAL **: IA__gtk_widget_style_get: assertion
					 * `GTK_IS_WIDGET (widget)' failed
					 *
					 * The solution is to use a theme that isn't broken on Ubuntu such as Plastique.
					 */
					QStyle *style_p = QStyleFactory :: create ("Fusion");
					QApplication :: setStyle (style_p);
					qDebug() << QStyleFactory::keys();


					*s_dummy_args_ss = data_p -> qcd_dummy_arg_s;
					* (s_dummy_args_ss + 1) = nullptr;

					try
						{
							data_p -> qcd_app_p = new QApplication (s_dummy_argc, s_dummy_args_ss);
						}
					catch (std :: exception &e)
						{
							qDebug () << e.what () << Qt :: endl;
						}

					data_p -> qcd_window_p = new MainWindow (data_p);

					data_p -> qcd_server_url_s = nullptr;
					if (connection_p -> co_type == CT_WEB)
						{
							WebConnection *conn_p = reinterpret_cast <WebConnection *> (connection_p);

							if (conn_p -> wc_uri_s)
								{
									data_p -> qcd_server_url_s = conn_p -> wc_uri_s;
								}
						}


					QObject :: connect (data_p -> qcd_window_p, &MainWindow :: Closed, data_p -> qcd_app_p, &QApplication :: quit);

					data_p -> qcd_results_widgets_p = new QVector <ResultsWindow *>;

					data_p -> qcd_progress_p = new ProgressWindow (data_p -> qcd_window_p, data_p);

					data_p -> qcd_viewer_widgets_p = new QVector <ViewerWidget *>;

					data_p -> qcd_init_flag = false;

					data_p -> qcd_verbose_flag = false;
				}
			else
				{
					FreeMemory (data_p);
					data_p = nullptr;
				}
		}

	return data_p;
}


static void FreeQTClientData (QTClientData *qt_data_p)
{
	delete (qt_data_p -> qcd_window_p);


	FreeCopiedString (qt_data_p -> qcd_dummy_arg_s);

	while (! (qt_data_p -> qcd_viewer_widgets_p -> isEmpty ()))
		{
			ViewerWidget *widget_p = qt_data_p -> qcd_viewer_widgets_p -> last ();
			qt_data_p -> qcd_viewer_widgets_p -> removeLast ();

			widget_p -> close ();
			delete widget_p;
		}
	delete (qt_data_p -> qcd_viewer_widgets_p);

	delete (qt_data_p -> qcd_progress_p);

	while (! (qt_data_p -> qcd_results_widgets_p -> isEmpty ()))
		{
			ResultsWindow *widget_p = qt_data_p -> qcd_results_widgets_p -> last ();
			qt_data_p -> qcd_results_widgets_p -> removeLast ();

			widget_p -> close ();
			delete widget_p;
		}
	delete (qt_data_p -> qcd_results_widgets_p);

	delete (qt_data_p -> qcd_app_p);

	FreeMemory (qt_data_p);

}


static const char *GetQTClientName (ClientData *client_data_p)
{
	return "Qt-based Grassroots client";
}


static const char *GetQTClientDescription (ClientData *client_data_p)
{
	return "A Qt-based Grassroots client user interface";
}


static json_t *RunQTClient (ClientData *client_data_p)
{
	QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_data_p);
	json_t *res_p = nullptr;
	QString s (qt_data_p -> qcd_dummy_arg_s);

	if (qt_data_p -> qcd_server_url_s)
		{
			s.append (" - ");
			s.append (qt_data_p -> qcd_server_url_s);
		}

	qt_data_p -> qcd_window_p -> setWindowTitle (s);

	qt_data_p -> qcd_window_p -> show ();
	qt_data_p -> qcd_window_p -> update ();

	if (! (qt_data_p -> qcd_init_flag))
		{
			qt_data_p -> qcd_init_flag = true;
			int res = qt_data_p -> qcd_app_p -> exec ();
		}

	return res_p;
}


static int AddServiceToQTClient (ClientData *client_data_p, const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t * const provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p)
{
	int res = 0;
	QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_data_p);

	qt_data_p -> qcd_window_p -> CreateAndAddServicePage (service_name_s, service_description_s, service_info_uri_s, service_icon_uri_s, provider_p, params_p, metadata_p);

	return res;
}



static json_t *DisplayResultsInQTClient (ClientData *client_data_p, json_t *response_p)
{
	QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_data_p);
	json_t *res_p = nullptr;

	if (qt_data_p -> qcd_verbose_flag)
		{
			PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, response_p, "response:\n");
		}

	if (! (qt_data_p -> qcd_init_flag))
		{
			qt_data_p -> qcd_window_p -> ProcessResults (response_p);
			RunQTClient (client_data_p);
		}



	return res_p;
}


static bool SetQTClientUser (ClientData *client_data_p, User *user_p)
{
	QTClientData *qt_data_p = reinterpret_cast <QTClientData *> (client_data_p);
	qt_data_p -> qcd_window_p -> SetUser (user_p);

	return true;
}
