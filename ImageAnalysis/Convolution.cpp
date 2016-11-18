#include "Convolution.h"

#include <QImage>
#include <QMessageBox>
#include <QVector>

/*static*/ Convolution::Image* Convolution::ApplyFilter(const Convolution::Image& image, const Convolution::Filter& filter, Convolution::Filter::SideHandle sideHandle, bool multi)
{
	// Multiconvolution case
	if(multi)
	{
		Convolution::Image* out = new Convolution::Image();
		out->format = image.format;

		if(sideHandle == Convolution::Filter::SideHandle::Crop)
		{
			out->width = (image.width - (filter.size - 1));
			out->height = (image.height - (filter.size - 1));
		}
		else
		{
			out->width = image.width;
			out->height = image.height;
		}
		uint32_t pixelSize = Convolution::PixelSize(image.format);
		uint32_t imageSize = out->width * out->height * pixelSize;
		out->pixels = new uint8_t[imageSize];

		Convolution::Image* buffers[8];
		buffers[0] = ApplyFilter(image, filter, sideHandle, false);
		Convolution::Filter* rotated = Rotate(filter);
		for(int i = 1; i < 8; ++i)
		{
			buffers[i] = ApplyFilter(image, *rotated, sideHandle, false);
			if(i != 7)
			{
				Convolution::Filter* tmpFilter = rotated;
				rotated = Rotate(*rotated);
				delete tmpFilter;
			}
		}
		delete rotated;
		for(uint32_t i = 0; i < imageSize; ++i)
		{
			out->pixels[i] = 0;
			for(uint32_t j = 0; j < 8; ++j)
			{
				if(buffers[j]->pixels[i] > out->pixels[i])
				{
					out->pixels[i] = buffers[j]->pixels[i];
				}
			}
		}
		for(uint32_t i = 0; i < 8; ++i)
		{
			delete buffers[i];
		}
		return out;
	}
	else
	{
		//
		// Create new image
		Convolution::Image* out = new Convolution::Image();
		out->format = image.format;

		uint32_t bufferWidth, bufferHeight;
		uint8_t* buffer = nullptr;

		uint32_t convStart;

		//
		// Select pixel size and fill indexed color tables
		uint32_t pixelSize = Convolution::PixelSize(image.format);
		if(image.format == Convolution::Image::Format::Indexed8)
		{
			for(uint32_t i = 0; i < 256; ++i)
			{
				out->colorTable[i] = image.colorTable[i];
			}
		}

		//
		// Select buffer size and output image size
		if(sideHandle == Convolution::Filter::SideHandle::Crop)
		{
			out->width = (image.width - (filter.size - 1));
			out->height = (image.height - (filter.size - 1));
			bufferWidth = image.width;
			bufferHeight = image.height;
			convStart = 0;
		}
		else
		{
			out->width = image.width;
			out->height = image.height;
			bufferWidth = image.width + filter.size - 1;
			bufferHeight = image.height + filter.size - 1;
			convStart = filter.size / 2;
		}

		//
		// Create buffers
		buffer = new uint8_t[bufferWidth * bufferHeight * pixelSize];
		out->pixels = new uint8_t[out->width * out->height * pixelSize];

		//
		// Copy image to buffer
		for(uint32_t j = 0; j < image.height; ++j)
		{
			for(uint32_t i = 0; i < image.width; ++i)
			{
				for(uint32_t k = 0; k < pixelSize; ++k)
				{
					uint8_t pixelData = image.pixels[pixelSize * (i + j * image.width) + k];
					buffer[pixelSize * (convStart + i + (j + convStart) * image.width) + k] = pixelData;
				}
			}
		}

		//
		// Wrap sides according to side handle
		if(sideHandle != Convolution::Filter::SideHandle::Crop)
		{
			for(uint32_t j = 0; j < convStart; ++j)
			{
				for(uint32_t i = 0; i < image.width; ++i)
				{
					for(uint32_t k = 0; k < pixelSize; ++k)
					{
						switch(sideHandle)
						{
						case Convolution::Filter::SideHandle::Zeros:
							buffer[pixelSize * (i + j * image.width) + k] = 0;
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = 0;
							break;
						case Convolution::Filter::SideHandle::Black:
							buffer[pixelSize * (i + j * image.width) + k] = 0;
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = 0;
							if(k == 0 && image.format == Convolution::Image::Format::ARGB)
							{
								buffer[pixelSize * (i + j * image.width) + k] = 0xff;
								buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = 0xff;
							}
							break;
						case Convolution::Filter::SideHandle::Ones:
						case Convolution::Filter::SideHandle::White:
							buffer[pixelSize * (i + j * image.width) + k] = 0xff;
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = 0xff;
							break;
						case Convolution::Filter::SideHandle::Continuous:
							buffer[pixelSize * (i + j * image.width) + k] = image.pixels[pixelSize * i + k];
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = image.pixels[pixelSize * (i + (image.height - 1) * image.width) + k];
							break;
						case Convolution::Filter::SideHandle::Mirror:
							buffer[pixelSize * (i + j * image.width) + k] = image.pixels[pixelSize * (i + (convStart - j) * image.width) + k];
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = image.pixels[pixelSize * (i + (image.height - 1 - convStart + j) * image.width) + k];
							break;
						case Convolution::Filter::SideHandle::Repeat:
							buffer[pixelSize * (i + j * image.width) + k] = image.pixels[pixelSize * (i + (image.height - 1 - convStart + j) * image.width) + k];
							buffer[pixelSize * (i + (image.height + convStart + convStart - 1 - j) * image.width) + k] = image.pixels[pixelSize * (i + (convStart - j) * image.width) + k];
							break;
						default:
							break;
						}
					}
				}
				uint32_t bufferHeight = image.height + convStart + convStart;
				uint32_t bufferWidth = image.width + convStart + convStart;
				for(uint32_t i = 0; i < bufferHeight; ++i)
				{
					for(uint32_t k = 0; k < pixelSize; ++k)
					{
						switch(sideHandle)
						{
						case Convolution::Filter::SideHandle::Zeros:
							buffer[pixelSize * (j + i * image.width) + k] = 0;
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = 0;
							break;
						case Convolution::Filter::SideHandle::Black:
							buffer[pixelSize * (j + i * image.width) + k] = 0;
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = 0;
							if(k == 0 && image.format == Convolution::Image::Format::ARGB)
							{
								buffer[pixelSize * (j + i * image.width) + k] = 0xff;
								buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = 0xff;
							}
							break;
						case Convolution::Filter::SideHandle::Ones:
						case Convolution::Filter::SideHandle::White:
							buffer[pixelSize * (j + i * image.width) + k] = 0xff;
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = 0xff;
							break;
						case Convolution::Filter::SideHandle::Continuous:
							buffer[pixelSize * (j + i * image.width) + k] = buffer[pixelSize * (convStart + i * image.width) + k];
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = buffer[pixelSize * (bufferWidth - 1 - convStart + i * image.width) + k];
							break;
						case Convolution::Filter::SideHandle::Mirror:
							buffer[pixelSize * (j + i * image.width) + k] = buffer[pixelSize * (convStart + convStart - j + i * image.width) + k];
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = buffer[pixelSize * (bufferWidth - 1 - convStart - convStart + j + i * image.width) + k];
							break;
						case Convolution::Filter::SideHandle::Repeat:
							buffer[pixelSize * ((bufferWidth - j - 1) + i * image.width) + k] = buffer[pixelSize * (convStart + convStart - j + i * image.width) + k];
							buffer[pixelSize * (j + i * image.width) + k] = buffer[pixelSize * (bufferWidth - 1 - convStart - convStart + j + i * image.width) + k];
							break;
						default:
							break;
						}
					}
				}
			}
		}

		//
		// Apply convolution
		for(uint32_t j = 0; j < out->height; ++j)
		{
			for(uint32_t i = 0; i < out->width; ++i)
			{
				for(uint32_t k = 0; k < pixelSize; ++k)
				{
					double acc = 0.0;
					double divisor = 0.0;
					for(uint32_t x = 0; x < filter.size; ++x)
					{
						for(uint32_t y = 0; y < filter.size; ++y)
						{
							acc += buffer[pixelSize * ((i + x) + (j + y) * out->width) + k] * filter.kernel[x + y * filter.size];
							divisor += fabs(filter.kernel[x + y * filter.size]);
						}
					}
					if(filter.divisor == 0.0)
					{
						out->pixels[pixelSize * (i + j * out->width) + k] = (uint8_t)(acc / divisor);
					}
					else
					{
						out->pixels[pixelSize * (i + j * out->width) + k] = (uint8_t)(acc / filter.divisor);
					}
				}
			}
		}


		//
		// Clean temporaries and return result image
		delete [] buffer;
		return out;
	}
}

