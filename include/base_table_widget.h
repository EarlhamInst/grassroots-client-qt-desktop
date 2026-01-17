#ifndef BASE_TABLE_WIDGET_H
#define BASE_TABLE_WIDGET_H

#include <QScrollArea>

#include "base_param_widget.h"
#include "droppable_table_widget.h"


class BaseTableWidget : public BaseParamWidget
{
	Q_OBJECT

public:
	static const char * const PTW_COLUMN_HEADERS_S;

	BaseTableWidget (Parameter * const param_p, QTParameterWidget * const parent_p);
	virtual ~BaseTableWidget ();

	virtual void RemoveConnection ();

	virtual void SetErrors (const json_t *errors_p);

	virtual void Clear ();

	virtual void SetWidgetEnabled (const bool enabled_flag);


	bool SetColumnHeaders (Parameter *param_p);

	bool AddColumnHeader (const char * const header_s);

	bool SetColumnHeader (int col, const char * const name_s, const char * const description_s, const char *type_s, const bool required_flag);

	bool AreColumnsAddable () const;


public slots:
	void ClearTable (bool triggered_flag = false);

protected:
	QScrollArea *ptw_scroller_p;
	DroppableTableWidget *ptw_table_p;
	char ptw_column_delimiter;
	char ptw_row_delimiter;
	QStringList ptw_additional_column_headers;
	bool ptw_addable_columns_flag;

	virtual QWidget *GetQWidget ();
};


#endif // BASE_TABLE_WIDGET_H
