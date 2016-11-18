#ifndef TRESHOLDBOX_H
#define TRESHOLDBOX_H

#include <QDialog>

namespace Ui {
class TresholdBox;
}

class TresholdBox : public QDialog
{
	Q_OBJECT

public:
	explicit	TresholdBox	(QWidget *parent = 0, bool simple = true);
	virtual		~TresholdBox(void);

	bool		IsValidated	(void) const;
	int			GetMin		(void) const;
	int			GetMax		(void) const;

public slots:

	void SetMin(int value);
	void SetMax(int value);

	void OnValidate(void);
	void OnCancel(void);

private:

	Ui::TresholdBox*	m_ui;
	bool				m_validated;
	int					m_min;
	int					m_max;

};

#endif // TRESHOLDBOX_H