void PermuteMatrix(double* in, double* out, int size, int* permutation)
{
	for(int i = 0; i < size; ++i)
	{
		out[i] = in[permutation[i]];
	}
}

int perm3 [9] = {
	3, 0, 1,
	6, 4, 2,
	7, 8, 5 };

int perm5 [25] = {
	10,  5,  0,  1,  2,
	15, 11,  6,  7,  3,
	20, 16, 12,  8,  4,
	21, 17, 18, 13,  9,
	22, 23, 24, 19, 14 };

int perm7 [49] = {
	21, 14,  7,  0,  1,  2,  3,
	28, 22, 15,  8,  9, 10,  4,
	35, 29, 23, 16, 17, 11,  5,
	42, 36, 30, 24, 18, 12,  6,
	43, 37, 31, 32, 25, 19, 13,
	44, 38, 39, 40, 33, 26, 20,
	45, 46, 47, 48, 41, 34, 27 };

int perm9 [81] = {
	21, 14,  7,  0,  1,  2,  3,
	28, 22, 15,  8,  9, 10,  4,
	35, 29, 23, 16, 17, 11,  5,
	42, 36, 30, 24, 18, 12,  6,
	43, 37, 31, 32, 25, 19, 13,
	44, 38, 39, 40, 33, 26, 20,
	45, 46, 47, 48, 41, 34, 27 };


