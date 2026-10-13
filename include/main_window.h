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
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>

#include "keyword_widget.h"
#include "prefs_widget.h"

#include "service_metadata.h"
#include "user_details.h"


struct QTClientData;
struct Connection;
class QThread;
class QToolBar;

class MainWindow : public QMainWindow
{
	Q_OBJECT

signals:
	void Closed ();
	void ServiceRequested (const char *service_name_s, const json_t *params_json_p);

public slots:
	void SetSimpleInterfaceLevel ();
	void SetAdvancedInterfaceLevel ();
	void SetWizardInterfaceLevel ();
	void LoadConfiguration ();
	void SaveConfiguration ();
	void RunServices (bool run_flag);
	void SelectService (const char *service_name_s, const json_t *params_json_p);

	void RunService (json_t *service_p);

	void GetIndexingData ();


	void RefreshServices ();
	void GetInterestedServices ();
	void GetNamedServices ();
	void ConnectToServer ();
	void ShowProgressWindow ();

public:
	MainWindow (struct QTClientData *data_p);
	virtual ~MainWindow ();

	void CreateAndAddServicePage (const char * const service_name_s, const char * const service_description_s, const char * const service_info_uri_s, const char * const service_icon_uri_s, const json_t *provider_p, ParameterSet *params_p, ServiceMetadata *metadata_p);


	json_t *GetUserValuesAsJSON (bool all_flag) const;

	void ProcessResults (json_t *services_json_p);

	void SetUser (User *user_p);

protected:
	virtual void dropEvent (QDropEvent *event_p);
	virtual void dragEnterEvent (QDragEnterEvent *event_p);
	virtual void closeEvent (QCloseEvent *event_p);

	void LoadConfigurationFile (QString &filename_r);


private:
	PrefsWidget *mw_prefs_widget_p;
	KeywordWidget *mw_keyword_widget_p;
	struct QTClientData *mw_client_data_p;
	QTabWidget *mw_tabs_p;
	ParameterLevel mw_current_level;
	QLabel *mw_user_label_p;
	QToolBar *mw_toolbar_p;
	QAction *mw_refresh_action_p;
	QAction *mw_connect_action_p;

	/** The thread used to get the services from the server, NULL if not running */
	QThread *mw_fetch_thread_p;

	/** The response from the server, set by mw_fetch_thread_p */
	json_t *mw_fetch_response_p;

	/** The Connection to a new server that we are trying to switch to */
	struct Connection *mw_pending_connection_p;

	void AddActions ();
	void SetParams (json_t *config_p);

	//bool AddResults (const json_t *job_results_p);
	void SetParameterLevel (ParameterLevel pl);
	void UseConnection (struct Connection *connection_p);
	void FetchServices (struct Connection *connection_p);
	void ProcessFetchedServices (json_t *response_p, struct Connection *connection_p);
	void SetFetchingServices (const bool fetching_flag, const QString &url_r = QString ());
	bool IsFetchingServices () const;

private slots:
	void Run ();
	void Reject ();

};


#endif		/* #ifndef EPR_WINDOW_H */
