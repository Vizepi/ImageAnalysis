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


/*static*/ Convolution::Filter* Convolution::Rotate(const Convolution::Filter& filter)
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

/*static*/ QImage Convolution::ToQImage(const Convolution::Image& image)
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

//
//
// COHEN-SUTHERLAND ALGORITHM TO CLIP LINE IN IMAGE

typedef int OutCode;

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)

// ASSUME THAT xmax, xmin, ymax and ymin are global constants.

OutCode ComputeOutCode(double x, double y, double xmin, double xmax, double ymin, double ymax)
{
	OutCode code;

	code = INSIDE;          // initialised as being inside of [[clip window]]

	if (x < xmin)           // to the left of clip window
		code |= LEFT;
	else if (x > xmax)      // to the right of clip window
		code |= RIGHT;
	if (y < ymin)           // below the clip window
		code |= BOTTOM;
	else if (y > ymax)      // above the clip window
		code |= TOP;

	return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
// diagonal from (xmin, ymin) to (xmax, ymax).
bool CohenSutherlandLineClip(	double x0, double y0, double x1, double y1,
								double&x0o,double&y0o,double&x1o,double&y1o,
								double xmin, double xmax, double ymin, double ymax)
{
	// compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
	OutCode outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
	OutCode outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
	bool accept = false;

	while (true) {
		if (!(outcode0 | outcode1)) { // Bitwise OR is 0. Trivially accept and get out of loop
			accept = true;
			break;
		} else if (outcode0 & outcode1) { // Bitwise AND is not 0. Trivially reject and get out of loop
			break;
		} else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			double x, y;

			// At least one endpoint is outside the clip rectangle; pick it.
			OutCode outcodeOut = outcode0 ? outcode0 : outcode1;

			// Now find the intersection point;
			// use formulas y = y0 + slope * (x - x0), x = x0 + (1 / slope) * (y - y0)
			if (outcodeOut & TOP) {           // point is above the clip rectangle
				x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
				y = ymax;
			} else if (outcodeOut & BOTTOM) { // point is below the clip rectangle
				x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
				y = ymin;
			} else if (outcodeOut & RIGHT) {  // point is to the right of clip rectangle
				y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
				x = xmax;
			} else if (outcodeOut & LEFT) {   // point is to the left of clip rectangle
				y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
				x = xmin;
			}

			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == outcode0) {
				x0 = x;
				y0 = y;
				outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
			} else {
				x1 = x;
				y1 = y;
				outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
			}
		}
	}
	if (accept) {
		x0o = x0;
		y0o = y0;
		x1o = x1;
		y1o = y1;
	}
	return accept;
}

// END COHEN-SUTHERLAND ALGORITHM
//
//

void SetPixel(Convolution::Image* image, int x, int y, int color)
{
	if(x < 0 || x >= image->width || y < 0 || y >= image->height)
	{
		return;
	}
	switch(image->format)
	{
	case Convolution::Image::Format::RGB:
		image->pixels[(x + y * image->width) * 3] =		(uint8_t)((color & 0xff0000) >> 16);
		image->pixels[(x + y * image->width) * 3 + 1] = (uint8_t)((color & 0xff00) >> 8);
		image->pixels[(x + y * image->width) * 3 + 2] = (uint8_t)((color & 0xff));
		break;
	case Convolution::Image::Format::ARGB:
		image->pixels[(x + y * image->width) * 4] = 0xff;
		image->pixels[(x + y * image->width) * 4 + 1] = (uint8_t)((color & 0xff0000) >> 16);
		image->pixels[(x + y * image->width) * 4 + 2] = (uint8_t)((color & 0xff00) >> 8);
		image->pixels[(x + y * image->width) * 4 + 3] = (uint8_t)((color & 0xff));
		break;
	case Convolution::Image::Format::Indexed8:
		image->pixels[x + y * image->width] = color & 0xff;
		break;
	}
}

//
//
// BRESENHAM ALGORITHM TO DRAW A LINE

