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
#ifndef SERVICES_TABS_H
#define SERVICES_TABS_H

#include <QTabWidget>

#include "service_ui.h"


class ServicesTabs : public QTabWidget, public ServiceUI
{
public:
  ServicesTabs (QWidget *parent_p = 0);

  virtual void AddService (const char * const service_name_s, ServicePrefsWidget *service_widget_p);
  virtual QWidget *GetWidget ();
	virtual void UpdateServicePrefs (const char *service_name_s, const json_t *params_json_p);
	virtual void ClearServices ();

public slots:
  void SelectService (const char *service_name_s, const json_t *params_json_p);


};

#endif // SERVICES_TABS_H

