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
#ifndef PREFS_WIDGET_H
#define PREFS_WIDGET_H

#include <QList>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTabWidget>
#include <QWidget>

#include "parameter.h"

#include "jansson.h"

#include "runnable_widget.h"
#include "services_list.h"


class MainWindow;
struct QTClientData;
class ServicePrefsWidget;

/**
 * @brief The PrefsWidget class
 */
class PrefsWidget : public QWidget, public RunnableWidget
{
	Q_OBJECT

signals:
	void InterfaceLevelChanged (ParameterLevel level);
	void RunServices (bool run_flag);

public slots:
	void SetInterfaceLevel (ParameterLevel level);

public:

	/*********************/
	/***** FUNCTIONS *****/
	/*********************/

	explicit PrefsWidget (MainWindow *parent_p, ParameterLevel initial_level, bool tabbed_display_flag, QTClientData *data_p);

	~PrefsWidget ();

	ParameterLevel GetCurrentParameterLevel () const;


	void CreateAndAddServicePage (const json_t * const service_json_p, ParameterLevel level);
	void CreateAndAddServicePage (const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p, ParameterLevel level);

	bool SetServiceParams (json_t *service_config_p);

	bool SetServiceErrors (const char *service_name_s, json_t * const errors_p);

	virtual json_t *GetUserValuesAsJSON (const bool full_flag, const ParameterLevel level);

	ServicePrefsWidget *GetServicePrefsWidget (const char * const service_name_s);

	bool SelectService (const char *service_name_s, const json_t * const params_p);

	QVector <const char *> *GetActiveServices ();

	void ClearServices ();

	int GetNumServices () const;


private:

	/*********************/
	/***** VARIABLES *****/
	/*********************/

	ParameterLevel pw_level;
	ServiceUI *pw_services_ui_p;
	QList <ServicePrefsWidget *> pw_service_widgets;
	struct QTClientData *pw_data_p;
};


#endif // PREFS_WIDGET_H
