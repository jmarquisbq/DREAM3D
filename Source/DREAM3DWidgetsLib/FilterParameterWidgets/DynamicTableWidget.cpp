/* ============================================================================
* Copyright (c) 2012 Michael A. Jackson (BlueQuartz Software)
* Copyright (c) 2012 Dr. Michael A. Groeber (US Air Force Research Laboratories)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* Neither the name of Michael A. Groeber, Michael A. Jackson, the US Air Force,
* BlueQuartz Software nor the names of its contributors may be used to endorse
* or promote products derived from this software without specific prior written
* permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*  This code was written under United States Air Force Contract number
*                           FA8650-10-D-5210
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "DynamicTableWidget.h"

#include <QtCore/QMetaProperty>

#include "DREAM3DLib/FilterParameters/FilterParameter.h"
#include "DREAM3DLib/FilterParameters/DynamicTableData.h"

#include "FilterParameterWidgetsDialogs.h"

#include "DREAM3DWidgetsLib/FilterParameterWidgets/DynamicTableItemDelegate.h"
#include "DREAM3DWidgetsLib/DREAM3DWidgetsLibConstants.h"


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DynamicTableWidget::DynamicTableWidget(FilterParameter* parameter, AbstractFilter* filter, QWidget* parent) :
FilterParameterWidget(parameter, filter, parent)
{
	m_FilterParameter = dynamic_cast<DynamicTableFilterParameter*>(parameter);
	Q_ASSERT_X(NULL != m_FilterParameter, "DynamicTableWidget can ONLY be used with Dynamic Table Filter Parameters", __FILE__);

	setupUi(this);
	setupGui();

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DynamicTableWidget::~DynamicTableWidget()
{}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::initializeWidget(FilterParameter* parameter, AbstractFilter* filter)
{
	setFilter(filter);
	setFilterParameter(parameter);
	setupGui();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::setFilterParameter(FilterParameter* value)
{
	m_FilterParameter = dynamic_cast<DynamicTableFilterParameter*>(value);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FilterParameter* DynamicTableWidget::getFilterParameter() const
{
	return m_FilterParameter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::setupGui()
{
	// Catch when the filter is about to execute the preflight
	connect(getFilter(), SIGNAL(preflightAboutToExecute()),
		this, SLOT(beforePreflight()));

	// Catch when the filter is finished running the preflight
	connect(getFilter(), SIGNAL(preflightExecuted()),
		this, SLOT(afterPreflight()));

	// Catch when the filter wants its values updated
	connect(getFilter(), SIGNAL(updateFilterParameters(AbstractFilter*)),
		this, SLOT(filterNeedsInputParameters(AbstractFilter*)));

	tableLabel->setText(m_FilterParameter->getHumanLabel());

	// Set the item delegate so that we can only enter 'double' values into the table
	DynamicTableItemDelegate* dlg = new DynamicTableItemDelegate;
	dynamicTable->setItemDelegate(dlg);

	// Get what is in the filter
	DynamicTableData data = getFilter()->property(PROPERTY_NAME_AS_CHAR).value<DynamicTableData>();

	// If there was nothing in the filter, use the defaults
	if (m_FilterParameter != NULL)
	{
		if (data.getTableData().size() == 0)
		{
			data.setTableData(m_FilterParameter->getDefaultTable());
			data.setNumRows(m_FilterParameter->getDefaultRowCount());
			data.setNumCols(m_FilterParameter->getDefaultColCount());
			data.setRowHeaders(m_FilterParameter->getRowHeaders());
			data.setColHeaders(m_FilterParameter->getColumnHeaders());
		}

		// Populate the table with the default values
		std::vector<std::vector<double> > tableData = data.getTableData();
		for (int row = 0; row < tableData.size(); row++)
		{
			dynamicTable->insertRow(row);
			for (int col = 0; col < tableData[row].size(); col++)
			{
				if (dynamicTable->columnCount() == col)
				{
					dynamicTable->insertColumn(col);
				}

				QTableWidgetItem* item = new QTableWidgetItem(QString::number(tableData[row][col]));
				dynamicTable->setItem(row, col, item);
			}
		}

		// Populate row and column headers
		dynamicTable->setVerticalHeaderLabels(data.getRowHeaders());
		dynamicTable->setHorizontalHeaderLabels(data.getColHeaders());

		// Resize rows and columns to contents
		dynamicTable->resizeColumnsToContents();
		dynamicTable->resizeRowsToContents();

		// Hide add/remove row buttons if row count is not dynamic
		if (m_FilterParameter->getAreRowsDynamic() == false)
		{
			addRowBtn->setHidden(true);
			deleteRowBtn->setHidden(true);
		}

		// Hide add/remove column buttons if column count is not dynamic
		if (m_FilterParameter->getAreColsDynamic() == false)
		{
			addColBtn->setHidden(true);
			deleteColBtn->setHidden(true);
		}
	}
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::on_dynamicTable_cellChanged(int row, int col)
{
	m_DidCausePreflight = true;
	emit parametersChanged();
	m_DidCausePreflight = false;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::filterNeedsInputParameters(AbstractFilter* filter)
{
	QStringList rHeaders, cHeaders;
	for (int i = 0; i < dynamicTable->rowCount(); i++)
	{
		QTableWidgetItem* vItem = dynamicTable->verticalHeaderItem(i);
		if (NULL != vItem)
		{
			QString vName = vItem->data(Qt::DisplayRole).toString();
			rHeaders << vName;
		}
	}
	for (int i = 0; i < dynamicTable->columnCount(); i++)
	{
		QTableWidgetItem* cItem = dynamicTable->horizontalHeaderItem(i);
		if (NULL != cItem)
		{
			QString cName = cItem->data(Qt::DisplayRole).toString();
			cHeaders << cName;
		}
	}

	DynamicTableData data(getData(), dynamicTable->rowCount(), dynamicTable->columnCount(), rHeaders, cHeaders);

	QVariant v;
	v.setValue(data);
	bool ok = filter->setProperty(PROPERTY_NAME_AS_CHAR, v);

	if (false == ok)
	{
		FilterParameterWidgetsDialogs::ShowCouldNotSetFilterParameter(getFilter(), m_FilterParameter);
	}
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
std::vector<std::vector<double> > DynamicTableWidget::getData()
{
	int rCount = dynamicTable->rowCount(), cCount = dynamicTable->columnCount();
	std::vector<std::vector<double> > data(rCount, std::vector<double>(cCount));

	for (int row = 0; row < rCount; row++)
	{
		for (int col = 0; col < cCount; col++)
		{
			bool ok = false;
			data[row][col] = dynamicTable->item(row, col)->data(Qt::DisplayRole).toDouble(&ok);

			if (ok == false)
			{
				qDebug() << "Could not set the model data into the DynamicTableData object.";
				data.clear();
				return data;
			}
		}
	}

	return data;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::beforePreflight()
{

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::afterPreflight()
{
	
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::on_addRowBtn_pressed()
{
	int row = dynamicTable->rowCount();

	// If we are adding the first row, add the first column too.
	if (row <= 0)
	{
		dynamicTable->insertColumn(0);
		dynamicTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Column 1"));
	}

	dynamicTable->insertRow(row);
	dynamicTable->setVerticalHeaderItem(row, new QTableWidgetItem("Row " + QString::number(dynamicTable->rowCount())));

	dynamicTable->blockSignals(true);
	for (int col = 0; col < dynamicTable->columnCount(); col++)
	{
		if (col + 1 == dynamicTable->columnCount())
		{
			// Only fire the signal after the last new item has been created
			dynamicTable->blockSignals(false);
		}

		QTableWidgetItem* item = new QTableWidgetItem("0");
		dynamicTable->setItem(row, col, item);
	}

	// Resize entire table to contents
	dynamicTable->resizeRowsToContents();
	dynamicTable->resizeColumnsToContents();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::on_deleteRowBtn_pressed()
{
	dynamicTable->removeRow(dynamicTable->currentRow());

	if (dynamicTable->rowCount() <= 0)
	{
		while (dynamicTable->columnCount() > 0)
		{
			dynamicTable->removeColumn(0);
		}
	}

	// Cause a preflight
	m_DidCausePreflight = true;
	emit parametersChanged();
	m_DidCausePreflight = false;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::on_addColBtn_pressed()
{
	int col = dynamicTable->columnCount();

	// If we are adding the first column, add the first row too.
	if (col <= 0)
	{
		dynamicTable->insertRow(0);
		dynamicTable->setVerticalHeaderItem(0, new QTableWidgetItem("Row 1"));
	}

	dynamicTable->insertColumn(col);
	dynamicTable->setHorizontalHeaderItem(col, new QTableWidgetItem("Column " + QString::number(dynamicTable->columnCount())));

	dynamicTable->blockSignals(true);
	for (int row = 0; row < dynamicTable->rowCount(); row++)
	{
		if (row + 1 == dynamicTable->rowCount())
		{
			// Only fire the signal after the last new item has been created
			dynamicTable->blockSignals(false);
		}

		QTableWidgetItem* item = new QTableWidgetItem("0");
		dynamicTable->setItem(row, col, item);
	}

	// Resize entire table to contents
	dynamicTable->resizeRowsToContents();
	dynamicTable->resizeColumnsToContents();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DynamicTableWidget::on_deleteColBtn_pressed()
{
	dynamicTable->removeColumn(dynamicTable->currentColumn());

	if (dynamicTable->columnCount() <= 0)
	{
		while (dynamicTable->rowCount() > 0)
		{
			dynamicTable->removeRow(0);
		}
	}

	// Cause a preflight
	m_DidCausePreflight = true;
	emit parametersChanged();
	m_DidCausePreflight = false;
}




