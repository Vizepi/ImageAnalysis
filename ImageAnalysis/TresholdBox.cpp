#include "TresholdBox.h"
#include "ui_TresholdBox.h"

TresholdBox::TresholdBox(QWidget *parent, bool simple)
	: QDialog(parent)
	, m_ui(new Ui::TresholdBox)
	, m_validated(false)
	, m_min(0)
	, m_max(255)
{
	m_ui->setupUi(this);

	connect(m_ui->ok_button, SIGNAL(clicked()), this, SLOT(OnValidate()));
	connect(m_ui->cancel_button, SIGNAL(clicked()), this, SLOT(OnCancel()));

	connect(m_ui->spin_min, SIGNAL(valueChanged(int)), this, SLOT(SetMin(int)));
	connect(m_ui->slide_min, SIGNAL(valueChanged(int)), this, SLOT(SetMin(int)));
	SetMin(0);

	if(simple)
	{
		m_ui->spin_max->setVisible(false);
		m_ui->label_1->setVisible(false);
		m_ui->slide_max->setVisible(false);
		m_ui->label_0->setText("Threshold");
	}
	else
	{
		connect(m_ui->spin_max, SIGNAL(valueChanged(int)), this, SLOT(SetMax(int)));
		connect(m_ui->slide_max, SIGNAL(valueChanged(int)), this, SLOT(SetMax(int)));
		SetMax(255);
	}
}

TresholdBox::~TresholdBox(void)
{
	delete m_ui;
}

bool TresholdBox::IsValidated(void) const
{
	return m_validated;
}

int TresholdBox::GetMin(void) const
{
	return m_min;
}

int TresholdBox::GetMax(void) const
{
	return m_max;
}

void TresholdBox::SetMin(int value)
{
	m_ui->slide_min->setValue(value);
	m_ui->spin_min->setValue(value);
	m_min = value;
}

void TresholdBox::SetMax(int value)
{
	m_ui->slide_max->setValue(value);
	m_ui->spin_max->setValue(value);
	m_max = value;
}

void TresholdBox::OnValidate(void)
{
	m_validated = true;
	close();
}

void TresholdBox::OnCancel(void)
{
	close();
}
