#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QGridLayout>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QCloseEvent>
#include <QMenu>

#include "FilterBox.h"
#include "TresholdBox.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);
	m_statusLabel = new QLabel(this);
	m_statusLabel->setText("No Actions | Gradient calculated : No");
	m_ui->statusBar->addPermanentWidget(m_statusLabel, 1);
	m_gradient = nullptr;
	m_imageInternal[0] = nullptr;
	m_imageInternal[1] = nullptr;
	m_imageLabel[0] = new QLabel();
	m_imageLabel[1] = new QLabel();
	m_scrollArea[0] = new QScrollArea();
	m_scrollArea[1] = new QScrollArea();
	m_scaleFactor = 1.0;
	m_lastAction = "";
	m_actionIsGradient = false;

	m_imageLabel[0]->setBackgroundRole(QPalette::Base);
	m_imageLabel[0]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_imageLabel[0]->setScaledContents(true);

	m_scrollArea[0]->setBackgroundRole(QPalette::Dark);
	m_scrollArea[0]->setWidget(m_imageLabel[0]);
	m_scrollArea[0]->setVisible(false);

	m_imageLabel[1]->setBackgroundRole(QPalette::Base);
	m_imageLabel[1]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_imageLabel[1]->setScaledContents(true);

	m_scrollArea[1]->setBackgroundRole(QPalette::Dark);
	m_scrollArea[1]->setWidget(m_imageLabel[1]);
	m_scrollArea[1]->setVisible(false);

	QGridLayout* layout = new QGridLayout();
	layout->addWidget(m_scrollArea[0], 0, 0);
	layout->addWidget(m_scrollArea[1], 0, 1);

	centralWidget()->setLayout(layout);

	connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(Open()));
	connect(m_ui->actionSave_As, SIGNAL(triggered()), this, SLOT(SaveAs()));
	connect(m_ui->actionExit, SIGNAL(triggered()), this, SLOT(Exit()));

	connect(m_ui->actionRefine, SIGNAL(triggered()), this, SLOT(Refine()));

	connect(m_ui->actionReset, SIGNAL(triggered()), this, SLOT(Reset()));
	connect(m_ui->actionUndo, SIGNAL(triggered()), this, SLOT(Undo()));
	connect(m_ui->actionRedo, SIGNAL(triggered()), this, SLOT(Redo()));

	connect(m_ui->actionCreate, SIGNAL(triggered()), this, SLOT(CreateFilter()));
	connect(m_ui->actionSimple, SIGNAL(triggered()), this, SLOT(ApplySimpleThreshold()));
	connect(m_ui->actionHysteresis, SIGNAL(triggered()), this, SLOT(ApplyHysteresisThreshold()));

	connect(m_scrollArea[0]->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(VerticalScroll(int)));
	connect(m_scrollArea[1]->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(VerticalScroll(int)));
	connect(m_scrollArea[0]->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(HorizontalScroll(int)));
	connect(m_scrollArea[1]->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(HorizontalScroll(int)));

	connect(m_ui->actionAbout, SIGNAL(triggered()), this, SLOT(About()));
	connect(m_ui->actionAbout_Qt, SIGNAL(triggered()), this, SLOT(AboutQt()));

	CreateDefaultFilters();
}

MainWindow::~MainWindow(void)
{
	delete m_ui;
	for(QMap<QString, Convolution::Filter>::iterator it = m_filters.begin();it != m_filters.end(); ++it)
	{
		delete [] it->kernel;
	}
	if(m_imageInternal[0] != nullptr)
	{
		delete m_imageInternal[0];
	}
	if(m_imageInternal[1] != nullptr)
	{
		delete m_imageInternal[1];
	}
	ClearStack(m_undo);
	ClearStack(m_redo);
}

void MainWindow::ClearStack(QStack<MainWindow::Action>& s)
{
	while(s.size() > 0)
	{
		if(s.top().result != nullptr)
		{
			if(s.top().gradient != nullptr && !s.top().actionIsGradient)
			{
				delete s.top().gradient;
			}
			delete s.top().result;
		}
		s.pop();
	}
}

