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
#include <iostream>

#include <QLayout>
#include <QGroupBox>
#include <QAction>
#include <QCheckBox>
#include <QMenuBar>
#include <QMenu>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QString>
#include <QTabWidget>
#include <QVector>

#include "prefs_widget.h"
#include "main_window.h"

#include "file_chooser_widget.h"
#include "qt_parameter_widget.h"
#include "services_list.h"
#include "services_tabs.h"
#include "qt_client_data.h"

#include "json_tools.h"
#include "string_utils.h"
#include "service_metadata.h"
#include "json_util.h"

using namespace std;



PrefsWidget :: PrefsWidget (MainWindow *parent_p,  ParameterLevel initial_level, const bool tabbed_display_flag, QTClientData *data_p)
:	QWidget (parent_p),
	pw_level (initial_level),
	pw_data_p (data_p)
{
	QVBoxLayout *layout_p = new QVBoxLayout;

	if (tabbed_display_flag)
		{
			pw_services_ui_p = new ServicesTabs (this);
		}
	else
		{
			pw_services_ui_p = new ServicesList (this);
		}

	layout_p -> addWidget (pw_services_ui_p -> GetWidget ());

	setLayout (layout_p);
}


PrefsWidget :: ~PrefsWidget ()
{
}


bool PrefsWidget :: SelectService (const char *service_name_s, const json_t * const params_p)
{
	bool success_flag = true;

	pw_services_ui_p -> UpdateServicePrefs (service_name_s, params_p);

	return success_flag;
}


void PrefsWidget :: CreateAndAddServicePage (const json_t * const service_json_p, ParameterLevel level)
{
	if (pw_data_p -> qcd_verbose_flag)
		{
			PrintJSONToLog (STM_LEVEL_INFO, __FILE__, __LINE__, service_json_p, "Creating service page from: ");
		}

	const char *service_name_s = GetServiceNameFromJSON (service_json_p);

	if (service_name_s)
		{
			const char *service_description_s = GetServiceDescriptionFromJSON (service_json_p);

			if (service_description_s)
				{
					ParameterSet *params_p = CreateParameterSetFromJSON (service_json_p, NULL, true);

					if (params_p)
						{
							const char *service_info_uri_s = GetOperationInformationURIFromJSON (service_json_p);
							const char *service_icon_uri_s = GetOperationIconURIFromJSON (service_json_p);
							const json_t *provider_p = GetProviderFromServiceJSON (service_json_p);
							ServiceMetadata *metadata_p = GetServiceMetadataFromJSON (service_json_p);


							CreateAndAddServicePage (service_name_s, service_description_s, service_info_uri_s, service_icon_uri_s, provider_p, params_p, metadata_p, level);
						}		/* if (params_p) */

				}		/* if (service_description_s) */

		}		/* if (service_name_s) */
}


void PrefsWidget :: CreateAndAddServicePage (const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p, ParameterLevel level)
{
	ServicePrefsWidget *service_widget_p = new ServicePrefsWidget (service_name_s, service_description_s, service_info_uri_s, service_icon_uri_s, provider_p, params_p, metadata_p, this -> pw_data_p, this, level);

	pw_services_ui_p -> AddService (service_name_s, service_widget_p);
	pw_service_widgets.append (service_widget_p);

	connect (this, &PrefsWidget :: InterfaceLevelChanged, service_widget_p, &ServicePrefsWidget :: CheckInterfaceLevel);
}


void PrefsWidget :: ClearServices ()
{
	pw_services_ui_p -> ClearServices ();

	qDeleteAll (pw_service_widgets);
	pw_service_widgets.clear ();
}


int PrefsWidget :: GetNumServices () const
{
	return pw_service_widgets.size ();
}