void Bresenham(int x1, int y1, int const x2, int const y2, int color, Convolution::Image* image)
{
	int delta_x(x2 - x1);
	// if x1 == x2, then it does not matter what we set here
	signed char const ix((delta_x > 0) - (delta_x < 0));
	delta_x = std::abs(delta_x) << 1;

	int delta_y(y2 - y1);
	// if y1 == y2, then it does not matter what we set here
	signed char const iy((delta_y > 0) - (delta_y < 0));
	delta_y = std::abs(delta_y) << 1;

	SetPixel(image, x1, y1, color);

	if (delta_x >= delta_y)
	{
		// error may go below zero
		int error(delta_y - (delta_x >> 1));

		while (x1 != x2)
		{
			if ((error >= 0) && (error || (ix > 0)))
			{
				error -= delta_x;
				y1 += iy;
			}
			// else do nothing

			error += delta_y;
			x1 += ix;

			SetPixel(image, x1, y1, color);
		}
	}
	else
	{
		// error may go below zero
		int error(delta_x - (delta_y >> 1));

		while (y1 != y2)
		{
			if ((error >= 0) && (error || (iy > 0)))
			{
				error -= delta_y;
				x1 += ix;
			}
			// else do nothing

			error += delta_x;
			y1 += iy;

			SetPixel(image, x1, y1, color);
		}
	}
}
#include <iostream>
/*static*/ Convolution::Image* Convolution::Hough(const Convolution::Image& in, const Convolution::Image& original, Convolution::Image** accumulator, int alphaPrecision, int treshold, int maximas, int lineColor)
{
	int accHeight = (sqrt(2.0) * (double)(in.height > in.width ? in.height : in.width)) / 2.0;
	int accHeight2 = accHeight * 2.0;
	int size = alphaPrecision * accHeight2;
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
	Convolution::Image* out = new Convolution::Image(original);

	if(maximas > 9)
	{
		maximas = 9;
	}
	else if(maximas < 1)
	{
		maximas = 1;
	}

	maximas /= 2;

	for(int rotation = 0; rotation < accHeight2; ++rotation)
	{
		for(int alpha = 0; alpha < alphaPrecision; ++alpha)
		{
			if(accu[alpha + rotation * alphaPrecision] >= treshold)
			{
				int max = accu[(rotation * alphaPrecision) + alpha];
				for(int ly = -maximas; ly <= maximas; ++ly)
				{
					 for(int lx = -maximas; lx <= maximas; ++lx)
					 {
						  if((ly + rotation >= 0 && ly + rotation < accHeight2) && (lx + alpha >= 0 && lx + alpha < alphaPrecision))
						  {
							  int index = ((rotation + ly) * alphaPrecision) + (alpha + lx);
							   if((int)accu[index] > max)
							   {
									max = accu[index];
									ly = lx = maximas + 1; // break all loops
							   }
						  }
					 }
				}
				if(max > accu[alpha + rotation * alphaPrecision])
				{
					continue;
				}
				int x1, y1, x2, y2;
				x1 = y1 = x2 = y2 = 0;
				double alphaRad = ConvertAngleDtoR(alpha);

				if(alpha >= 45 && alpha <= 135)
				{
					 x1 = 0;
					 y1 = ((double)(rotation - (accHeight2 / 2.0)) - (((double)x1 - (out->width / 2.0) ) * cos(alphaRad))) / sin(alphaRad) + (out->height / 2.0);
					 x2 = out->width;
					 y2 = ((double)(rotation - (accHeight2 / 2.0)) - (((double)x2 - (out->width / 2.0) ) * cos(alphaRad))) / sin(alphaRad) + (out->height / 2.0);
				}
				else
				{
					 y1 = 0;
					 x1 = ((double)(rotation - (accHeight2 / 2.0)) - (((double)y1 - (out->height / 2.0) ) * sin(alphaRad))) / cos(alphaRad) + (out->width / 2.0);
					 y2 = out->height;
					 x2 = ((double)(rotation - (accHeight2 / 2.0)) - (((double)y2 - (out->height / 2.0) ) * sin(alphaRad))) / cos(alphaRad) + (out->width / 2.0);
				}

				double drawX1 = 0, drawX2 = 0, drawY1 = 0, drawY2 = 0;
				if(CohenSutherlandLineClip(x1, y1, x2, y2, drawX1, drawY1, drawX2, drawY2, 0, out->width, 0, out->height))
				{
					Bresenham(drawX1, drawY1, drawX2, drawY2, lineColor, out);
					std::cout << rotation << " " << alpha << std::endl;
				}
			}
		}
	}

	*accumulator = new Convolution::Image(alphaPrecision, accHeight * 2.0, Convolution::Image::Format::Indexed8);

	double multiplier = 255.0 / maxHough;
	for(int i = 0; i < size; ++i)
	{
		(*accumulator)->pixels[i] = accu[i] * multiplier;
	}

	return out;
}
