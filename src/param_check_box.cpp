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
#include <QDebug>

#include "param_check_box.h"

#include "progress_window.h"
#include "prefs_widget.h"

#include "string_utils.h"


ParamCheckBox :: ParamCheckBox (BooleanParameter * const param_p, QTParameterWidget * const parent_p)
: BaseParamWidget (& (param_p -> bp_base_param), parent_p)
{
	pcb_param_p = param_p;

	pcb_check_box_p = new QCheckBox (parent_p);

	if (param_p -> bp_base_param.pa_refresh_service_flag)
		{
            QObject ::  connect (pcb_check_box_p, &QCheckBox :: checkStateChanged, parent_p, &QTParameterWidget :: RefreshService);
		}
}


ParamCheckBox ::	~ParamCheckBox ()
{
	delete pcb_check_box_p;
}


bool ParamCheckBox :: SetParameter (Parameter *param_p)
{
	bool success_flag = false;

	if (IsBooleanParameter (param_p))
		{
			if (BaseParamWidget :: SetParameter (param_p))
				{
					BooleanParameter *bool_param_p = (BooleanParameter *) param_p;
					const bool *value_p = GetBooleanParameterCurrentValue (bool_param_p);

					if (value_p)
						{
							pcb_check_box_p -> setChecked (*value_p);
							pcb_param_p = bool_param_p;

							success_flag = true;
						}
				}
		}

	return success_flag;
}


bool ParamCheckBox :: SetFromParameterValue (Parameter *param_p)
{
	bool success_flag = false;

	if (IsBooleanParameter (param_p))
		{
			BooleanParameter *bool_param_p = (BooleanParameter *) param_p;
			const bool *value_p = GetBooleanParameterCurrentValue (bool_param_p);

			SetBooleanParameterCurrentValue (pcb_param_p, value_p);

			if (value_p)
				{
					pcb_check_box_p -> setChecked (*value_p);

					SetWidgetEnabled (! (param_p -> pa_read_only_flag));

					success_flag = true;
				}
		}

	return success_flag;
}


bool ParamCheckBox :: StoreParameterValue (bool refresh_flag)
{
	bool b = pcb_check_box_p -> isChecked ();

	return SetBooleanParameterCurrentValue (pcb_param_p, &b);
}


void ParamCheckBox :: SetDefaultValue ()
{
	const bool *b_p = GetBooleanParameterDefaultValue (pcb_param_p);
	bool b = false;

	if (b_p)
		{
			b = *b_p;
		}

	pcb_check_box_p -> setChecked (b);
}


bool ParamCheckBox :: SetValueFromText (const char *value_s)
{
	bool success_flag = false;

	if (value_s)
		{
			if (strcmp (value_s, "false") == 0)
				{
					pcb_check_box_p -> setChecked (false);
					success_flag = true;
				}
			else if (strcmp (value_s, "true") == 0)
				{
					pcb_check_box_p -> setChecked (true);
					success_flag = true;
				}
		}

	return success_flag;
}


bool ParamCheckBox :: SetValueFromJSON (const json_t * const value_p)
{
	bool success_flag = false;

	if (json_is_boolean (value_p))
		{
			if (value_p == json_true ())
				{
					pcb_check_box_p -> setChecked (true);
					success_flag = true;
				}
			else if (value_p == json_false ())
				{
					pcb_check_box_p -> setChecked (false);
					success_flag = true;
				}
		}

	return success_flag;
}


QWidget *ParamCheckBox :: GetQWidget ()
{
	return pcb_check_box_p;
}
