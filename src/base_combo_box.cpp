#include "base_combo_box.h"
#include "qt_parameter_widget.h"


BaseComboBox :: BaseComboBox (Parameter * const param_p, QTParameterWidget * const parent_p)
	: BaseParamWidget (param_p, parent_p)
{
	int max_items = 20;

	bcb_combo_box_p = new QComboBox;

	SetMaxVisibleItemsFromParameter (param_p);

	if (param_p -> pa_refresh_service_flag)
		{
			QObject ::  connect (bcb_combo_box_p,  &QComboBox :: currentTextChanged, parent_p, &QTParameterWidget :: RefreshService);
		}
}

BaseComboBox :: ~BaseComboBox ()
{
	delete bcb_combo_box_p;
}


void BaseComboBox :: SetMaxVisibleItemsFromParameter (Parameter *param_p)
{
    int max_items = 20;

	if (param_p -> pa_options_p)
		{
        if (static_cast <uint32> (max_items) < param_p -> pa_options_p -> ll_size)
				{
					max_items = param_p -> pa_options_p -> ll_size;
				}
		}

	bcb_combo_box_p -> setMaxCount (max_items);
	bcb_combo_box_p -> setMaxVisibleItems (max_items);
}


bool BaseComboBox :: SetParameter (Parameter *param_p)
{
	if (BaseParamWidget :: SetParameter (param_p))
		{
			SetMaxVisibleItemsFromParameter (param_p);
			return true;
		}

	return false;
}


QWidget *BaseComboBox :: GetQWidget ()
{
	return bcb_combo_box_p;
}


bool BaseComboBox :: IsComboBoxWidget () const
{
	return true;
}
