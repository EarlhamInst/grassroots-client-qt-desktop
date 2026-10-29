#include <QDebug>


#include "repeatable_param_group_box.h"
#include "parameter_group.h"
#include "string_utils.h"
#include "qt_parameter_widget.h"
#include "qt_client_data.h"
#include "streams.h"


RepeatableParamGroupBox :: RepeatableParamGroupBox (ParameterGroup *group_p, QTParameterWidget *qt_param_widget_p,  bool removable_flag, bool add_params_flag)
 : ParamGroupBox (group_p, qt_param_widget_p, removable_flag, add_params_flag)
{
  rpgb_entries_p = new QListWidget (this);
  rpgb_entries_p -> setSelectionMode (QAbstractItemView :: SingleSelection);

  rpgb_remove_entry_button_p = new QPushButton (QIcon ("images/remove"), "Remove", this);
  connect (rpgb_remove_entry_button_p, &QPushButton :: clicked, this, &RepeatableParamGroupBox :: RemoveEntry);

  //	rpgb_label_param_p = nullptr;

}


RepeatableParamGroupBox :: ~RepeatableParamGroupBox ()
{

}



void RepeatableParamGroupBox :: Init (bool add_params_flag)
{
  QVBoxLayout *layout_p = new QVBoxLayout ();

	qDebug () << "RepeatableParamGroupBox :: init for " << pgb_parameter_group_p -> pg_name_s << Qt :: endl;


	layout_p -> addWidget (rpgb_entries_p);

	QHBoxLayout *buttons_layout_p = new QHBoxLayout;
	QPushButton *row_button_p = new QPushButton (QIcon ("images/add"), "Add", this);
	connect (row_button_p, &QPushButton :: clicked, this, &RepeatableParamGroupBox :: AddEntry);
	buttons_layout_p -> addWidget (row_button_p);

	buttons_layout_p -> addWidget (rpgb_remove_entry_button_p);

	layout_p -> addItem (buttons_layout_p);

	layout_p -> addItem (pgb_layout_p);


	connect (rpgb_entries_p, &QListWidget :: itemSelectionChanged, this, &RepeatableParamGroupBox :: SelectedListEntryChanged);

	setLayout (layout_p);
}



void RepeatableParamGroupBox :: AddParameterWidget (BaseParamWidget *param_widget_p)
{
	ParamGroupBox :: AddParameterWidget (param_widget_p);

    param_widget_p -> SetUpdateOnEdit (true);
}


