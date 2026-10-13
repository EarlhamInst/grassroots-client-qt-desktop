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
#include "services_tabs.h"

#include "filesystem_utils.h"
#include "string_utils.h"


ServicesTabs :: ServicesTabs (QWidget *parent_p)
: QTabWidget (parent_p)
{

}


void ServicesTabs :: SelectService (const char *service_name_s, const json_t *params_json_p)
{

}


void ServicesTabs :: AddService (const char * const service_name_s, ServicePrefsWidget *service_widget_p)
{
	const char * const icon_path_s = service_widget_p -> GetServiceIconUri ();
	QString service_name (service_name_s);

	if (icon_path_s)
		{
			QIcon icon (icon_path_s);

			addTab (service_widget_p, icon, service_name);
		}
	else
		{
			addTab (service_widget_p, service_name);
		}
}


QWidget *ServicesTabs :: GetWidget ()
{
	return this;
}


void ServicesTabs :: UpdateServicePrefs (const char *service_name_s, const json_t *params_json_p)
{

}


void ServicesTabs :: ClearServices ()
{
	/* This removes the tabs but does not delete their widgets */
	clear ();
}