void MainWindow::Open(void)
{
	if(m_imageInternal[1] != nullptr && QMessageBox::No == QMessageBox::warning(this, tr("Open ?"), tr("There is a result image. Are you sure you want to open an other image ?"), QMessageBox::Yes, QMessageBox::No))
	{
		return;
	}
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), tr("./"), "Images (*.png *.jpg *.jpeg *.bmp *.gif);;All files (*.*)");
	if(!fileName.isNull())
	{
		ClearStack(m_undo);
		ClearStack(m_redo);

		Convolution::Image* newImage = Convolution::LoadImage(fileName.toStdString());
		if (newImage == nullptr) {
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
									 tr("Cannot load %1")
									 .arg(QDir::toNativeSeparators(fileName)));
			return;
		}
		Do(newImage, false, QString("Open image: \"") + fileName + QString("\""), true);
		delete newImage;
	}
}

void MainWindow::SaveAs(void)
{
	if(m_imageInternal[1] == nullptr)
	{
		return;
	}
	//TODO : optimize for outputing the same format as input
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), tr("./"),
													"Portable Network Graphics (*.png);;JPEG (*.jpg *.jpeg);;Bitmap (*.bmp);;GIF (*.gif);;All files (*.*)");

	if(!fileName.isNull())
	{
		Convolution::SaveImage(*m_imageInternal[1], fileName.toStdString());
	}
}

void MainWindow::Exit(void)
{
	close();
}

void MainWindow::CreateFilter(void)
{
	FilterBox f(this);
	f.Initialize(&m_filters);
	f.setModal(true);
	f.exec();
	QString name = f.GetName();
	if(!name.isNull())
	{
		AddFilter(name);
	}
}

void MainWindow::ApplyFilter(void)
{
	ApplyFilterInternal(false);
}

void MainWindow::ApplyMultiFilter(void)
{
	ApplyFilterInternal(true);
}

void MainWindow::EditFilter(void)
{
	QAction* action = qobject_cast<QAction*>(sender());
	if(action == nullptr)
	{
		return;
	}
	QMenu* menu = qobject_cast<QMenu*>(action->parent());
	if(menu == nullptr)
	{
		return;
	}
	QString filterName = menu->menuAction()->text();
	if(m_filters.contains(filterName))
	{
		FilterBox f(this);
		f.Initialize(&m_filters, filterName);
		f.setModal(true);
		f.exec();
	}
}

void MainWindow::DeleteFilter(void)
{
	QAction* action = qobject_cast<QAction*>(sender());
	if(action == nullptr)
	{
		return;
	}
	QMenu* menu = qobject_cast<QMenu*>(action->parent());
	if(menu == nullptr)
	{
		return;
	}
	QString filterName = menu->menuAction()->text();
	//TODO find and delete
	if(m_filters.contains(filterName))
	{
		m_filters.remove(filterName);
		m_ui->menuApply_filter->removeAction(menu->menuAction());
	}
}

void MainWindow::ApplySimpleThreshold(void)
{
	if(m_imageInternal[1] == nullptr)
	{
		return;
	}
	TresholdBox t(this, true);
	t.setModal(true);
	t.exec();
	if(t.IsValidated())
	{
		Convolution::Image* result = Convolution::Treshold(m_imageInternal[1], t.GetMin(), t.GetMin());
		Do(result, false, "Apply simple treshold");
	}
}

void MainWindow::ApplyHysteresisThreshold(void)
{
	if(m_imageInternal[1] == nullptr)
	{
		return;
	}
	TresholdBox t(this, false);
	t.setModal(true);
	t.exec();
	if(t.IsValidated())
	{
		Convolution::Image* result = Convolution::Treshold(m_imageInternal[1], t.GetMin(), t.GetMax());
		Do(result, false, "Apply hysteresis treshold");
	}
}

void MainWindow::Refine(void)
{
	if(m_imageInternal[1] == nullptr)
	{
		return;
	}

	if(m_gradient == nullptr)
	{
		QMessageBox::information(this, tr("No gradient"), tr("No gradient has been calcualted."), QMessageBox::Ok);
		return;
	}
	Convolution::Image* result = Convolution::Refine(*m_imageInternal[1], *m_gradient);
	Do(result, false, "Refine edges");
}

void MainWindow::Reset(void)
{
	if(m_imageInternal[1] != nullptr)
	{
		if(QMessageBox::Yes == QMessageBox::warning(this, tr("Reset ?"), tr("The Undo/Redo stack will be cleared. Are you sure you want to reset ?"), QMessageBox::Yes, QMessageBox::No))
		{
			while(m_undo.size() > 2)
			{
				if(m_undo.top().result != nullptr)
				{
					delete m_undo.top().result;
				}
				m_undo.pop();
			}
			if(m_undo.size() == 2)
			{
				Undo();
			}
			ClearStack(m_redo);
			m_lastAction = "Reset image";
		}
	}
}