json_t *RepeatableParamGroupBox :: GetParametersAsJSON ()
{
	json_t *res_p = json_array ();

	if (res_p)
		{
			/*
			 * Add the initial parameter names to this array so that we have
			 *
			 * [
			 * 	 { "param_0_name": [ ] },
			 * 	 { "param_1_name": [ ] }
			 * ]
			 */
			bool success_flag = true;
			ParameterNode *param_node_p = reinterpret_cast <ParameterNode *> (pgb_parameter_group_p -> pg_params_p -> ll_head_p);
			const char *group_s = this -> GetGroupName ();

			while (param_node_p && success_flag)
				{
					json_t *param_json_p = json_object ();

					success_flag = false;

					if (param_json_p)
						{
							if (json_array_append_new (res_p, param_json_p) == 0)
								{
									if (SetJSONString (param_json_p, PARAM_NAME_S, param_node_p -> pn_parameter_p -> pa_name_s))
										{
											json_t *param_values_p = json_array ();

											if (param_values_p)
												{
													if (json_object_set_new (param_json_p, PARAM_CURRENT_VALUE_S, param_values_p) == 0)
														{
															if (SetJSONString (param_json_p, PARAM_GROUP_S, group_s))
																{
																	success_flag = true;
																}
														}

													if (!success_flag)
														{
															json_decref (param_values_p);
														}
												}

										}
								}
							else
								{
									json_decref (param_json_p);
								}
						}

					if (success_flag)
						{
							param_node_p = reinterpret_cast <ParameterNode *> (param_node_p -> pn_node.ln_next_p);
						}
				}


			if (success_flag)
				{
					/*
					 * Now we iterate over the entries and add them into the json array
					 * that we created above
					 */
					const int num_entries = rpgb_entries_p -> count ();
					const size_t num_params = json_array_size (res_p);

					for (int i = 0; i < num_entries; ++ i)
						{
							QListWidgetItem *item_p = rpgb_entries_p -> item (i);
							json_t *entry_json_p = GetListItemDataAsJSON (item_p);

							if (entry_json_p)
								{
									json_t *entry_parameters_json_p = json_object_get (entry_json_p, PARAM_SET_PARAMS_S);

									if (entry_parameters_json_p)
										{
											const size_t num_entry_params = json_array_size (entry_parameters_json_p);

											for (size_t j = 0; j < num_entry_params; ++ j)
												{
													json_t *entry_param_p = json_array_get (entry_parameters_json_p, j);

													const char *param_s = GetJSONString (entry_param_p, PARAM_NAME_S);

													if (param_s)
														{
															/*
															 * Find the matching parameter json in our
															 * results array
															 */
															json_t *param_p = nullptr;

															for (size_t k = 0; k < num_params; ++ k)
																{
																	json_t *tmp_p = json_array_get (res_p, k);
																	const char *tmp_name_s = GetJSONString (tmp_p, PARAM_NAME_S);

																	if (strcmp (tmp_name_s, param_s) == 0)
																		{
																			param_p = tmp_p;
																			k = num_params;		/* force exit from loop */
																		}

																}

															if (param_p)
																{
																	json_t *value_p = json_object_get (entry_param_p, PARAM_CURRENT_VALUE_S);

																	if (value_p)
																		{
																			json_t *param_values_p = json_object_get (param_p, PARAM_CURRENT_VALUE_S);

																			if (param_values_p)
																				{
																					/*
																					 * Parameters that already have array values, such as
																					 * StringArrayParameters, need their entries added rather
																					 * than the array itself, otherwise we'd get an array of
																					 * arrays which the server can't parse.
																					 */
																					const int res = json_is_array (value_p) ?
																						json_array_extend (param_values_p, value_p) :
																						json_array_append (param_values_p, value_p);

																					if (res != 0)
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, value_p, "Failed to add value for \"%s\"", param_s);
																						}
																				}
																		}
																}


														}		/* if (param_s) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_json_p, "Failed to get %s\n", PARAM_SET_PARAMS_S);
														}


												}		/* for (size_t j = 0; j < num_entry_params; ++ j) */




										}		/* if (entry_parameters_json_p)*/
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_json_p, "Failed to get %s\n", PARAM_SET_PARAMS_S);
										}

								}		/* if (entry_json_p) */
							else
								{
									qDebug () << "no data for " << i << Qt :: endl;
								}

						}		/* for (int i = 0; i < num_entries; ++ i) */

				}		/* if (success_flag) */


		}		/* if (res_p) */


	return res_p;
}


bool RepeatableParamGroupBox :: SetParametersFromJSON (json_t *params_json_p)
{
	if (pgb_parent_p -> GetClientData () -> qcd_verbose_flag)
		{
			PrintJSONToLog (STM_LEVEL_SEVERE, __FILE__, __LINE__, params_json_p, "RepeatableParamGroupBox :: SetParametersFromJSON () for \"%s\"", GetGroupName ());
		}

	return false;
}


