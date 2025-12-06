#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;


static void magSpectrums( cv::InputArray _src, cv::OutputArray _dst)
{
    cv::Mat src = _src.getMat();
    int depth = src.depth(), cn = src.channels(), type = src.type();
    int rows = src.rows, cols = src.cols;
    int j, k;

    CV_Assert( type == CV_32FC1 || type == CV_32FC2 || type == CV_64FC1 || type == CV_64FC2 );

    if(src.depth() == CV_32F)
        _dst.create( src.rows, src.cols, CV_32FC1 );
    else
        _dst.create( src.rows, src.cols, CV_64FC1 );

    cv::Mat dst = _dst.getMat();
    dst.setTo(0);//Mat elements are not equal to zero by default!

    bool is_1d = (rows == 1 || (cols == 1 && src.isContinuous() && dst.isContinuous()));

    if( is_1d )
        cols = cols + rows - 1, rows = 1;

    int ncols = cols*cn;
    int j0 = cn == 1;
    int j1 = ncols - (cols % 2 == 0 && cn == 1);

    if( depth == CV_32F )
    {
        const float* dataSrc = src.ptr<float>();
        float* dataDst = dst.ptr<float>();

        size_t stepSrc = src.step/sizeof(dataSrc[0]);
        size_t stepDst = dst.step/sizeof(dataDst[0]);

        if( !is_1d && cn == 1 )
        {
            for( k = 0; k < (cols % 2 ? 1 : 2); k++ )
            {
                if( k == 1 )
                    dataSrc += cols - 1, dataDst += cols - 1;
                dataDst[0] = (float)std::abs(dataSrc[0]);
                if( rows % 2 == 0 )
                    dataDst[(rows-1)*stepDst] = (float)std::abs(dataSrc[(rows-1)*stepSrc]);

                for( j = 1; j <= rows - 2; j += 2 )
                {
                    dataDst[j*stepDst] = (float)std::sqrt((double)dataSrc[j*stepSrc]*dataSrc[j*stepSrc] +
                                                          (double)dataSrc[(j+1)*stepSrc]*dataSrc[(j+1)*stepSrc]);
                }

                if( k == 1 )
                    dataSrc -= cols - 1, dataDst -= cols - 1;
            }
        }

        for( ; rows--; dataSrc += stepSrc, dataDst += stepDst )
        {
            if( is_1d && cn == 1 )
            {
                dataDst[0] = (float)std::abs(dataSrc[0]);
                if( cols % 2 == 0 )
                    dataDst[j1] = (float)std::abs(dataSrc[j1]);
            }

            for( j = j0; j < j1; j += 2 )
            {
                dataDst[j] = (float)std::sqrt((double)dataSrc[j]*dataSrc[j] + (double)dataSrc[j+1]*dataSrc[j+1]);
            }
        }
    }
    else
    {
        const double* dataSrc = src.ptr<double>();
        double* dataDst = dst.ptr<double>();

        size_t stepSrc = src.step/sizeof(dataSrc[0]);
        size_t stepDst = dst.step/sizeof(dataDst[0]);

        if( !is_1d && cn == 1 )
        {
            for( k = 0; k < (cols % 2 ? 1 : 2); k++ )
            {
                if( k == 1 )
                    dataSrc += cols - 1, dataDst += cols - 1;
                dataDst[0] = std::abs(dataSrc[0]);
                if( rows % 2 == 0 )
                    dataDst[(rows-1)*stepDst] = std::abs(dataSrc[(rows-1)*stepSrc]);

                for( j = 1; j <= rows - 2; j += 2 )
                {
                    dataDst[j*stepDst] = std::sqrt(dataSrc[j*stepSrc]*dataSrc[j*stepSrc] +
                                                   dataSrc[(j+1)*stepSrc]*dataSrc[(j+1)*stepSrc]);
                }

                if( k == 1 )
                    dataSrc -= cols - 1, dataDst -= cols - 1;
            }
        }

        for( ; rows--; dataSrc += stepSrc, dataDst += stepDst )
        {
            if( is_1d && cn == 1 )
            {
                dataDst[0] = std::abs(dataSrc[0]);
                if( cols % 2 == 0 )
                    dataDst[j1] = std::abs(dataSrc[j1]);
            }

            for( j = j0; j < j1; j += 2 )
            {
                dataDst[j] = std::sqrt(dataSrc[j]*dataSrc[j] + dataSrc[j+1]*dataSrc[j+1]);
            }
        }
    }
}