void MainWindow::Undo(void)
{
	if(m_undo.size() <= 1)
	{
		return;
	}
	Action a = { m_imageInternal[1], m_lastAction, m_gradient, m_actionIsGradient };
	m_redo.push(a);
	a = m_undo.pop();
	m_lastAction = a.action;
	m_actionIsGradient = a.actionIsGradient;
	m_gradient = a.gradient;
	UpdateActions();
	SetImage(a.result, false);
}

void MainWindow::Redo(void)
{
	if(m_redo.size() == 0)
	{
		return;
	}
	Action a = { m_imageInternal[1], m_lastAction, m_gradient, m_actionIsGradient };
	m_undo.push(a);
	a = m_redo.pop();
	m_lastAction = a.action;
	m_actionIsGradient = a.actionIsGradient;
	m_gradient = a.gradient;
	UpdateActions();
	SetImage(a.result, false);
}

void MainWindow::Do(Convolution::Image* image, bool gradient, const QString &action, bool source)
{
	ClearStack(m_redo);
	Action a = { m_imageInternal[1], m_lastAction, m_gradient, m_actionIsGradient };
	m_undo.push(a);
	m_lastAction = action;
	m_gradient = gradient ? Convolution::ToGrayScale(*image) : m_gradient;
	m_actionIsGradient = gradient;
	UpdateActions();
	SetImage(image, source);
}

void MainWindow::About(void)
{
	QMessageBox::about(this, tr("About"), tr("<p>Image analysis M2 project - 2016</p><p><b>Chavot Joris</b> - <b>Kieffer Joseph</b></p>"));
}

void MainWindow::AboutQt(void)
{
	QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::ZoomIn(void)
{
	m_scaleFactor *= 1.25;
}

void MainWindow::ZoomOut(void)
{
	m_scaleFactor *= 0.8;
}

void MainWindow::ZoomZero()
{
	m_imageLabel[0]->adjustSize();
	m_scaleFactor = 1.0;
}

void MainWindow::VerticalScroll(int position)
{
	m_scrollArea[0]->verticalScrollBar()->setValue(position);
	m_scrollArea[1]->verticalScrollBar()->setValue(position);
}

void MainWindow::HorizontalScroll(int position)
{
	m_scrollArea[0]->horizontalScrollBar()->setValue(position);
	m_scrollArea[1]->horizontalScrollBar()->setValue(position);
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	if(m_imageInternal[1] != nullptr)
	{
		if(QMessageBox::No == QMessageBox::warning(this, tr("Quit ?"), tr("There is a result image. Are you sure you want to quit ?"), QMessageBox::Yes, QMessageBox::No))
		{
			event->ignore();
		}
		else
		{
			event->accept();
		}
	}
}

void MainWindow::ApplyFilterInternal(bool multi)
{
	if(m_imageInternal[1] == nullptr)
	{
		return;
	}
	QAction* action = qobject_cast<QAction*>(sender());
	if(action == nullptr)
	{
		return;
	}
	QMenu* menu = qobject_cast<QMenu*>(action->parent());
	if(menu == nullptr)
	{
		return;
	}
	QString filterName = menu->menuAction()->text();
	if(m_filters.contains(filterName))
	{
		Convolution::Image* result = Convolution::ApplyFilter(*m_imageInternal[1], m_filters[filterName], Convolution::Filter::SideHandle::Continuous, multi);
		Do(result, true, QString("Apply filter") + (multi ? " (multi):" : ":") + " \"" + filterName + QString("\""));
	}
}

void MainWindow::SetImage(const QImage &newImage, bool source)
{
	if(source)
	{
		m_imageLabel[0]->setPixmap(QPixmap::fromImage(newImage));
		m_imageLabel[0]->adjustSize();
		m_scrollArea[0]->setVisible(true);
	}
	m_imageLabel[1]->setPixmap(QPixmap::fromImage(newImage));
	m_imageLabel[1]->adjustSize();
	m_scrollArea[1]->setVisible(true);
}

void MainWindow::SetImage(Convolution::Image* newImage, bool source)
{
	if(source)
	{
		m_imageInternal[0] = new Convolution::Image(*newImage);
	}
	m_imageInternal[1] = new Convolution::Image(*newImage);
	SetImage(Convolution::ToQImage(*newImage), source);
}

void MainWindow::ScaleImage(double factor)
{

}

void MainWindow::AdjustScrollBar(QScrollBar* scrollBar, double factor)
{

}

void MainWindow::UpdateActions(void)
{
	m_statusLabel->setText(QString(
							   ((m_lastAction == "") ? "No Actions" : ("Last action [ " + m_lastAction + " ]")) +
							   " | Gradient calculated : " + ((m_gradient == nullptr) ? "No" : "Yes")));
}

void MainWindow::AddFilter(const QString& filterName)
{
	QMenu* newMenu = new QMenu(filterName, this);
	m_ui->menuApply_filter->addMenu(newMenu);

	QAction* applyAction = new QAction("Apply", newMenu);
	newMenu->addAction(applyAction);
	connect(applyAction, SIGNAL(triggered()), this, SLOT(ApplyFilter()));

	QAction* multiAction = new QAction("Multiconvolution", newMenu);
	newMenu->addAction(multiAction);
	connect(multiAction, SIGNAL(triggered()), this, SLOT(ApplyMultiFilter()));

	QAction* editAction = new QAction("Edit...", newMenu);
	newMenu->addAction(editAction);
	connect(editAction, SIGNAL(triggered()), this, SLOT(EditFilter()));

	QAction* deleteAction = new QAction("Delete", newMenu);
	newMenu->addAction(deleteAction);
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(DeleteFilter()));
}

