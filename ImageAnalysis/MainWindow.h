#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Convolution.h"

#include <QMainWindow>
#include <QImage>
#include <QScrollBar>
#include <QLabel>
#include <QScrollArea>
#include <QStack>
#include <QMap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

private:

	struct Action
	{
		Convolution::Image* result;
		QString action;
		Convolution::Image* gradient;
		bool actionIsGradient;
	};

public:
	explicit MainWindow				(QWidget *parent = 0);
	virtual	~MainWindow				(void);

	void	ClearStack				(QStack<Action>& s);

public slots:

	void	Open					(void);
	void	SaveAs					(void);
	void	Exit					(void);

	void	CreateFilter			(void);
	void	ApplyFilter				(void);
	void	ApplyMultiFilter		(void);
	void	EditFilter				(void);
	void	DeleteFilter			(void);

	void	ApplySimpleThreshold	(void);
	void	ApplyHysteresisThreshold(void);

	void	Refine					(void);
	void	HoughTransform			(void);

	void	Reset					(void);
	void	Undo					(void);
	void	Redo					(void);
	void	Do						(Convolution::Image* image, bool gradient, const QString& action, bool source = false);

	void	About					(void);
	void	AboutQt					(void);

	void	ZoomIn					(void);
	void	ZoomOut					(void);
	void	ZoomZero				(void);

	void	VerticalScroll			(int position);
	void	HorizontalScroll		(int position);

	void	closeEvent				(QCloseEvent * event);

private:

	void	ApplyFilterInternal		(bool multi);
	void	SetImage				(const QImage& newImage, bool source = true);
	void	SetImage				(Convolution::Image* newImage, bool source = true);
	void	ScaleImage				(double factor);
	void	AdjustScrollBar			(QScrollBar* scrollBar, double factor);
	void	UpdateActions			(void);
	void	AddFilter				(const QString& filterName);
	void	CreateDefaultFilters	(void);

	Ui::MainWindow*						m_ui;

	Convolution::Image*					m_gradient;
	Convolution::Image*					m_imageInternal[2];
	QLabel*								m_imageLabel[2];
	QScrollArea*						m_scrollArea[2];
	double								m_scaleFactor;
	QMap<QString, Convolution::Filter>	m_filters;
	QString								m_lastAction;
	QLabel*								m_statusLabel;
	bool								m_actionIsGradient;

	QStack<Action> m_undo;
	QStack<Action> m_redo;

};

#endif // MAINWINDOW_H
