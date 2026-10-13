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
#include <QVBoxLayout>
#include <QPushButton>


#include "progress_widget.h"
#include "progress_window.h"
#include "json_tools.h"
#include "json_util.h"
#include "memory_allocations.h"
#include "qt_client_data.h"



ProgressWindow :: ProgressWindow (QMainWindow *parent_p, QTClientData *data_p)
:	pw_data_p (data_p)
{

	UpdateTitle ();
	setWindowIcon (QIcon ("images/progress"));

	//pw_timer_p = new QTimer (this);
	// connect (pw_timer_p, &QTimer :: timeout, this, &ProgressWindow :: UpdateStatuses);

	pw_timer_started_flag = false;
	pw_timer_p = 0;

	pw_results_button_p = new QPushButton (QIcon ("images/go"), tr ("Refresh all jobs"));
	connect (pw_results_button_p, &QPushButton :: clicked, this, &ProgressWindow :: ViewResults);
	pw_results_button_p -> setEnabled (false);

	QHBoxLayout *buttons_layout_p = new QHBoxLayout;
	buttons_layout_p -> addWidget (pw_results_button_p);


	QVBoxLayout *layout_p = new QVBoxLayout;

	setLayout (layout_p);
	layout_p -> addLayout (buttons_layout_p);
}


ProgressWindow ::	~ProgressWindow ()
{
}



void ProgressWindow :: UpdateTitle ()
{
	QString s (tr ("Jobs Progress"));
	s.append (" - ");
	s.append (pw_data_p -> qcd_server_url_s);

	setWindowTitle (s);
}


bool ProgressWindow :: HasProgressItems () const
{
	return !pw_widgets.isEmpty ();
}


void ProgressWindow :: ClearProgressItems ()
{
	while (!pw_widgets.isEmpty ())
		{
			RemoveProgressWidget (pw_widgets.last ());
		}
}


void ProgressWindow :: show ()
{
	QWidget :: show ();
}


bool ProgressWindow :: AddProgressItemFromJSON (const json_t *json_p, const char * const service_name_s, const char * const service_description_s, const char * const service_uri_s)
{
	bool success_flag = false;
	ProgressWidget *widget_p = ProgressWidget :: CreateProgressWidgetFromJSON (json_p, this, service_name_s, service_description_s, service_uri_s);

	if (widget_p)
		{
			pw_widgets.append (widget_p);
			layout () -> addWidget (widget_p);


			if (! (pw_results_button_p -> isEnabled ()))
				{
					pw_results_button_p -> setEnabled (true);
				}


			OperationStatus status;

			if (GetStatusFromJSON (json_p, &status))
				{
					switch (status)
					{
						case OS_PENDING:
						case OS_STARTED:
						case OS_FINISHED:
							//pw_check_button_p -> setEnabled (true);
							pw_results_button_p -> setEnabled (true);
							break;

						default:
							break;
					}
				}

			success_flag = true;
		}

	if (pw_timer_started_flag == false)
		{
			//			pw_timer_p -> start (5000);
			pw_timer_started_flag = true;
		}


	return success_flag;
}


bool ProgressWindow :: RemoveProgressWidget (ProgressWidget *widget_p)
{
	bool success_flag = false;
	int index = -1;
	const int last_index = pw_widgets.size () - 1;

	for (int i = last_index; i >= 0; -- i)
		{
			if (pw_widgets.at (i) == widget_p)
				{
					index = i;
					i = -1;		// force exit from loop
				}
		}

	if (index != -1)
		{
			pw_widgets.remove (index);
			layout () -> removeWidget (widget_p);
			delete widget_p;
			update ();
			success_flag = true;

			if (last_index == 0)
				{
					/*
					 * We've removed the last ProgressWidget
					 * so disable the refresh button.
					 */

					pw_results_button_p -> setEnabled (false);
				}
		}



	return success_flag;
}