void MainWindow::CreateDefaultFilters(void)
{
	Convolution::Filter f;
	f.divisor = 6;
	f.size = 3;
	f.kernel = new double[9];
	f.kernel[0] = f.kernel[2] = f.kernel[6] = f.kernel[8] = 0.0;
	f.kernel[1] = f.kernel[3] = f.kernel[5] = f.kernel[7] = -1.0;
	f.kernel[4] = 4;
	m_filters["Laplacian"] = f;

	f.divisor = 9;
	f.size = 3;
	f.kernel = new double[9];
	f.kernel[0] = f.kernel[1] = f.kernel[2] = f.kernel[3] = f.kernel[4] = f.kernel[5] = f.kernel[6] = f.kernel[7] = f.kernel[8] = 1;
	m_filters["Box Blur"] = f;

	f.divisor = 3;
	f.size = 3;
	f.kernel = new double[9];
	f.kernel[0] = f.kernel[1] = f.kernel[2] = 1;
	f.kernel[3] = f.kernel[4] = f.kernel[5] = 0;
	f.kernel[6] = f.kernel[7] = f.kernel[8] = -1;
	m_filters["Prewitt"] = f;

	f.divisor = 4;
	f.size = 3;
	f.kernel = new double[9];
	f.kernel[0] = f.kernel[2] = 1;
	f.kernel[3] = f.kernel[4] = f.kernel[5] = 0;
	f.kernel[6] = f.kernel[8] = -1;
	f.kernel[1] = 2;
	f.kernel[7] = -2;
	m_filters["Sobel"] = f;

	f.divisor = 15;
	f.size = 3;
	f.kernel = new double[9];
	f.kernel[0] = f.kernel[1] = f.kernel[2] = 5;
	f.kernel[3] = f.kernel[5] = f.kernel[6] = f.kernel[7] = f.kernel[8] = -3;
	f.kernel[4] = 0;
	m_filters["Kirsch"] = f;

	f.divisor = 159;
	f.size = 5;
	f.kernel = new double[25];
	f.kernel[0] = f.kernel[4] = f.kernel[20] = f.kernel[24] = 2;
	f.kernel[1] = f.kernel[3] = f.kernel[5] = f.kernel[9] = f.kernel[15] = f.kernel[19] = f.kernel[21] = f.kernel[23] = 4;
	f.kernel[2] = f.kernel[10] = f.kernel[14] = f.kernel[22] = 5;
	f.kernel[6] = f.kernel[8] = f.kernel[16] = f.kernel[18] = 9;
	f.kernel[7] = f.kernel[11] = f.kernel[13] = f.kernel[17] = 12;
	f.kernel[12] = 15;
	m_filters["Gaussian Blur"] = f;

	for(QMap<QString, Convolution::Filter>::iterator it = m_filters.begin();it != m_filters.end(); ++it)
	{
		AddFilter(it.key());
	}
}
