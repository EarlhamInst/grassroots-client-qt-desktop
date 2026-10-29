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
#include <QLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QIcon>

#include "qt_client_data.h"
#include "service_prefs_widget.h"
#include "json_tools.h"
#include "json_util.h"
#include "service.h"
#include "streams.h"
#include "parameter.h"
#include "string_utils.h"


#ifdef _DEBUG
	#define SERVICE_PREFS_WIDGET_DEBUG (DEBUG_FINE)
#else
	#define SERVICE_PREFS_WIDGET_DEBUG (DEBUG_NONE)
#endif

ServicePrefsWidget::ServicePrefsWidget (const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p, QTClientData *client_data_p, QWidget *parent_p, ParameterLevel level)
: QWidget (parent_p),
	spw_service_name_s (EasyCopyToNewString (service_name_s)),
	spw_client_data_p (client_data_p),
	spw_icon_uri_s (service_icon_uri_s ? EasyCopyToNewString (service_icon_uri_s) : nullptr)
{
	QLayout *layout_p = new QVBoxLayout;
	QScrollArea *scroller_p = new QScrollArea;
	scroller_p -> setBackgroundRole (QPalette :: Button);
	scroller_p -> setSizePolicy (QSizePolicy :: Expanding, QSizePolicy :: Expanding);
	spw_params_widget_p = new QTParameterWidget (service_name_s, service_description_s, service_info_uri_s, provider_p, params_p, metadata_p, this, PL_SIMPLE, spw_client_data_p);

	spw_params_widget_p -> setSizePolicy (QSizePolicy :: Expanding, QSizePolicy :: Expanding);
	scroller_p -> setWidget (spw_params_widget_p);
	scroller_p -> setWidgetResizable (true);
	layout_p -> addWidget (scroller_p);
	//layout_p -> addWidget (spw_params_widget_p);

	QString s ("Run ");
	s.append (service_name_s);

	spw_run_service_button_p = new QCheckBox (s, this);
	spw_run_service_button_p -> setChecked (Qt :: Unchecked);
    connect (spw_run_service_button_p, &QCheckBox :: checkStateChanged, this, &ServicePrefsWidget :: SetRunFlag);

	QPushButton *reset_button_p = new QPushButton (QIcon ("images/reload"), "Restore Defaults", this);
	connect (reset_button_p, &QAbstractButton :: clicked, spw_params_widget_p, &QTParameterWidget :: ResetToDefaults);

	QVBoxLayout *buttons_layout_p = new QVBoxLayout;
	buttons_layout_p -> addWidget (spw_run_service_button_p);
	buttons_layout_p -> addWidget (reset_button_p);
	buttons_layout_p -> setAlignment (reset_button_p, Qt::AlignRight);


	layout_p -> addItem (buttons_layout_p);

	CheckInterfaceLevel (level);

	setLayout (layout_p);
}


ServicePrefsWidget :: ~ServicePrefsWidget ()
{
	delete spw_params_widget_p;

	if (spw_service_name_s)
		{
			FreeCopiedString (spw_service_name_s);
		}

	if (spw_icon_uri_s)
		{
			FreeCopiedString (spw_icon_uri_s);
		}
}


const char *ServicePrefsWidget ::GetServiceName () const
{
	return spw_service_name_s;
}


const char *ServicePrefsWidget :: GetServiceIconUri () const
{
	return spw_icon_uri_s;
}


void ServicePrefsWidget :: ToggleRunFlag ()
{
	if (GetRunFlag ())
		{
			spw_run_service_button_p -> setCheckState (Qt :: Unchecked);
		}
	else
		{
			spw_run_service_button_p -> setCheckState (Qt :: Checked);
		}
}


void ServicePrefsWidget :: SetRunFlag (bool run_flag)
{
    const Qt :: CheckState state = run_flag ? Qt :: Checked : Qt :: Unchecked;
    spw_run_service_button_p -> setCheckState (state);
    emit RunStatusChanged (spw_service_name_s, run_flag);
}


bool ServicePrefsWidget :: GetRunFlag () const
{
	return (spw_run_service_button_p -> checkState() == Qt :: Checked);
}


json_t *ServicePrefsWidget :: GetServiceParamsAsJSON (bool full_flag, const ParameterLevel level) const
{
	json_t *res_p = nullptr;

	if (GetRunFlag ())
		{
			json_t *params_json_p  = spw_params_widget_p -> GetParameterValuesAsJSON ();

			if (params_json_p)
				{
					const SchemaVersion *sv_p = spw_client_data_p -> qcd_base_data.cd_schema_p;

					res_p = GetServiceRunRequestFromJSON (spw_service_name_s, params_json_p, sv_p, true, level);

					if (!res_p)
						{
							json_decref (params_json_p);
						}
				}
		}

	return res_p;
}



bool ServicePrefsWidget :: SetServiceParams (const json_t *service_config_p)
{
	bool success_flag = true;
	const json_t *json_p = json_object_get (service_config_p, SERVICE_RUN_S);

	if (json_p && (json_is_true (json_p)))
		{
			SetRunFlag (true);
		}
	else
		{
			SetRunFlag (false);
		}

	/* Set the params */
	json_p = json_object_get (service_config_p, PARAM_SET_KEY_S);

	if (json_p)
		{
			success_flag = spw_params_widget_p -> SetParamValuesFromJSON (json_p);
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, json_p, "Failed to get child %s from json", PARAM_SET_KEY_S);
		}

	return success_flag;
}


bool ServicePrefsWidget :: SetServiceErrors (json_t * const errors_p)
{
	bool success_flag = false;

	if (json_is_object (errors_p))
		{
			json_t *error_p;
			const char *param_name_s;

			json_object_foreach (errors_p, param_name_s, error_p)
				{
					if (strcmp (param_name_s, JOB_ERROR_S) == 0)
						{
							/*
							 * General runtime errors
							 */
						}
					else
						{
							BaseParamWidget *widget_p = spw_params_widget_p -> GetWidgetForParameter (param_name_s);

							if (widget_p)
								{
									json_t *errors_array_p = json_object_get (error_p, JOB_ERRORS_S);

									if (errors_array_p)
										{
											widget_p -> SetErrors (errors_array_p);
										}
								}
						}
				}		/* json_array_foreach (json_p, i, param_p) */


		}		/* if (json_is_array (json_p)) */


	return success_flag;
}



void ServicePrefsWidget :: CheckInterfaceLevel (ParameterLevel level)
{
	spw_params_widget_p -> UpdateParameterLevel (level, this);
}