/*static*/ Convolution::Filter* Convolution::Rotate(const Filter& filter)
{
	Convolution::Filter* out = new Convolution::Filter();
	out->divisor = filter.divisor;
	out->size = filter.size;
	out->kernel = new double[out->size * out->size];
	switch(out->size)
	{
	case 1:
		out->kernel[0] = filter.kernel[0];
		break;
	case 3:
		PermuteMatrix(filter.kernel, out->kernel, 9, perm3);
		break;
	case 5:
		PermuteMatrix(filter.kernel, out->kernel, 25, perm5);
		break;
	case 7:
		PermuteMatrix(filter.kernel, out->kernel, 49, perm7);
		break;
	case 9:
		PermuteMatrix(filter.kernel, out->kernel, 81, perm9);
		break;
	}
	return out;
}

/*static*/ Convolution::Image* Convolution::Refine(const Convolution::Image& tresholded, const Convolution::Image& gradient)
{
	Convolution::Image* out = new Convolution::Image(tresholded.width, tresholded.height, Convolution::Image::Format::Indexed8);

	for(int j = 0; j < (int)out->height; ++j)
	{
		for(int i = 0; i < (int)out->width; ++i)
		{
			if(tresholded.pixels[(i + j * out->width)] != 0)
			{
				bool found = false;
				for(int x = -1; x < 2 && !found; ++x)
				{
					for(int y = -1; y < 2 && !found; ++y)
					{
						if(i + x < 0 || i + x >= (int)out->width || j + y < 0 || j + y >= (int)out->height)
						{
							continue;
						}
						if(tresholded.pixels[(i + x) + (j + y) * out->width] != 0 && gradient.pixels[(i + x) + (j + y) * out->width] > gradient.pixels[i + j * out->width])
						{
							found = true;
						}
					}
				}
				if(found)
				{
					out->pixels[i + j * out->width] = 0;
				}
				else
				{
					out->pixels[i + j * out->width] = 255;
				}
			}
		}
	}

	return out;
}

/*static*/ Convolution::Image* Convolution::ToGrayScale(const Convolution::Image& in)
{
	Convolution::Image* out = new Convolution::Image(in.width, in.height, Convolution::Image::Format::Indexed8);
	uint8_t gray = 0;
	for(int j = 0; j < (int)out->height; ++j)
	{
		for(int i = 0; i < (int)out->width; ++i)
		{
			switch(in.format)
			{
			case Convolution::Image::Format::RGB:
				gray = GrayScale(
							in.pixels[(i + j * out->width) * 3],
							in.pixels[(i + j * out->width) * 3 + 1],
							in.pixels[(i + j * out->width) * 3 + 2]);
				out->pixels[i + j * out->width] = gray;
				break;
			case Convolution::Image::Format::ARGB:
				gray = GrayScale(
							in.pixels[(i + j * out->width) * 4 + 1],
							in.pixels[(i + j * out->width) * 4 + 2],
							in.pixels[(i + j * out->width) * 4 + 3]);
				out->pixels[i + j * out->width] = gray;
				break;
			case Convolution::Image::Format::Indexed8:
				out->pixels[i + j * out->width] = in.pixels[i + j * out->width];
				break;
			}
		}
	}
	return out;
}