bool PrefsWidget :: SetServiceParams (json_t *services_config_p)
{
	bool success_flag = false;

	if (json_is_array (services_config_p))
		{
			json_t *service_config_p;
			size_t i;

			json_array_foreach (services_config_p, i, service_config_p)
				{
					const char *service_name_s = GetJSONString (service_config_p, OPERATION_ID_S);

					if (service_name_s)
						{
							ServicePrefsWidget *service_widget_p = GetServicePrefsWidget (service_name_s);

							if (service_widget_p)
								{
									service_widget_p -> SetServiceParams (service_config_p);
								}		/* if (service_widget_p) */

						}		/* if (service_name_s) */
				}

			success_flag = true;
		}

	return success_flag;

}


ServicePrefsWidget *PrefsWidget :: GetServicePrefsWidget (const char * const service_name_s)
{
	ServicePrefsWidget *service_widget_p = 0;

	/* find the service widget */
	for (int i = pw_service_widgets.size () - 1; i >= 0; -- i)
		{
			ServicePrefsWidget *widget_p = pw_service_widgets.at (i);

			if (strcmp (widget_p -> GetServiceName (), service_name_s) == 0)
				{
					service_widget_p = widget_p;
					i = -1;		// force exit from loop
				}
		}

	return service_widget_p;
}


bool PrefsWidget :: SetServiceErrors (const char *service_name_s, json_t * const errors_p)
{
	bool success_flag = false;
	ServicePrefsWidget *service_widget_p = GetServicePrefsWidget (service_name_s);

	if (service_widget_p)
		{
			success_flag = service_widget_p -> SetServiceErrors (errors_p);
		}

	return success_flag;
}



ParameterLevel PrefsWidget :: GetCurrentParameterLevel () const
{
	return pw_level;
}


void PrefsWidget :: SetInterfaceLevel (ParameterLevel level)
{
	if (pw_level != level)
		{
			emit InterfaceLevelChanged (level);
			pw_level = level;
		}		/* if (pw_level != level) */
}


json_t *PrefsWidget :: GetUserValuesAsJSON (const bool full_flag, const ParameterLevel level)
{
	json_t *root_p = json_array ();

	if (root_p)
		{
			const int num_services = pw_service_widgets.size ();
			int i = 0;
			bool success_flag = true;

			char *dump_s = json_dumps (root_p, 0);

			if (dump_s)
				{
					free (dump_s);
				}

			while (success_flag && (i < num_services))
				{
					ServicePrefsWidget *spw_p = pw_service_widgets.at (i);

					if (spw_p -> GetRunFlag ())
						{
							json_t *service_json_p = spw_p -> GetServiceParamsAsJSON (full_flag, level);

							if (service_json_p)
								{
									success_flag = (json_array_append_new (root_p, service_json_p) == 0);
								}
							else
								{
									PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to get params JSON for %s", spw_p ->  GetServiceName());
									success_flag = false;
								}
						}

					if (success_flag)
						{
							++ i;
						}
				}

			if (!success_flag)
				{
					json_decref (root_p);
					return nullptr;
				}

		}		/* if (root_p) */

	char *client_results_s = json_dumps (root_p, 0);
	size_t l = json_array_size (root_p);
	for (size_t i = 0; i < l; ++ i)
		{
			json_t *json_p = json_array_get (root_p, i);
			char *dump_s = json_dumps (json_p, 0);

			if (dump_s)
				{
					free (dump_s);
				}
		}

	if (client_results_s)
		{
			free (client_results_s);
		}

	return root_p;
}


QVector <const char *> *PrefsWidget :: GetActiveServices ()
{
	QVector <const char *> *active_services_p = new QVector <const char *> ();

	if (active_services_p)
		{
			const int num_services = pw_service_widgets.size ();
			int i = 0;
			bool success_flag = true;

			while (success_flag && (i < num_services))
				{
					ServicePrefsWidget *spw_p = pw_service_widgets.at (i);

					if (spw_p -> GetRunFlag ())
						{
							const char *name_s = spw_p -> GetServiceName ();
							active_services_p -> push_back (name_s);

							qDebug () << "added: " << name_s << Qt :: endl;

						}

					++ i;
				}


		}		/* if (root_p) */

	return active_services_p;
}



