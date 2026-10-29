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

/**
 * @file
 * @brief
 */
#ifndef SERVICE_PREFS_WIDGET_H
#define SERVICE_PREFS_WIDGET_H

#include <QWidget>
#include <QCheckBox>

#include "parameter_set.h"
#include "qt_parameter_widget.h"


#include "jansson.h"
#include "service_metadata.h"


// forward declarations
struct QTClientData;

class ServicePrefsWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ServicePrefsWidget (const char * const service_name_s, const char * const service_description_s, const char * const service_info_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p, QTClientData *data_p, QWidget *parent_p, ParameterLevel level);
	~ServicePrefsWidget ();

	bool GetRunFlag () const;

	json_t *GetServiceParamsAsJSON (bool full_flag, const ParameterLevel level) const;

	const char *GetServiceName () const;

	const char *GetServiceIconUri () const;

	bool SetServiceParams (const json_t *service_config_p);

	void CheckInterfaceLevel (ParameterLevel level);

	bool SetServiceErrors (json_t * const errors_p);


signals:
	void RunStatusChanged (const char * const service_name_s, bool status);


public slots:
    void SetRunFlag (bool run_flag);
	void ToggleRunFlag ();

private:
	QTParameterWidget *spw_params_widget_p;
	char *spw_service_name_s;
	QCheckBox *spw_run_service_button_p;
	const struct QTClientData *spw_client_data_p;
	char *spw_icon_uri_s;
};

#endif // SERVICE_PREFS_WIDGET_H
