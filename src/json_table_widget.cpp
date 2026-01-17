

#include "json_table_widget.h"


#include "qt_parameter_widget.h"
#include "qt_client_data.h"


JSONTableWidget :: JSONTableWidget (JSONParameter * const param_p, QTParameterWidget * const parent_p)
	: BaseTableWidget (& (param_p -> jp_base_param), parent_p)
{
	jtw_param_p = param_p;

	SetWidgetEnabled (! (jtw_param_p -> jp_base_param.pa_read_only_flag));

}


JSONTableWidget :: ~JSONTableWidget ()
{

}


void JSONTableWidget :: SetDefaultValue ()
{
	const json_t *def_value_p = GetJSONParameterDefaultValue (jtw_param_p);

	if (def_value_p)
		{

		}
	else
		{
			ClearTable ();
		}
}


bool JSONTableWidget :: SetValueFromText (const char *value_s)
{
	bool success_flag = false;
	json_error_t err;
	json_t *value_p = json_loads (value_s, 0, &err);

	if (value_p)
		{
			success_flag = SetValueFromJSON (value_p);
			json_decref (value_p);
		}

	return success_flag;
}


bool JSONTableWidget :: SetValueFromJSON (const json_t * const value_p)
{
	bool success_flag = false;

	if (bpw_parent_p -> GetClientData () -> qcd_verbose_flag)
		{
			PrintJSONToLog (STM_LEVEL_INFO, __FILE__, __LINE__, value_p, "JSONTableWidget :: SetValueFromJSON on widget \"%s\"\n", GetParameterName ());
		}

	ClearTable ();

	if (json_is_array (value_p))
			{
				size_t num_rows = json_array_size (value_p);
				const int num_cols = ptw_table_p -> columnCount ();

				/*
				 * Make sure that the table is of sufficient size
				 */
				if (num_rows >= (size_t) (ptw_table_p -> rowCount ()))
					{
						ptw_table_p -> setRowCount (num_rows + 1);
					}

				for (size_t i = 0; i < num_rows; ++ i)
					{
						json_t *row_data_p = json_array_get (value_p, i);

						for (int j = 0; j < num_cols; ++ j)
							{
								QString s = ptw_table_p -> horizontalHeaderItem (j) -> text ();
								QByteArray ba = s.toLocal8Bit ();
								const char *key_s = ba.constData ();
								json_t *json_value_p = json_object_get (row_data_p, key_s);
								QVariant v =  ptw_table_p -> horizontalHeaderItem (j) -> data (Qt::UserRole);
								QString column_type = v.toString();
								QByteArray column_type_ba = column_type.toLocal8Bit ();
								const char *column_type_s = column_type_ba.constData ();
								QTableWidgetItem *item_p = ptw_table_p -> item (i, j);

								/* default to strings */
								ParameterType param_type = PT_STRING;

								if (column_type_s)
									{
										if (!GetGrassrootsTypeFromString (column_type_s, &param_type))
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get datatype for column \"%s\" fpr column type \"%s\"\n", key_s, column_type_s);
											}
									}

								if (!item_p)
									{
										item_p = new QTableWidgetItem ();

										if (j >= num_cols)
											{
												ptw_table_p -> setColumnCount (num_cols + 1);
											}

										ptw_table_p -> setItem (i, j, item_p);
									}

								if (item_p)
									{
										QVariant *v_p = nullptr;

										switch (param_type)
											{
											case PT_BOOLEAN:
												if (json_is_boolean (json_value_p))
													{
														v_p = new QVariant (json_is_true (json_value_p) ? true : false);
													}
												break;

											case PT_CHAR:
												if (json_is_string (json_value_p))
													{
														const char *value_s = json_string_value (json_value_p);

														if (value_s && (strlen (value_s) == 1))
															{
																v_p = new QVariant (*value_s);
															}
													}
												break;

											case PT_SIGNED_INT:
											case PT_NEGATIVE_INT:
											case PT_UNSIGNED_INT:
												if (json_is_integer (json_value_p))
													{
														v_p = new QVariant (json_integer_value (json_value_p));
													}
												break;


											case PT_SIGNED_REAL:
											case PT_UNSIGNED_REAL:
												if (json_is_real (json_value_p))
													{
														v_p = new QVariant (json_real_value (json_value_p));
													}
												else if (json_is_integer (json_value_p))
													{
														v_p = new QVariant (static_cast <double> (json_integer_value (json_value_p)));
													}
												break;


											case PT_DIRECTORY:
											case PT_FILE_TO_READ:
											case PT_FILE_TO_WRITE:
												{
												}
												break;

											case PT_TABLE:
											case PT_STRING:
											case PT_LARGE_STRING:
											case PT_PASSWORD:
											case PT_KEYWORD:
											case PT_FASTA:
											case PT_COMPLETABLE_STRING:
												if (json_is_string (json_value_p))
													{
														v_p = new QVariant (json_string_value (json_value_p));
													}
												break;

											case PT_STRING_ARRAY:
												break;

											case PT_JSON:
											case PT_JSON_TABLE:
												{
												}
												break;

											case PT_TIME:
												if (json_is_string (json_value_p))
													{
														const char *value_s = json_string_value (json_value_p);
//														struct tm *time_p = GetTimeFromString (value_s);

														v_p = new QVariant (value_s);
													}
												break;

											case PT_NUM_TYPES:
												break;
											}

										if (v_p)
											{
												item_p -> setData (Qt::DisplayRole, *v_p);
												delete v_p;
											}
										else
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No value to set\n");
											}

									}
							}
					}

				success_flag = true;
			}
		else if (json_is_string (value_p))
			{
				const char *value_s = json_string_value (value_p);

				success_flag = SetValueFromText (value_s);
			}

		return success_flag;
}


bool JSONTableWidget :: SetFromParameterValue (Parameter *param_p)
{
	bool success_flag = false;

	if (IsJSONParameter (param_p))
		{
			const json_t *value_p = GetJSONParameterCurrentValue ((JSONParameter *) param_p);

			if (SetValueFromJSON (value_p))
				{
					SetWidgetEnabled (! (param_p -> pa_read_only_flag));
					success_flag = true;
				}
		}

	return success_flag;
}


bool JSONTableWidget :: StoreParameterValue (bool refresh_flag)
{
	bool success_flag = false;
	json_t *table_json_p = ptw_table_p -> GetValueAsJSON (&success_flag);

	if (success_flag && table_json_p)
		{
			success_flag = SetJSONParameterCurrentValue (jtw_param_p, table_json_p);

			json_decref (table_json_p);
		}

	return success_flag;
}


void JSONTableWidget :: Clear ()
{
	BaseTableWidget :: Clear ();

	SetJSONParameterCurrentValue (jtw_param_p, nullptr);
}
