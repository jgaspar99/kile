/**********************************************************************

	--- Qt Architect generated file ---

	File: legendOpData.h
	Last generated: Sun Feb 22 20:07:13 1998

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#ifndef legendOpData_included
#define legendOpData_included

#include <qdialog.h>
#include <qchkbox.h>
#include <qlined.h>
#include <qradiobt.h>
#include <qcombo.h>

class legendOpData : public QDialog
{
    Q_OBJECT

public:

    legendOpData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~legendOpData();

public slots:


protected slots:

    virtual void setLegendOpDefaults();
    virtual void setLegendOptions();

protected:
    QRadioButton* keyButton;
    QRadioButton* noKeyButton;
    QCheckBox* positionLeftButton;
    QCheckBox* positionRightButton;
    QCheckBox* positionTopButton;
    QCheckBox* positionBottomButton;
    QCheckBox* positionOutsideButton;
    QCheckBox* positionBelowButton;
    QLineEdit* positionXEdit;
    QLineEdit* positionYEdit;
    QLineEdit* positionZEdit;
    QComboBox* textJustifyList;
    QComboBox* reverseList;
    QComboBox* boxList;
    QLineEdit* lineTypeEdit;
    QLineEdit* sampleLengthEdit;
    QLineEdit* spacingEdit;
    QLineEdit* widthIncrementEdit;
    QLineEdit* legendTitleEdit;

};

#endif // legendOpData_included