json_t *RepeatableParamGroupBox :: GetParametersAsGroupJSON ()
{
	json_t *res_p = json_array ();

	if (res_p)
		{
			const int num_entries = rpgb_entries_p -> count ();

			for (int i = 0; i < num_entries; ++ i)
				{
					QListWidgetItem *item_p = rpgb_entries_p -> item (i);
					json_t *entry_json_p = GetListItemDataAsJSON (item_p);

					if (entry_json_p)
						{
							if (json_array_append_new (res_p, entry_json_p) != 0)
								{
									json_decref (entry_json_p);
								}
						}
					else
						{
							qDebug () << "failed to convert " << i << " to json " << Qt :: endl;
						}
				}

		}		/* if (res_p) */


	return res_p;
}



void RepeatableParamGroupBox :: AddListEntry (const char *label_s, json_t *group_json_p)
{
	QString label;


	if (label_s)
		{
			label = label_s;
		}
	else
		{
			int i = rpgb_entries_p -> count ();
			label = QString :: number (i);
		}


	if (pgb_parent_p -> GetClientData () -> qcd_verbose_flag)
		{
			PrintJSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, group_json_p, "Adding \"%s\" to list for repeatable group \"%s\"", label_s, pgb_parameter_group_p -> pg_name_s);
		}

	QListWidgetItem *item_p = new QListWidgetItem (label, rpgb_entries_p);

	char *value_s = json_dumps (group_json_p, 0);
	if (value_s)
		{
			SetListItemData (item_p, value_s);

			qDebug () << "adding " << value_s << " for " << label << ": " << GetListItemDataAsJSON (item_p) << Qt :: endl;
			free (value_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to get group json");
		}

	qDebug () << "adding " << label << ": " << GetListItemDataAsJSON (item_p)  << Qt :: endl;

	rpgb_entries_p -> addItem (item_p);
}


void RepeatableParamGroupBox :: ClearList ()
{
	rpgb_entries_p -> clear ();
}


char *RepeatableParamGroupBox :: GetRepeatableLabel ()
{
	char *label_s = NULL;
	ByteBuffer *buffer_p = AllocateByteBuffer (1024);

	if (buffer_p)
		{
			if (pgb_parameter_group_p -> pg_repeatable_label_params_p)
				{
					ParameterNode *node_p = (ParameterNode *) (pgb_parameter_group_p -> pg_repeatable_label_params_p -> ll_head_p);

					while (node_p)
						{
							bool alloc_flag = false;
							char *value_s = GetParameterValueAsString (node_p -> pn_parameter_p, &alloc_flag);

							AppendStringsToByteBuffer(buffer_p, value_s, " ", NULL);

							if (alloc_flag)
									{
										FreeCopiedString (value_s);
									}

							node_p = (ParameterNode *) (node_p -> pn_node.ln_next_p);
						}
				}

			label_s = DetachByteBufferData (buffer_p);

		}

	return label_s;
}

void RepeatableParamGroupBox :: AddEntry ()
{
	StoreValues ();

	const SchemaVersion *sv_p = pgb_parent_p -> GetClientData () -> qcd_base_data.cd_schema_p;
	json_t *group_json_p = GetParameterGroupAsJSON (pgb_parameter_group_p, true, false, sv_p);

	if (group_json_p)
		{
			ByteBuffer *buffer_p = AllocateByteBuffer (1024);

			if (buffer_p)
				{
					if (pgb_parameter_group_p -> pg_repeatable_label_params_p)
						{
							ParameterNode *node_p = (ParameterNode *) (pgb_parameter_group_p -> pg_repeatable_label_params_p -> ll_head_p);

							while (node_p)
								{
									bool alloc_flag = false;
									char *label_s = GetParameterValueAsString (node_p -> pn_parameter_p, &alloc_flag);

									AppendStringsToByteBuffer(buffer_p, label_s, " ", NULL);

									if (alloc_flag)
											{
												FreeCopiedString (label_s);
											}


									node_p = (ParameterNode *) (node_p -> pn_node.ln_next_p);
								}
						}

					AddListEntry (GetByteBufferData (buffer_p), group_json_p);

				}

			json_decref (group_json_p);
		}
}