static void fftShift(cv::InputOutputArray _out)
{
    cv::Mat out = _out.getMat();

    if(out.rows == 1 && out.cols == 1)
    {
        // trivially shifted.
        return;
    }

    std::vector<cv::Mat> planes;
    split(out, planes);

    int xMid = out.cols >> 1;
    int yMid = out.rows >> 1;

    bool is_1d = xMid == 0 || yMid == 0;

    if(is_1d)
    {
        int is_odd = (xMid > 0 && out.cols % 2 == 1) || (yMid > 0 && out.rows % 2 == 1);
        xMid = xMid + yMid;

        for(size_t i = 0; i < planes.size(); i++)
        {
            cv::Mat tmp;
            cv::Mat half0(planes[i], cv::Rect(0, 0, xMid + is_odd, 1));
            cv::Mat half1(planes[i], cv::Rect(xMid + is_odd, 0, xMid, 1));

            half0.copyTo(tmp);
            half1.copyTo(planes[i](cv::Rect(0, 0, xMid, 1)));
            tmp.copyTo(planes[i](cv::Rect(xMid, 0, xMid + is_odd, 1)));
        }
    }
    else
    {
        int isXodd = out.cols % 2 == 1;
        int isYodd = out.rows % 2 == 1;
        for(size_t i = 0; i < planes.size(); i++)
        {
            // perform quadrant swaps...
            cv::Mat q0(planes[i], cv::Rect(0,    0,    xMid + isXodd, yMid + isYodd));
            cv::Mat q1(planes[i], cv::Rect(xMid + isXodd, 0,    xMid, yMid + isYodd));
            cv::Mat q2(planes[i], cv::Rect(0,    yMid + isYodd, xMid + isXodd, yMid));
            cv::Mat q3(planes[i], cv::Rect(xMid + isXodd, yMid + isYodd, xMid, yMid));

            if(!(isXodd || isYodd))
            {
                cv::Mat tmp;
                q0.copyTo(tmp);
                q3.copyTo(q0);
                tmp.copyTo(q3);

                q1.copyTo(tmp);
                q2.copyTo(q1);
                tmp.copyTo(q2);
            }
            else
            {
                cv::Mat tmp0, tmp1, tmp2 ,tmp3;
                q0.copyTo(tmp0);
                q1.copyTo(tmp1);
                q2.copyTo(tmp2);
                q3.copyTo(tmp3);

                tmp0.copyTo(planes[i](cv::Rect(xMid, yMid, xMid + isXodd, yMid + isYodd)));
                tmp3.copyTo(planes[i](cv::Rect(0, 0, xMid, yMid)));

                tmp1.copyTo(planes[i](cv::Rect(0, yMid, xMid, yMid + isYodd)));
                tmp2.copyTo(planes[i](cv::Rect(xMid, 0, xMid + isXodd, yMid)));
            }
        }
    }

    merge(planes, out);
}

static cv::Point2d weightedCentroid(cv::InputArray _src, cv::Point peakLocation, cv::Size weightBoxSize, double* response)
{
    cv::Mat src = _src.getMat();

    int type = src.type();
    CV_Assert( type == CV_32FC1 || type == CV_64FC1 );

    int minr = peakLocation.y - (weightBoxSize.height >> 1);
    int maxr = peakLocation.y + (weightBoxSize.height >> 1);
    int minc = peakLocation.x - (weightBoxSize.width  >> 1);
    int maxc = peakLocation.x + (weightBoxSize.width  >> 1);

    cv::Point2d centroid;
    double sumIntensity = 0.0;

    // clamp the values to min and max if needed.
    if(minr < 0)
    {
        minr = 0;
    }

    if(minc < 0)
    {
        minc = 0;
    }

    if(maxr > src.rows - 1)
    {
        maxr = src.rows - 1;
    }

    if(maxc > src.cols - 1)
    {
        maxc = src.cols - 1;
    }

    if(type == CV_32FC1)
    {
        const float* dataIn = src.ptr<float>();
        dataIn += minr*src.cols;
        for(int y = minr; y <= maxr; y++)
        {
            for(int x = minc; x <= maxc; x++)
            {
                centroid.x   += (double)x*dataIn[x];
                centroid.y   += (double)y*dataIn[x];
                sumIntensity += (double)dataIn[x];
            }

            dataIn += src.cols;
        }
    }
    else
    {
        const double* dataIn = src.ptr<double>();
        dataIn += minr*src.cols;
        for(int y = minr; y <= maxr; y++)
        {
            for(int x = minc; x <= maxc; x++)
            {
                centroid.x   += (double)x*dataIn[x];
                centroid.y   += (double)y*dataIn[x];
                sumIntensity += dataIn[x];
            }

            dataIn += src.cols;
        }
    }

    if(response)
        *response = sumIntensity;

    sumIntensity += DBL_EPSILON; // prevent div0 problems...

    centroid.x /= sumIntensity;
    centroid.y /= sumIntensity;

    return centroid;
}

