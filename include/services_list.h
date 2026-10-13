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
#ifndef SERVICES_LIST_H
#define SERVICES_LIST_H

#include <QListWidget>
#include <QStackedWidget>
#include <QStyledItemDelegate>

#include "service_ui.h"

class ServicesList : public QListWidget, public ServiceUI
{
  Q_OBJECT

public:
  ServicesList (QWidget *parent_p = 0);

  virtual void AddService (const char * const service_name_s, ServicePrefsWidget *services_widget_p);
  virtual QWidget *GetWidget ();
	virtual void UpdateServicePrefs (const char *service_name_s, const json_t *params_json_p);
	virtual void ClearServices ();

	virtual QSize sizeHint () const;


public slots:
  //void SelectService (const char *service_name_s, const json_t *params_json_p);

private slots:
  void ToggleServiceRunStatus (const QModelIndex &index_r);
  void CheckServiceRunStatus (const QListWidgetItem *item_p);
  void SetCurrentService (const QModelIndex &index_r);
  void SetServiceRunStatus (const char * const service_name_s, bool state);

private:
  QStackedWidget *sl_stacked_widgets_p;
  QListWidget *sl_services_p;

	QListWidgetItem *FindListWidgetItem (const char *service_name_s);
};



class ServicesListItem : public QListWidgetItem, public QObject
{


public:
  ServicesListItem (const QIcon &icon_r, const QString &service_name_r, QListWidget *list_p);
  ServicesListItem (const QString &service_name_r, QListWidget *list_p);

  virtual QSize sizeHint () const;

  /*
public slots:
  void SetRunStatus (bool state);
  */
};


#endif // SERVICES_LIST_H

