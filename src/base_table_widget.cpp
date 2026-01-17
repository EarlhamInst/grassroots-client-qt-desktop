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
#include "base_table_widget.h"

#include <stdio.h>

#include <QDebug>
#include <QFont>
#include <QMimeData>
#include <QTableWidgetItem>
#include <QMenu>
#include <QAction>
#include "prefs_widget.h"

#include "string_utils.h"
#include "byte_buffer.h"

#include "json_parameter.h"
#include "qt_parameter_widget.h"
#include "qt_client_data.h"


const char * const BaseTableWidget :: PTW_COLUMN_HEADERS_S = "COLUMN_HEADERS";




BaseTableWidget :: BaseTableWidget (Parameter * const param_p, QTParameterWidget * const parent_p)
:		BaseParamWidget (param_p, parent_p)
{
	bool first_row_headers_flag = false;

	const char *value_s = GetParameterKeyValue (param_p, PA_TABLE_COLUMN_DELIMITER_S);

	if (value_s)
		{
			ptw_column_delimiter = *value_s;
		}
	else
		{
			ptw_column_delimiter = *PA_TABLE_DEFAULT_COLUMN_DELIMITER_S;
		}

	value_s = GetParameterKeyValue (param_p, PA_TABLE_ROW_DELIMITER_S);

	if (value_s)
		{
			ptw_row_delimiter = *value_s;
		}
	else
		{
			ptw_row_delimiter = *PA_TABLE_DEFAULT_ROW_DELIMITER_S;
		}

	value_s = GetParameterKeyValue (param_p, PA_TABLE_COLUMN_HEADERS_PLACEMENT_S);

	if (value_s)
		{
			if (strcmp (value_s, PA_TABLE_COLUMN_HEADERS_PLACEMENT_FIRST_ROW_S) == 0)
				{
					first_row_headers_flag = true;
				}
		}

	ptw_table_p = new DroppableTableWidget (parent_p, this, ptw_row_delimiter, ptw_column_delimiter, first_row_headers_flag);

	ptw_scroller_p = new QScrollArea (parent_p);
	ptw_scroller_p -> setWidgetResizable (true);
	ptw_scroller_p -> setWidget (ptw_table_p);
	ptw_scroller_p -> setSizeAdjustPolicy (QAbstractScrollArea :: AdjustToContents);


	if (!SetColumnHeaders (param_p))
		{
		}

	/*
	 * Additional columns
	 */
	ptw_addable_columns_flag = false;

	value_s = GetParameterKeyValue (param_p, PA_TABLE_ADD_COLUMNS_FLAG_S);
	if (value_s)
		{
			if (strcmp (value_s, "true") == 0)
				{
					ptw_addable_columns_flag = true;
				}
		}

	if (!value_s)
		{
			value_s = "NULL";
		}

	printf (">>>>>>> %s: %s\n", PA_TABLE_ADD_COLUMNS_FLAG_S, value_s);


	if (param_p -> pa_refresh_service_flag)
		{
			QObject ::  connect (ptw_table_p,  &QTableWidget :: itemChanged, parent_p, &QTParameterWidget :: RefreshService);
		}
}


BaseTableWidget ::	~BaseTableWidget ()
{
	delete ptw_scroller_p;
	ptw_scroller_p = nullptr;
}


void BaseTableWidget :: RemoveConnection ()
{
}


QWidget *BaseTableWidget :: GetQWidget ()
{
	return ptw_scroller_p;
}


bool BaseTableWidget :: AreColumnsAddable () const
{
	return ptw_addable_columns_flag;
}


bool BaseTableWidget :: AddColumnHeader (const char * const name_s)
{
	bool success_flag = false;

	int i = (ptw_table_p -> columnCount ());

	ptw_table_p -> setColumnCount (i + 1);

	if (SetColumnHeader (i, name_s, nullptr, nullptr, false))
		{
			ptw_additional_column_headers << QString (name_s);
			success_flag = true;
		}
	else
		{
			ptw_table_p -> setColumnCount (i);
		}

	return success_flag;
}



