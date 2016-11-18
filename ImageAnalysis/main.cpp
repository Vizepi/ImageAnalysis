#include "MainWindow.h"
#include <QApplication>

#include "Convolution.h"
#include <QImage>

int main(int argc, char *argv[])
{

	/*Convolution::Image* img = Convolution::LoadImage("Data/g");
	Convolution::Filter filter = { 3, nullptr, 6 };
	filter.kernel = new double[9];
	filter.kernel[0] = filter.kernel[2] = filter.kernel[6] = filter.kernel[8] = 0.0;
	filter.kernel[1] = filter.kernel[3] = filter.kernel[5] = filter.kernel[7] = -1.0;
	filter.kernel[4] = 4;

	Convolution::Image* out = Convolution::ApplyFilter(*img, filter, Convolution::Filter::SideHandle::Continuous);

	Convolution::SaveImage(*out, "Out/base_tower.png");

	delete [] filter.kernel;
	delete out;
	delete img;*/

	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