void LoadRGB32(Convolution::Image** out, const QImage& in)
{
	*out = new Convolution::Image(in.width(), in.height(), Convolution::Image::Format::RGB);
	const uchar* bits = in.constBits();
	uint32_t size = (*out)->width * (*out)->height * 3;
	for(uint32_t i = 0; i < size; ++i)
	{
		(*out)->pixels[i] = bits[((i / 3) * 4) + 2 - (i % 3)];
	}
}

void LoadARGB32(Convolution::Image** out, const QImage& in)
{
	*out = new Convolution::Image(in.width(), in.height(), Convolution::Image::Format::ARGB);
	const uchar* bits = in.constBits();
	uint32_t size = (*out)->width * (*out)->height * 4;
	memcpy((*out)->pixels, bits, size);
}

void LoadIndexed8(Convolution::Image** out, const QImage& in)
{
	*out = new Convolution::Image(in.width(), in.height(), Convolution::Image::Format::Indexed8);
	const uchar* bits = in.constBits();
	uint32_t size = (*out)->width * (*out)->height;
	memcpy((*out)->pixels, bits, size);
}

/*static*/ Convolution::Image* Convolution::LoadImage(const std::string& file)
{
	Convolution::Image* out = nullptr;
	
	QImage in(file.c_str());

	switch(in.format())
	{
	case QImage::Format_RGB32:
		LoadRGB32(&out, in);
		break;
	case QImage::Format_ARGB32:
		LoadARGB32(&out, in);
		break;
	case QImage::Format_Indexed8:
		LoadIndexed8(&out, in);
		break;
	default:
		QMessageBox::warning(nullptr, QObject::tr("Unsupported format"), QObject::tr("The image format is not supported by application"), QMessageBox::Ok);
		break;
	}
	
	return out;
}

/*static*/ QImage Convolution::ToQImage(const Image& image)
{
	QImage out;
	QVector<QRgb> colorTable(256);
	uint32_t dimension = image.width * image.height;
	uint32_t size = dimension * Convolution::PixelSize(image.format);
	uchar* buffer = nullptr;
	switch(image.format)
	{
	case Convolution::Image::Format::RGB:
		buffer = new uchar[dimension * 4];
		for(uint32_t i = 0; i < dimension; ++i)
		{
			buffer[i * 4 + 3] = 0xff;
		}
		for(uint32_t i = 0; i < size; ++i)
		{
			buffer[((i / 3) * 4) + 2 - (i % 3)] = image.pixels[i];
		}
		out = QImage(image.width, image.height, QImage::Format_RGB32);
		memcpy(out.bits(), buffer, dimension * 4);
		break;
	case Convolution::Image::Format::ARGB:
		buffer = new uchar[dimension * 4];
		memcpy(buffer, image.pixels, size);
		out = QImage(image.width, image.height, QImage::Format_ARGB32);
		memcpy(out.bits(), buffer, dimension * 4);
		break;
	case Convolution::Image::Format::Indexed8:
		buffer = new uchar[dimension];
		memcpy(buffer, image.pixels, size);
		out = QImage(image.width, image.height, QImage::Format_Indexed8);
		memcpy(out.bits(), buffer, dimension);
		for(uint32_t i = 0; i < 256; ++i)
		{
			colorTable[i] = image.colorTable[i];
		}
		out.setColorCount(256);
		out.setColorTable(colorTable);
		break;
	}
	delete buffer;
	return out;
}

/*static*/ void Convolution::SaveImage(const Convolution::Image& image, const std::string& file)
{
	QImage out = ToQImage(image);
	out.save(file.c_str());
}