void ProgressWindow :: UpdateStatuses ()
{
	json_t *req_p = 0;
	const size_t size = pw_widgets.size ();
	const uuid_t **ids_pp = (const uuid_t **) AllocMemoryArray (size, sizeof (const uuid_t *));

	if (ids_pp)
		{
			Connection *connection_p = pw_data_p -> qcd_base_data.cd_connection_p;
			const SchemaVersion *schema_p = pw_data_p -> qcd_base_data.cd_schema_p;

			for (size_t i = 0; i < size; ++ i)
				{
					* (ids_pp + i) = pw_widgets.at (i) -> GetUUID ();
				}

			req_p = GetServicesResultsRequest (ids_pp, size, connection_p, schema_p);

			if (req_p)
				{
					json_t *statuses_json_p = MakeRemoteJsonCall (req_p, connection_p);

					if (statuses_json_p)
						{
							json_t *services_json_p = json_object_get (statuses_json_p, SERVICES_NAME_S);

							if (pw_data_p -> qcd_verbose_flag)
								{
									PrintJSONToLog (STM_LEVEL_FINER, __FILE__, __LINE__, statuses_json_p, "results");
								}

							if (services_json_p)
								{
									if (json_is_array (services_json_p))
										{
											const size_t num_services = json_array_size (services_json_p);
											size_t i;
											json_t *service_json_p;

											json_array_foreach (services_json_p, i, service_json_p)
											{
												json_t *uuid_json_p = json_object_get (service_json_p, SERVICE_UUID_S);

												if (uuid_json_p)
													{
														if (json_is_string (uuid_json_p))
															{
																const char *uuid_s = json_string_value (uuid_json_p);
																uuid_t uuid;

                                                                if (uuid_parse (const_cast <char *> (uuid_s), uuid) == 0)
																	{
																		size_t j = i;
																		ProgressWidget *progress_widget_p = 0;

																		while ((progress_widget_p == 0) && (j < num_services))
																			{
																				ProgressWidget *widget_p = pw_widgets.at (j);
																				const uuid_t *id_p = widget_p -> GetUUID ();

																				if (uuid_compare (*id_p, uuid) == 0)
																					{
																						progress_widget_p = widget_p;
																					}
																				else
																					{
																						++ j;
																					}
																			}

																		if (!progress_widget_p)
																			{
																				j = 0;

																				while ((progress_widget_p == 0) && (j < i))
																					{
																						ProgressWidget *widget_p = pw_widgets.at (j);
																						const uuid_t *id_p = widget_p -> GetUUID ();

																						if (uuid_compare (*id_p, uuid) == 0)
																							{
																								progress_widget_p = widget_p;
																							}
																						else
																							{
																								++ j;
																							}
																					}
																			}

																		if (progress_widget_p)
																			{
																				OperationStatus status;

																				if (GetStatusFromJSON (service_json_p, &status))
																					{
																						progress_widget_p -> SetStatus (status);
																					}

																			}		/* if (progress_widget_p) */
																	}
															}

													}		/* if (uuid_json_p) */



											}		/* for (size_t i = 0; i < num_services; ++ i) */

										}		/* if (json_is_array (services_json_p)) */


								}		/* if (services_json_p) */

						}		/* if (statuses_json_p) */

				}		/* if (req_p) */

				FreeMemory (ids_pp);
		}		/* if (ids_pp) */

}



void ProgressWindow :: ViewResults ()
{
	ProgressWidget **widgets_pp = pw_widgets.data ();
	RefreshStatuses (widgets_pp, pw_widgets.size ());
}


ProgressWidget *FindProgressWidgetByUUID (ProgressWidget **widgets_pp, const size_t num_widgets, const json_t *service_json_p)
{
	json_t *uuid_json_p = json_object_get (service_json_p, JOB_UUID_S);

	if (uuid_json_p)
		{
			if (json_is_string (uuid_json_p))
				{
					const char *uuid_s = json_string_value (uuid_json_p);
					uuid_t uuid;

                    if (uuid_parse (const_cast <char *> (uuid_s), uuid) == 0)
						{
							for (size_t i = 0; i < num_widgets; ++ i)
								{
									ProgressWidget *widget_p = * (widgets_pp + i);
									const uuid_t *id_p = widget_p -> GetUUID ();

									if (uuid_compare (*id_p, uuid) == 0)
										{
											return widget_p;
										}
								}
						}
				}
		}

	return 0;
}