bool BaseTableWidget :: SetColumnHeader (int col, const char * const name_s, const char * const description_s, const char *type_s, const bool required_flag)
{
	bool success_flag = false;
	const bool verbose_flag = bpw_parent_p -> GetClientData () -> qcd_verbose_flag;

	QTableWidgetItem *column_header_p = new QTableWidgetItem (name_s);

	if (verbose_flag)
		{
            qDebug () << "name_s: " << name_s << Qt :: endl;
		}

	if (column_header_p)
		{
			if (type_s)
				{
					QVariant v (type_s);
					column_header_p -> setData (Qt::UserRole, v);
				}
			else
				{
					type_s = "NULL";
				}


			if (description_s)
				{
					column_header_p -> setToolTip (description_s);
				}

			if (required_flag)
				{
					QFont f = column_header_p -> font ();
					f.setBold (true);
					column_header_p -> setFont (f);
				}

			if (verbose_flag)
				{
                    qDebug () << "setting header " <<  column_header_p -> text () << " for column " << col << " with data " << type_s << " = " << column_header_p -> data (Qt :: UserRole) <<  Qt :: endl;
				}

			ptw_table_p -> setHorizontalHeaderItem (col, column_header_p);
			success_flag = true;
		}
	else
		{
            qDebug () << "failed to create column header for header_s: " << name_s << Qt :: endl;
		}

	return success_flag;
}


void BaseTableWidget :: SetWidgetEnabled (const bool enabled_flag)
{
	if (ptw_table_p -> isEnabled () != enabled_flag)
		{
			ptw_table_p -> setEnabled (enabled_flag);
		}
}



bool BaseTableWidget :: SetColumnHeaders (Parameter *param_p)
{
	bool success_flag = false;
	const char *value_s = GetParameterKeyValue (param_p, PA_TABLE_COLUMN_HEADINGS_S);

	if (value_s)
		{
			json_error_t json_err;
			json_t *value_p = json_loads (value_s, 0, &json_err);

			if (value_p)
				{
					if (json_is_array (value_p))
						{
							QVector <QTableWidgetItem *> column_headers;
							const size_t num_columns = json_array_size (value_p);
							size_t i;

							ptw_table_p -> setColumnCount (num_columns);

							for (i = 0; i < num_columns; ++ i)
								{
									json_t *column_info_p = json_array_get (value_p, i);

									const char *name_s = GetJSONString (column_info_p, PARAM_NAME_S);

									if (name_s)
										{
											const char *type_s = GetJSONString (column_info_p, PARAM_TYPE_S);
											const char *description_s = GetJSONString (column_info_p, PARAM_DESCRIPTION_S);
											bool required_flag = false;

											GetJSONBoolean (column_info_p, PARAM_REQUIRED_S, &required_flag);

											bool b = SetColumnHeader (i, name_s, description_s, type_s, required_flag);
										}

								}

						}

					json_decref (value_p);
				}


			success_flag = true;
			ptw_table_p -> setRowCount (5);

		}		/* if (value_s) */
	else
		{
            qDebug () << "no headers: " << Qt :: endl;
		}

	return success_flag;
}


void BaseTableWidget :: ClearTable (bool triggered_flag)
{
	Clear ();
}


void BaseTableWidget :: Clear ()
{
	ptw_table_p -> clear ();
	ptw_additional_column_headers.clear ();
	SetColumnHeaders (bpw_param_p);

}


void BaseTableWidget :: SetErrors (const json_t *errors_p)
{
	if (errors_p)
		{
			if (json_is_array (errors_p))
				{
					size_t i;
					json_t *value_p;
					const int num_columns = ptw_table_p -> columnCount ();

					json_array_foreach (errors_p, i, value_p)
					{
						json_int_t row;

						if (GetJSONInteger (value_p, TABLE_PARAM_ROW_S, &row))
							{
								const char *column_s = GetJSONString (value_p, TABLE_PARAM_COLUMN_S);

								/*
								 * The error messages start counting from row 1 whereas our table
								 * starts from row 0 so decrement it.
								 */
								-- row;

								if (column_s)
									{
										for (int j = num_columns - 1; j >= 0; -- j)
											{
												QTableWidgetItem *item_p = ptw_table_p -> horizontalHeaderItem (j);
												QString title = item_p -> text ();
												QByteArray title_ba = title.toLocal8Bit ();
												const char *title_s = title_ba.constData ();

												if (strcmp (title_s, column_s) == 0)
													{
														item_p = ptw_table_p -> item (row, j);

														if (!item_p)
															{
																item_p = new QTableWidgetItem ();
																ptw_table_p -> setItem (row, j, item_p);
															}

														if (item_p)
															{
																QBrush b (Qt :: red);
																const char *error_s = GetJSONString (value_p, JOB_ERROR_S);

																item_p -> setBackground (b);

																if (error_s)
																	{
																		item_p -> setToolTip (error_s);
																	}
															}

														/* force exit from loop */
														j = -1;
													}
											}

									}
							}

					}		/* json_array_foreach (errors_p, i, value_p) */

				}		/* if (json_is_array (errors_p)) */

		}		/* if (errors_p) */
}


