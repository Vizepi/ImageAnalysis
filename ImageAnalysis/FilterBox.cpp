#include "FilterBox.h"
#include "ui_FilterBox.h"

#include <QDoubleValidator>
#include <QRect>
#include <QMessageBox>

FilterBox::FilterBox(QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::FilterBox)
	, m_filters(nullptr)
	, m_name()
{
	m_ui->setupUi(this);
	FillKernel();

	connect(m_ui->size_box, SIGNAL(valueChanged(int)), this, SLOT(OnSizeChanged(int)));
	connect(m_ui->default_divisor_button, SIGNAL(clicked()), this, SLOT(OnDefaultDivisor()));
	connect(m_ui->ok_button, SIGNAL(clicked()), this, SLOT(OnValidate()));
	connect(m_ui->cancel_button, SIGNAL(clicked()), this, SLOT(OnCancel()));

	OnSizeChanged(3);
}

FilterBox::~FilterBox()
{
	delete m_ui;
}

void FilterBox::Initialize(QMap<QString, Convolution::Filter>*	filters, const QString& modify)
{
	m_filters = filters;
	QMap<QString, Convolution::Filter>::iterator it = filters->find(modify);
	if(it != filters->end())
	{
		m_name = modify;
		m_modifying = true;
		OnSizeChanged(it->size);
		m_ui->divisor_box->setValue(it->divisor);
		m_ui->name_edit->setText(modify);
		m_ui->name_edit->setDisabled(true);
		int index = 0;
		for(int i = 0; i < 81; ++i)
		{
			m_kernel[i]->setText("0");
			if((abs(i % 9 - 4) <= (it->size / 2)) && (abs(i / 9 - 4) <= (it->size / 2)))
			{
				m_kernel[i]->setText(QString::number(it->kernel[index]));
				index++;
			}
		}
	}
	else
	{
		m_modifying = false;
		OnSizeChanged(3);
		m_kernel[40]->setText("1");
		OnDefaultDivisor();
	}
}

const QString& FilterBox::GetName(void) const
{
	return m_name;
}

void FilterBox::FillKernel(void)
{
	m_kernel.clear();
	m_kernel.resize(81);
#define ADD_TO_K(n) m_kernel[n-1] = m_ui->lineEdit_##n; \
	m_kernel[n-1]->setValidator(new QDoubleValidator(this)); \
	m_kernel[n-1]->setGeometry(QRect(10 + ((n-1) % 9) * 40, 20 + ((n-1) / 9) * 40, 31, 31)); \
	m_kernel[n-1]->setText("0")
	ADD_TO_K(1); ADD_TO_K(2); ADD_TO_K(3); ADD_TO_K(4); ADD_TO_K(5); ADD_TO_K(6); ADD_TO_K(7); ADD_TO_K(8); ADD_TO_K(9);
	ADD_TO_K(10);ADD_TO_K(11);ADD_TO_K(12);ADD_TO_K(13);ADD_TO_K(14);ADD_TO_K(15);ADD_TO_K(16);ADD_TO_K(17);ADD_TO_K(18);
	ADD_TO_K(19);ADD_TO_K(20);ADD_TO_K(21);ADD_TO_K(22);ADD_TO_K(23);ADD_TO_K(24);ADD_TO_K(25);ADD_TO_K(26);ADD_TO_K(27);
	ADD_TO_K(28);ADD_TO_K(29);ADD_TO_K(30);ADD_TO_K(31);ADD_TO_K(32);ADD_TO_K(33);ADD_TO_K(34);ADD_TO_K(35);ADD_TO_K(36);
	ADD_TO_K(37);ADD_TO_K(38);ADD_TO_K(39);ADD_TO_K(40);ADD_TO_K(41);ADD_TO_K(42);ADD_TO_K(43);ADD_TO_K(44);ADD_TO_K(45);
	ADD_TO_K(46);ADD_TO_K(47);ADD_TO_K(48);ADD_TO_K(49);ADD_TO_K(50);ADD_TO_K(51);ADD_TO_K(52);ADD_TO_K(53);ADD_TO_K(54);
	ADD_TO_K(55);ADD_TO_K(56);ADD_TO_K(57);ADD_TO_K(58);ADD_TO_K(59);ADD_TO_K(60);ADD_TO_K(61);ADD_TO_K(62);ADD_TO_K(63);
	ADD_TO_K(64);ADD_TO_K(65);ADD_TO_K(66);ADD_TO_K(67);ADD_TO_K(68);ADD_TO_K(69);ADD_TO_K(70);ADD_TO_K(71);ADD_TO_K(72);
	ADD_TO_K(73);ADD_TO_K(74);ADD_TO_K(75);ADD_TO_K(76);ADD_TO_K(77);ADD_TO_K(78);ADD_TO_K(79);ADD_TO_K(80);ADD_TO_K(81);
}

void FilterBox::OnSizeChanged(int size)
{
	size = (size / 2) * 2 + 1; // Keep odd size
	m_ui->size_box->setValue(size);
	for(int i = 0; i < 81; ++i)
	{
		if(abs(i % 9 - 4) > (size / 2))
		{
			m_kernel[i]->setDisabled(true);
		}
		else if(abs(i / 9 - 4) > (size / 2))
		{
			m_kernel[i]->setDisabled(true);
		}
		else
		{
			m_kernel[i]->setEnabled(true);
		}
	}
}

void FilterBox::OnDivisorChanged(double divisor)
{

}

void FilterBox::OnDefaultDivisor(void)
{
	int size = m_ui->size_box->value();
	double divisor = 0.0;
	for(int i = 0; i < 81; ++i)
	{
		if((abs(i % 9 - 4) <= (size / 2)) || (abs(i / 9 - 4) <= (size / 2)))
		{
			divisor += fabs(m_kernel[i]->text().toDouble());
		}
	}
	if(divisor == 0.0)
	{
		divisor = 1.0;
	}
	m_ui->divisor_box->setValue(divisor);
}

void FilterBox::OnValidate(void)
{
	Convolution::Filter filter;
	Convolution::Filter* pFilter = &filter;
	if(!m_modifying)
	{
		if(m_filters->contains(m_ui->name_edit->text()))
		{
			QMessageBox::information(this, tr("Filter exists"), tr("A filter with the given name already exists."), QMessageBox::Ok);
			return;
		}
		m_name = m_ui->name_edit->text();
	}
	else
	{
		pFilter = &((*m_filters)[m_name]);
		delete [] pFilter->kernel;
	}
	pFilter->size = m_ui->size_box->value();
	pFilter->kernel = new double[pFilter->size * pFilter->size];

	int index = 0;
	for(int i = 0; i < 81; ++i)
	{
		if((abs(i % 9 - 4) <= (pFilter->size / 2)) && (abs(i / 9 - 4) <= (pFilter->size / 2)))
		{
			pFilter->kernel[index] = m_kernel[i]->text().toDouble();
			index++;
		}
	}

	pFilter->divisor = m_ui->divisor_box->text().toDouble();
	if(pFilter->divisor == 0.0)
	{
		OnDefaultDivisor();
		pFilter->divisor = m_ui->divisor_box->text().toDouble();
	}
	if(!m_modifying)
	{
		(*m_filters)[m_name] = filter;
	}
	close();
}

void FilterBox::OnCancel(void)
{
	if(QMessageBox::Yes == QMessageBox::question(this, tr("Cancel?"), tr("Do you really want to cancel the filter creation ?")))
	{
		close();
	}
}