void ProgressWindow :: RefreshStatuses (ProgressWidget **widgets_pp, const size_t num_widgets)
{
	size_t num_ids = 0;

	for (size_t i = 0; i < num_widgets; ++ i)
		{
			OperationStatus status = (* (widgets_pp + i)) -> GetCurrentStatus ();

			if (status == OS_STARTED || status == OS_PENDING)
				{
					++ num_ids;
				}
		}

	if (num_ids > 0)
		{
			const uuid_t **ids_pp = (const uuid_t **) AllocMemoryArray (num_ids, sizeof (const uuid_t *));

			if (ids_pp)
				{
					const uuid_t **id_pp = ids_pp;

					for (size_t i = 0; i < num_widgets; ++ i)
						{
							OperationStatus status = (* (widgets_pp + i)) -> GetCurrentStatus ();

							if (status == OS_STARTED || status == OS_PENDING)
								{
									const uuid_t *id_p = (* (widgets_pp + i)) -> GetUUID ();

									*id_pp = id_p;
									++ id_pp;
								}
						}

					Connection *connection_p = pw_data_p -> qcd_base_data.cd_connection_p;
					const SchemaVersion *schema_p = pw_data_p -> qcd_base_data.cd_schema_p;

					json_t *req_p = GetServicesResultsRequest (ids_pp, num_ids, connection_p, schema_p);

					if (req_p)
						{
							json_t *results_json_p = NULL;

							if (pw_data_p -> qcd_verbose_flag)
								{
									PrintJSONToLog (STM_LEVEL_FINER, __FILE__, __LINE__, req_p, "req:");
								}


							results_json_p = MakeRemoteJsonCall (req_p, connection_p);

							if (results_json_p)
								{
									if (pw_data_p -> qcd_verbose_flag)
										{
											PrintJSONToLog (STM_LEVEL_FINER, __FILE__, __LINE__, results_json_p, "results");
										}

									if (json_is_array (results_json_p))
										{
											ResultsWindow *results_p = GetNewResultsWindow (pw_data_p);
											bool show_results_flag = false;

											size_t i;
											json_t *job_p;

											json_array_foreach (results_json_p, i, job_p)
											{
												const char *service_name_s = GetJSONString (job_p, SERVICE_NAME_S);
												const char *service_description_s = GetJSONString (job_p, OPERATION_DESCRIPTION_S);
												const char *service_uri_s =  GetJSONString (job_p, OPERATION_INFORMATION_URI_S);

												/* Get the job status */
												OperationStatus status = OS_ERROR;
												const char *value_s = GetJSONString (job_p, SERVICE_STATUS_S);

												if (value_s)
													{
														status = GetOperationStatusFromString (value_s);
													}
												else
													{
														json_int_t k;
														/* Get the job status */

														if (GetJSONInteger(job_p, SERVICE_STATUS_VALUE_S, &k))
															{
																if ((k > OS_LOWER_LIMIT) && (k < OS_UPPER_LIMIT))
																	{
																		status = (OperationStatus) k;
																	}
															}
													}


												ProgressWidget *widget_p = FindProgressWidgetByUUID (widgets_pp, num_widgets, job_p);

												if (widget_p)
													{
														widget_p -> SetStatus (status);

														if ((status == OS_SUCCEEDED) || (status == OS_PARTIALLY_SUCCEEDED))
															{
																//widget_p -> CacheResults (job_p);
																results_p -> AddAllResultsPagesFromJSON (job_p, service_name_s, service_description_s, service_uri_s);
																show_results_flag = true;
															}
													}
											}

											if (show_results_flag)
												{
													results_p -> show ();
												}

										}

									json_decref (results_json_p);
								}		/* if (statuses_json_p) */

								json_decref (req_p);
						}		/* if (req_p) */

						FreeMemory (ids_pp);
				}

		}

}



json_t *ProgressWindow :: BuildResultsRequest ()
{
	json_t *req_p = 0;
	const size_t size = pw_widgets.size ();
	const uuid_t **ids_pp = (const uuid_t **) AllocMemoryArray (size, sizeof (const uuid_t *));

	if (ids_pp)
		{
			for (size_t i = 0; i < size; ++ i)
				{
					* (ids_pp + i) = pw_widgets.at (i) -> GetUUID ();
				}

			req_p = GetServicesResultsRequest (ids_pp, size, pw_data_p -> qcd_base_data.cd_connection_p, pw_data_p -> qcd_base_data.cd_schema_p);

			FreeMemory (ids_pp);
		}		/* if (ids_pp) */

		return req_p;
}

