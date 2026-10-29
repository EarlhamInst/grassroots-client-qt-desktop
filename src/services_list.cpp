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
#include <QGroupBox>
#include <QSplitter>

#include "services_list.h"

#include "filesystem_utils.h"
#include "string_utils.h"
#include "curl_tools.h"
#include "ui_utils.h"


ServicesList :: ServicesList (QWidget *parent_p)
: QListWidget (parent_p)
{
  sl_stacked_widgets_p = new QStackedWidget (this);
  sl_services_p = new QListWidget (this);

  connect (sl_services_p, &QAbstractItemView :: clicked, this, &ServicesList :: SetCurrentService);
  connect (sl_services_p, &QAbstractItemView :: doubleClicked, this, &ServicesList :: ToggleServiceRunStatus);
  connect (sl_services_p, &QListWidget :: itemChanged, this, &ServicesList :: CheckServiceRunStatus);

  QGroupBox *box_p = new QGroupBox (tr ("Services"));
  box_p -> setAlignment (Qt :: AlignCenter);
  QHBoxLayout *services_layout_p = new QHBoxLayout;

  services_layout_p -> addWidget (sl_services_p);
  box_p -> setLayout (services_layout_p);


	QHBoxLayout *layout_p = new QHBoxLayout;

	/*
	layout_p -> addWidget (box_p);
	layout_p -> addWidget (sl_stacked_widgets_p);
	*/

	QSplitter *splitter_p = new QSplitter (this);
	splitter_p -> setHandleWidth (2);
	splitter_p -> addWidget (box_p);
	splitter_p -> addWidget (sl_stacked_widgets_p);
	layout_p -> addWidget (splitter_p);

  setLayout (layout_p);

	QSizePolicy p;
	p.setHorizontalPolicy (QSizePolicy :: MinimumExpanding);
	p.setVerticalPolicy (QSizePolicy :: MinimumExpanding);
	sl_services_p -> setSizePolicy (p);
}



void ServicesList :: SetCurrentService (const QModelIndex &index_r)
{
  const int row = index_r.row ();
  sl_stacked_widgets_p -> setCurrentIndex (row);
}


void ServicesList :: ToggleServiceRunStatus (const QModelIndex &index_r)
{
  int row = index_r.row ();
  QListWidgetItem *list_item_p = sl_services_p -> item (row);

  CheckServiceRunStatus (list_item_p);
}


void ServicesList :: CheckServiceRunStatus (const QListWidgetItem *item_p)
{
  int row = sl_services_p -> row (item_p);

  ServicePrefsWidget *widget_p = static_cast <ServicePrefsWidget *> (sl_stacked_widgets_p -> widget (row));
  widget_p -> SetRunFlag (item_p -> checkState ());
}


void ServicesList :: AddService (const char * const service_name_s, ServicePrefsWidget *services_widget_p)
{
	const char * const icon_path_s = services_widget_p -> GetServiceIconUri ();
	QString service_name (service_name_s);
	ServicesListItem *item_p = nullptr;

	if (icon_path_s)
		{
			QIcon *icon_p = UIUtils :: GetRemoteIconImage (icon_path_s);
			if (icon_p)
				{
					item_p = new ServicesListItem (*icon_p, service_name, sl_services_p);
					delete icon_p;
				}
			else
				{
					item_p = new ServicesListItem (service_name, sl_services_p);
				}
		}
	else
		{
			item_p = new ServicesListItem (service_name, sl_services_p);
		}

	sl_services_p -> addItem (item_p);
	int index = sl_stacked_widgets_p -> addWidget (services_widget_p);

	item_p -> setCheckState (Qt :: Unchecked);
	item_p -> setFlags (Qt :: ItemIsEnabled | Qt :: ItemIsUserCheckable | Qt :: ItemIsSelectable);

	connect (services_widget_p, &ServicePrefsWidget :: RunStatusChanged, this, &ServicesList :: SetServiceRunStatus);
}


QWidget *ServicesList :: GetWidget ()
{
	return this;
}


void ServicesList :: UpdateServicePrefs (const char *service_name_s, const json_t *params_json_p)
{
	QListWidgetItem *item_p = FindListWidgetItem (service_name_s);

	if (item_p)
		{
			int row = sl_services_p -> row (item_p);

			if (row >= 0)
				{
					if (row < sl_stacked_widgets_p -> count ())
						{
							ServicePrefsWidget *service_prefs_p = static_cast <ServicePrefsWidget *> (sl_stacked_widgets_p -> widget (row));
							const char *widget_name_s = service_prefs_p -> GetServiceName ();

							if (strcmp (widget_name_s, service_name_s) == 0)
								{
									item_p -> setSelected (true);
									sl_stacked_widgets_p -> setCurrentIndex (row);

									if (! (service_prefs_p -> SetServiceParams (params_json_p)))
										{

										}
								}
						}
				}
		}
}


void ServicesList :: ClearServices ()
{
	sl_services_p -> clear ();

	while (sl_stacked_widgets_p -> count () > 0)
		{
			sl_stacked_widgets_p -> removeWidget (sl_stacked_widgets_p -> widget (0));
		}
}


QListWidgetItem *ServicesList :: FindListWidgetItem (const char *service_name_s)
{
	QList <QListWidgetItem *> widgets = sl_services_p -> findItems (service_name_s, Qt :: MatchExactly);

	if (widgets.size () == 1)
		{
			return widgets.at (0);
		}

	return 0;
}


void ServicesList :: SetServiceRunStatus (const char * const service_name_s, bool state)
{
	QListWidgetItem *item_p = nullptr;

	if (service_name_s)
		{
			item_p = FindListWidgetItem (service_name_s);

			if (item_p)
				{
					item_p -> setCheckState (state ? Qt :: Checked : Qt :: Unchecked);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ServicesList :: SetServiceRunStatus (): unknown service \"%s\"", service_name_s);

				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ServicesList :: SetServiceRunStatus (): No service name");
		}
}


QSize ServicesList :: sizeHint () const
{
	QSize s = QListWidget :: sizeHint ();

//	int w = s.width ();
//	s.setWidth (w + 32);

	return s;
}


ServicesListItem :: ServicesListItem (const QIcon &icon_r, const QString &service_name_r, QListWidget *list_p)
: QListWidgetItem (icon_r, service_name_r, list_p)
{}


ServicesListItem :: ServicesListItem (const QString &service_name_r, QListWidget *list_p)
: QListWidgetItem (service_name_r, list_p)
{}


QSize ServicesListItem :: sizeHint () const
{
	QSize original_size = QListWidgetItem :: sizeHint ();
/*
	int width = original_size.width ();

	original_size.setHeight (width + 32);
*/
	return original_size;
}

/*
void ServicesListItem :: SetRunStatus (bool state)
{
	setCheckState (state ? Qt :: Checked : Qt :: Unchecked);
}
*/
