#ifndef FILTERBOX_H
#define FILTERBOX_H

#include "Convolution.h"

#include <QDialog>
#include <QLineEdit>
#include <QVector>
#include <QMap>
#include <QString>

namespace Ui {
class FilterBox;
}

class FilterBox : public QDialog
{
	Q_OBJECT

public:

	explicit FilterBox(QWidget *parent = 0);
	virtual ~FilterBox(void);

	void Initialize(QMap<QString, Convolution::Filter>*	filters, const QString& modify = QString());
	const QString& GetName(void) const;

	void FillKernel(void);

public slots:

	void OnSizeChanged(int size);
	void OnDivisorChanged(double divisor);
	void OnDefaultDivisor(void);

	void OnValidate(void);
	void OnCancel(void);

private:

	Ui::FilterBox*						m_ui;
	QVector<QLineEdit*>					m_kernel;
	QMap<QString, Convolution::Filter>*	m_filters;
	QString								m_name;
	bool								m_modifying;

};

#endif // FILTERBOX_H
