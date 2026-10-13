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
#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QTimer>
#include <QVector>

#include "progress_widget.h"
#include "jansson.h"

struct QTClientData;

class ProgressWindow : public QWidget
{
public:
	ProgressWindow (QMainWindow *parent_p, QTClientData *data_p);
	~ProgressWindow ();

	bool AddProgressItemFromJSON (const json_t *json_p, const char * const service_name_s, const char * const service_description_s, const char * const service_uri_s);

	bool RemoveProgressWidget (ProgressWidget *widget_p);

	json_t *BuildResultsRequest ();


	void RefreshStatuses (ProgressWidget **widgets_pp, const size_t num_widgets);

	bool HasProgressItems () const;

	void ClearProgressItems ();

	void UpdateTitle ();

public slots:
	void UpdateStatuses ();
	void ViewResults ();
	void show ();

private:
	QTimer *pw_timer_p;
	bool pw_timer_started_flag;
	QVector <ProgressWidget *> pw_widgets;
	QTClientData *pw_data_p;
	QPushButton *pw_results_button_p;
};

#endif // PROGRESS_WINDOW_H