/*static*/ Convolution::Image* Convolution::Treshold(Convolution::Image* image, int tresholdMin, int tresholdMax)
{
	Convolution::Image* out = new Convolution::Image(image->width, image->height, Convolution::Image::Format::Indexed8);
	int gray;
	for(uint32_t j = 0; j < image->height; ++j)
	{
		for(uint32_t i = 0; i < image->width; ++i)
		{
			switch(image->format)
			{
			case Convolution::Image::Format::RGB:
				gray = GrayScale(
							image->pixels[(i + j * image->width) * 3],
							image->pixels[(i + j * image->width) * 3 + 1],
							image->pixels[(i + j * image->width) * 3 + 2]);
				break;
			case Convolution::Image::Format::ARGB:
				gray = GrayScale(
							image->pixels[(i + j * image->width) * 4 + 1],
							image->pixels[(i + j * image->width) * 4 + 2],
							image->pixels[(i + j * image->width) * 4 + 3]);
				break;
			case Convolution::Image::Format::Indexed8:
				gray = image->pixels[i + j * image->width];
				break;
			}
			if(gray <= tresholdMin)
			{
				out->pixels[i + j * out->width] = 0;
			}
			else if(gray > tresholdMax)
			{
				out->pixels[i + j * out->width] = 255;
			}
			else
			{
				out->pixels[i + j * out->width] = 150;
			}
		}
	}
	if(tresholdMin != tresholdMax)
	{
		for(int j = 0; j < (int)out->height; ++j)
		{
			for(int i = 0; i < (int)out->width; ++i)
			{
				if(out->pixels[(i + j * out->width)] == 150)
				{
					bool found = false;
					for(int x = -1; x < 2 && !found; ++x)
					{
						for(int y = -1; y < 2 && !found; ++y)
						{
							if(i + x < 0 || i + x >= (int)out->width || j + y < 0 || j + y >= (int)out->height)
							{
								continue;
							}
							if(out->pixels[(i + x) + (j + y) * out->width] == 255)
							{
								found = true;
							}
						}
					}
					if(found)
					{
						out->pixels[i + j * out->width] = 200;
					}
					else
					{
						out->pixels[i + j * out->width] = 0;
					}
				}
			}
		}
		for(uint32_t j = 0; j < out->height; ++j)
		{
			for(uint32_t i = 0; i < out->width; ++i)
			{
				if(out->pixels[(i + j * out->width)] == 200)
				{
					out->pixels[i + j * out->width] = 255;
				}
			}
		}
	}
	return out;
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
inline double ConvertAngleDtoR(double alpha)
{
	return alpha * M_PI / 180.0;
}
inline double ConvertAngleRtoD(double alpha)
{
	return alpha / 180.0 / M_PI;
}

/*static*/ Convolution::Image* Convolution::Hough(const Convolution::Image& in, const Convolution::Image& original, Convolution::Image** accumulator, int alphaPrecision, int lineCount)
{
	int accHeight = (sqrt(2.0) * (double)(in.height > in.width ? in.height : in.width)) / 2.0;
	int size = alphaPrecision * accHeight * 2.0;
	int* accu = new int[size];
	for(int i = 0; i< size; ++i)
	{
		accu[i] = 0;
	}

	double centerX = in.width / 2;
	double centerY = in.height / 2;

	int maxHough = 0;

	for(int j = 0; j < in.height; ++j)
	{
		for(int i = 0; i < in.width; ++i)
		{
			if(in.pixels[i + j * in.width] == 255)
			{
				for(int alpha = 0; alpha < alphaPrecision; ++alpha)
				{
					double alphaRad = ConvertAngleDtoR(alpha);
					double rotation = (((double)i - centerX) * cos(alphaRad)) + (((double)j - centerY) * sin(alphaRad));
					int v = ++accu[(int)(round(rotation + accHeight) * alphaPrecision) + alpha];
					if(v > maxHough)
					{
						maxHough = v;
					}
				}
			}
		}
	}
	Convolution::Image* out = new Convolution::Image(in.width, in.height, original.format);

	*accumulator = new Convolution::Image(alphaPrecision, accHeight * 2.0, Convolution::Image::Format::Indexed8);

	double multiplier = 255.0 / maxHough;
	for(int i = 0; i < size; ++i)
	{
		(*accumulator)->pixels[i] = accu[i] * multiplier;
	}

	return out;
}