cv::Point2d My_phaseCorrelate(cv::InputArray _src1, cv::InputArray _src2, cv::InputArray _window, double* response)
{
    cv::Mat src1 = _src1.getMat();
    cv::Mat src2 = _src2.getMat();
    cv::Mat window = _window.getMat();

    CV_Assert( src1.type() == src2.type());
    CV_Assert( src1.type() == CV_32FC1 || src1.type() == CV_64FC1 );
    CV_Assert( src1.size == src2.size);

    cv::Mat FFT1, FFT2, P, Pm, C;
    int M = cv::getOptimalDFTSize(src1.rows);
    int N = cv::getOptimalDFTSize(src1.cols);

    cout << "M: " << M << std::endl;
    cout << "N: " << N << std::endl;

    // execute phase correlation equation
    // Reference: http://en.wikipedia.org/wiki/Phase_correlation
    dft(src1, FFT1, cv::DFT_REAL_OUTPUT);
    dft(src2, FFT2, cv::DFT_REAL_OUTPUT);
    // dft(src1, FFT1, cv::DFT_COMPLEX_OUTPUT);
    // dft(src2, FFT2, cv::DFT_COMPLEX_OUTPUT);

    cv::mulSpectrums(FFT1, FFT2, P, 0, true);

    magSpectrums(P, Pm);
    cv::divSpectrums(P, Pm, C, 0, false); // FF* / |FF*| (phase correlation equation completed here...)

    cv::idft(C, C); // gives us the nice peak shift location...

    cv::imwrite("cv_C.pfm",C);

    fftShift(C); // shift the energy to the center of the frame.

    cv::imwrite("cv_C_shift.pfm",C);

    cout <<"type: " << C.type() << endl;
    // cv::Mat read;
    // read = cv::imread("cv_C_shift.pfm",cv::IMREAD_ANYDEPTH);
    // read.convertTo(C, CV_32F); 

    // locate the highest peak
    cv::Point peakLoc;
    cv::minMaxLoc(C, NULL, NULL, NULL, &peakLoc);

    cout <<"peakLoc: ()" << peakLoc.x << "," <<peakLoc.y <<"" << endl;

    // get the phase shift with sub-pixel accuracy, 5x5 window seems about right here...
    cv::Point2d t;
    t = weightedCentroid(C, peakLoc, cv::Size(5, 5), response);

    // max response is M*N (not exactly, might be slightly larger due to rounding errors)
    if(response)
        *response /= M*N;

    // adjust shift relative to image center...
    cv::Point2d center((double)src1.cols / 2.0, (double)src2.rows / 2.0);

    return (center - t);
}

int main()
{
    int shift_x = 10;
    int shift_y = 50;
    int split_x = 2;
    int split_y = 2;

    cv::Mat img_1 = cv::imread("../GoogleColab/img_1_256_256.pfm",cv::IMREAD_GRAYSCALE);
    cv::Mat img_2 = cv::imread("../GoogleColab/img_2_256_256.pfm",cv::IMREAD_GRAYSCALE);

    cv::Mat img1_f32,img2_f32;
    img_1.convertTo(img1_f32, CV_32F); 
    img_2.convertTo(img2_f32, CV_32F); 

    // ---- 全体の位相相関 ----
    cv::Point2d full_shift;
    double full_resp;
    // full_shift= cv::phaseCorrelate(img1_f32, img2_f32, cv::noArray(), &full_resp);
    full_shift= My_phaseCorrelate(img1_f32, img2_f32, cv::noArray(), &full_resp);

    cout << "full shift=(" << full_shift.x << ", " << full_shift.y << "), response=" << full_resp << std::endl;

    // ---- タイルごとの位相相関 ----
    img_1 = cv::imread("../GoogleColab/img_1.pfm",cv::IMREAD_GRAYSCALE);
    img_2 = cv::imread("../GoogleColab/img_2.pfm",cv::IMREAD_GRAYSCALE);
    img_1.convertTo(img1_f32, CV_32F); 
    img_2.convertTo(img2_f32, CV_32F); 
    
    int h = img1_f32.rows;
    int w = img1_f32.cols;

    int tile_w = w / split_x;
    int tile_h = h / split_y;

    for (int ty = 0; ty < split_y; ty++) {
        for (int tx = 0; tx < split_x; tx++) {
            int y0 = ty * tile_h;
            int x0 = tx * tile_w;

            cv::Rect roi(x0, y0, tile_w, tile_h);
            cv::Mat tile1 = img1_f32(roi);
            cv::Mat tile2 = img2_f32(roi);

            cv::Point2d s;
            double r = 0;
            s = cv::phaseCorrelate(tile1, tile2, cv::noArray(),&r);
            
            std::cout << "tile(" << tx << "," << ty
                      << ") shift=(" << s.x << ", " << s.y
                      << "), response=" << r << std::endl;
        }
    }

    return 0;
}