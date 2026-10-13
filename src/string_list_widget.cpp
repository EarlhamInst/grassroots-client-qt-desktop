/*
 * string_list_widget.cpp
 *
 *  Created on: 11 Jan 2024
 *      Author: billy
 */

#include "string_list_widget.h"
#include "string_parameter.h"
#include "string_utils.h"



StringListWidget :: StringListWidget (StringArrayParameter * const param_p, QTParameterWidget * const parent_p)
:		BaseParamWidget (& (param_p -> sap_base_param), parent_p)
{
	slw_param_p = param_p;

	slw_list_p = new QListWidget (parent_p);
	slw_list_p -> setSelectionMode (QAbstractItemView :: MultiSelection);

	AddOptions (bpw_param_p);


	if (bpw_param_p -> pa_refresh_service_flag)
		{
			QObject ::  connect (slw_list_p,  &QListWidget :: itemClicked, parent_p, &QTParameterWidget :: RefreshService);
		}

}


StringListWidget ::	~StringListWidget ()
{
	delete slw_list_p;
	slw_list_p = NULL;
}

void StringListWidget :: RemoveConnection ()
{
}


void StringListWidget :: AddOptions (Parameter *param_p)
{
	if (param_p -> pa_options_p)
		{
			StringParameterOptionNode *node_p = reinterpret_cast <StringParameterOptionNode *> (param_p -> pa_options_p -> ll_head_p);

			while (node_p)
				{
					const StringParameterOption *option_p = node_p -> spon_option_p;
					QString s (option_p -> spo_description_s);
					QListWidgetItem *item_p = new QListWidgetItem (s, slw_list_p);
					QByteArray ba (option_p -> spo_value_s);
					QVariant v (ba);

					item_p -> setData (Qt :: UserRole, v);
					slw_list_p -> addItem (item_p);

					node_p = reinterpret_cast <StringParameterOptionNode *> (node_p -> spon_node.ln_next_p);
				}
		}

}


bool StringListWidget :: SetParameter (Parameter *param_p)
{
	bool success_flag = false;

	if (IsStringArrayParameter (param_p))
		{
			slw_param_p = (StringArrayParameter *) param_p;

			if (BaseParamWidget :: SetParameter (param_p))
				{
					const char **values_ss = GetStringArrayParameterCurrentValues (slw_param_p);


					success_flag = true;
				}
		}

	return success_flag;
}


bool StringListWidget :: SetFromParameterValue (Parameter *param_p)
{
	bool success_flag = false;

	if (IsStringArrayParameter (param_p))
		{
			StringArrayParameter *new_string_array_param_p = reinterpret_cast <StringArrayParameter *> (param_p);
			const char **values_ss = GetStringArrayParameterCurrentValues (new_string_array_param_p);
			size_t num_values = GetNumberOfStringArrayCurrentParameterValues (new_string_array_param_p);

			if (values_ss)
				{
					for (size_t i = 0; i < num_values; ++ i)
						{
							PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "setting value [%u] = \"%s\"\n", i, * (values_ss + i), i);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStringArrayParameterCurrentValues () failed for \"%s\"", new_string_array_param_p -> sap_base_param.pa_name_s);
				}

			if (SetStringArrayParameterCurrentValues (slw_param_p, const_cast <char **> (values_ss), num_values))
				{
					if (new_string_array_param_p -> sap_base_param.pa_options_p)
						{
							if (CopyStringParameterOptions (& (new_string_array_param_p -> sap_base_param), & (slw_param_p -> sap_base_param), true))
								{
									slw_list_p -> clear ();
									AddOptions (& (slw_param_p -> sap_base_param));

									const int list_size = slw_list_p -> count ();
									const char **value_ss = values_ss;

									for (size_t i = 0; i < num_values; ++ i, ++ value_ss)
										{
											if (*value_ss)
												{
													size_t num_matches = 0;
													const char *value_s = *value_ss;

													for (int j = 0; j < list_size; ++ j)
														{
															QListWidgetItem *item_p = slw_list_p -> item (j);
															QVariant v = item_p -> data (Qt :: UserRole);
															QByteArray ba = v.toByteArray ();
															const char *item_value_s = ba.constData ();

															if (strcmp (item_value_s, value_s) == 0)
																{
																	item_p -> setSelected (true);
																	++ num_matches;
																}
														}

													PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "findItems () found %u matches for  \"%s\"\n", num_matches,  value_s);
												}

										}

									SetWidgetEnabled (! (param_p -> pa_read_only_flag));


									success_flag = true;

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No Parameter Options for \"%s\"\n", new_string_array_param_p -> sap_base_param.pa_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyStringParameterOptions () failed for \"%s\"\n", slw_param_p -> sap_base_param.pa_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetStringArrayParameterCurrentValues () failed for \"%s\"\n", slw_param_p -> sap_base_param.pa_name_s);

					for (size_t i = 0; i < num_values; ++ i)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "value [%u] = \"%s\"\n", i, * (values_ss + i), i);
						}
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Param \"%s\" is not a StringArrayParameter, %d\n", param_p -> pa_name_s, param_p -> pa_type);
		}

	return success_flag;
}