void RepeatableParamGroupBox :: RemoveEntry ()
{
	QList <QListWidgetItem *> l =	rpgb_entries_p -> selectedItems ();
	const int count = l.size ();

	if (count == 1)
		{
			QListWidgetItem *item_p = l.at (0);

			qDebug () << "removing " << item_p -> text () << ": " << GetListItemDataAsJSON (item_p) << Qt :: endl;

			rpgb_entries_p -> removeItemWidget (item_p);
			delete item_p;
		}
}

void RepeatableParamGroupBox :: SetListItemData  (QListWidgetItem *item_p, const char *value_s)
{
	QString s (value_s);

	QVariant v (s);
	item_p -> setData (Qt :: UserRole, v);

	qDebug () << "SetListItemData " << item_p -> text () << ": " << v.toString () << Qt :: endl;

}


json_t *RepeatableParamGroupBox :: GetListItemDataAsJSON  (QListWidgetItem *item_p)
{
	json_t *group_json_p = NULL;
	QString s = item_p -> data (Qt :: UserRole).toString ();
	QByteArray ba = s.toLocal8Bit ();
	const char *group_json_s = ba.constData ();


	if (group_json_s)
		{
			json_error_t err;

			group_json_p = json_loads (group_json_s, 0, &err);
		}

	qDebug () << "GetListItemDataAsJSON " << item_p -> text () << ": " << group_json_s << Qt :: endl;

	return group_json_p;
}


void RepeatableParamGroupBox :: SelectedListEntryChanged ()
{
	QList <QListWidgetItem *> l =	rpgb_entries_p -> selectedItems ();
	const int count = l.size ();

	if (count == 1)
		{
			QListWidgetItem *item_p = l.at (0);
			int row = rpgb_entries_p -> row (item_p);
			json_t *group_json_p = GetListItemDataAsJSON (item_p);

			if (group_json_p)
				{
					json_t *params_json_p = json_object_get (group_json_p, PARAM_GROUP_PARAMS_S);

					if (params_json_p)
						{
							if (json_is_array (params_json_p))
								{
									size_t i;
									json_t *param_json_p;

									json_array_foreach (params_json_p, i, param_json_p)
										{
											const char *param_name_s = GetJSONString (param_json_p, PARAM_NAME_S);

											if (param_name_s)
												{
													json_t *value_p = json_object_get (param_json_p, PARAM_CURRENT_VALUE_S);

													if (json_is_array (value_p))
														{
															const size_t num_values = json_array_size (value_p);

                                                        if (static_cast <size_t> (row) < num_values)
																{
																	BaseParamWidget *widget_p = pgb_parent_p -> GetWidgetForParameter (param_name_s);

																	if (widget_p)
																		{
																			json_t *new_param_value_p = json_array_get (value_p, row);

																			if (pgb_parent_p -> GetClientData () -> qcd_verbose_flag)
																				{
																					PrintJSONToLog (STM_LEVEL_INFO, __FILE__, __LINE__, new_param_value_p, "Calling SetValueFromJSON on widget \"%s\" for \"%s\"\n", widget_p -> GetParameterName (),  param_name_s);
																				}
																			widget_p -> SetValueFromJSON (new_param_value_p);
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Failed to get widget for \"%s\"\n", param_name_s);
																		}

																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, value_p, "Error, array size is %d and row is %d for\"%s\"\n", (int) num_values, row,  param_name_s);
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, value_p, "Error, value is not an array for \"%s\"\n",  param_name_s);
														}

												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Failed to get param name\n");
												}

										}		/* json_array_foreach (params_json_p, i, param_json_p) */

								}		/* if (json_is_array (params_json_p)) */

						}		/* if (params_json_p) */

					json_decref (group_json_p);
				}		/* if (group_json_p) */

		}

	if (count > 0)
		{
			rpgb_remove_entry_button_p -> setEnabled (true);
		}
	else
		{
			rpgb_remove_entry_button_p -> setEnabled (false);
		}
}