void StringListWidget :: SetDefaultValue ()
{
	const char **values_ss = GetStringArrayParameterDefaultValues (slw_param_p);



//	ple_text_box_p -> setText (value_s);
}


QWidget *StringListWidget :: GetQWidget ()
{
	return slw_list_p;
}


bool StringListWidget :: SetValueFromText (const char *value_s)
{
	if (value_s)
		{
			QString s (value_s);
			QStringList entries = s.split ("\n");

			const qsizetype num_values = entries.size ();

			QSet <QByteArray> selected_values;

			for (qsizetype i = 0; i < num_values; ++ i)
				{
					selected_values.insert (entries.at (i).toUtf8 ());
				}

			const int num_entries = slw_list_p -> count ();

			for (int i = 0; i < num_entries; ++ i)
				{
					QListWidgetItem *item_p = slw_list_p -> item (i);

					item_p -> setSelected (selected_values.contains (GetItemValue (item_p)));
				}

		}
	else
		{
			SelectAllEntries (false);
		}

	return true;
}


void StringListWidget :: SelectAllEntries (const bool selected_flag)
{
	const int num_entries = slw_list_p -> count ();

	for (int i = 0; i < num_entries; ++ i)
		{
			slw_list_p -> item (i) -> setSelected (selected_flag);
		}
}


/*
 * Each item's text is its option's description, the option's value
 * is stored as the item's data and is what we send to the server.
 */
QByteArray StringListWidget :: GetItemValue (const QListWidgetItem *item_p)
{
	return item_p -> data (Qt :: UserRole).toByteArray ();
}


bool StringListWidget :: SetValueFromJSON (const json_t * const value_p)
{
	bool success_flag = false;

	if (json_is_array (value_p))
		{
			size_t i;
			json_t *entry_p;

			const int num_entries = slw_list_p -> count ();

			SelectAllEntries (false);

			json_array_foreach (value_p, i, entry_p)
				{
				if (json_is_string (entry_p))
					{
						const QByteArray entry (json_string_value (entry_p));

						for (int j = 0; j < num_entries; ++ j)
							{
								QListWidgetItem *item_p = slw_list_p -> item (j);

								if (GetItemValue (item_p) == entry)
									{
										item_p -> setSelected (true);
									}
							}

					}

				}

			success_flag = true;
		}
	else if (json_is_null (value_p))
		{
			SelectAllEntries (false);
			success_flag = true;
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, value_p, "Failed to set value for StringListWidget %s", bpw_param_name_s);
		}

	return success_flag;
}



bool StringListWidget :: StoreParameterValue (bool refresh_flag)
{
	bool success_flag = false;
	char **values_ss = nullptr;
	size_t num_values = 0;

	if (GetValues (&values_ss, &num_values))
		{
			if ((!(IsRequired ())) || refresh_flag || (num_values > 0))
				{
					if (GetErrorFlag ())
						{
							SetErrorMessage (nullptr);
						}

					success_flag = UpdateConfigValue (values_ss, num_values);
				}
			else
				{
					SetErrorMessage (BaseParamWidget ::  BPW_REQUIRED_S);
				}

			if (values_ss)
				{
					FreeStringArray (values_ss, num_values);
				}
		}

	return success_flag;
}


bool StringListWidget :: UpdateConfigValue (char ** const values_ss, const size_t num_values)
{
	bool success_flag = SetStringArrayParameterCurrentValues (slw_param_p, values_ss, num_values);

	//qDebug () << "Setting " << bpw_param_p -> pa_name_s << " to " << value_s;

	return success_flag;
}


bool StringListWidget :: GetValues (char *** values_sss, size_t *num_values_p)
{
	bool success_flag = false;
	QList <QByteArray> values;
	const int num_entries = slw_list_p -> count ();
	int num_values;

	for (int i = 0; i < num_entries; ++ i)
		{
			QListWidgetItem *item_p = slw_list_p -> item (i);

			if (item_p -> isSelected ())
				{
					values.append (GetItemValue (item_p));
				}
		}

	if ((num_values = values.size ()) > 0)
		{
			char **results_ss = reinterpret_cast <char **> (AllocMemoryArray (num_values, sizeof (char *)));

			if (results_ss)
				{
					char **result_ss = results_ss;

					success_flag = true;

					/*
					 * Copy each value as the QByteArray's data is only valid
					 * whilst the QByteArray is in scope.
					 */
					for (qsizetype i = 0; i < num_values; ++ i, ++ result_ss)
						{
							*result_ss = EasyCopyToNewString (values.at (i).constData ());

							if (! (*result_ss))
								{
									success_flag = false;
									i = num_values;		/* force exit from loop */
								}
						}

					if (success_flag)
						{
							*values_sss = results_ss;
							*num_values_p = num_values;
						}
					else
						{
							FreeStringArray (results_ss, num_values);
						}
				}
		}
	else
		{
			*values_sss = nullptr;
			*num_values_p = 0;
			success_flag = true;
		}

	return success_flag;
}

