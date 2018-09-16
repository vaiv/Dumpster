/*
    Copyright (C) 2016  Vaibhav Darbari

    OpenANPR is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenANPR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"charExtraction.h"

charExtraction::charExtraction(Mat src,int idx)
{
	this->idx=idx;
	this->src=src;
	imgArea=src.rows*src.cols;
	res="NA";
	cout<<"beginning enhancement"<<endl;
	enhanceImage();
        cout<<"enhancement complete"<<endl;
	ocr();	
	
}
void equalise(Mat &bgr_image)
{
    
    cv::Mat lab_image;
    cv::cvtColor(bgr_image, lab_image, CV_BGR2Lab);

    // Extract the L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);  // now we have the L image in lab_planes[0]

    // apply the CLAHE algorithm to the L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4);
    cv::Mat dst;
    clahe->apply(lab_planes[0], dst);

    // Merge the the color planes back into an Lab image
    dst.copyTo(lab_planes[0]);
    cv::merge(lab_planes, lab_image);

   // convert back to RGB
   cv::Mat image_clahe;
   cv::cvtColor(lab_image, bgr_image, CV_Lab2BGR);
}
Mat charExtraction::Cluster(Mat img)
{

	Mat3b imlab;
	
    	cvtColor(img, imlab, CV_BGR2Lab);
	
    	/* Cluster image */
    	vector<cv::Mat1b> imgRGB;
    	int k = 3;
    	int n = img.rows * img.cols;
    	Mat img3xN(n, 3, CV_8U);

    	split(imlab, imgRGB);
	
	
    	for (int i = 0; i != 3; ++i)
        	imgRGB[i].reshape(1, n).copyTo(img3xN.col(i));
	

    	img3xN.convertTo(img3xN, CV_32F);

    	vector<int> bestLables;
    	kmeans(img3xN, k, bestLables, cv::TermCriteria(), 10, cv::KMEANS_RANDOM_CENTERS);
	cout<<"clustering done"<<endl;
    	/* Find the largest cluster*/
    	int max = 0, indx= 0, id = 0;
    	vector<int> clusters(k,0);

    	for (size_t i = 0; i < bestLables.size(); i++)
    		{
        		id = bestLables[i];
        		clusters[id]++;

        		if (clusters[id] > max)
        		{
            			max = clusters[id];
            			indx = id;
        		}
    		}
	

    /* save largest cluster */
    	int cluster = indx;

    	vector<Point> shape; 
    	shape.reserve(2000);

    	for (int y = 0; y < imlab.rows; y++)
    	{
        	for (int x = 0; x < imlab.cols; x++)
        	{
            		if (bestLables[y*imlab.cols + x] == cluster) 
            			{
                			shape.emplace_back(x, y);
            			}
        	}
    	}

    	int inc = shape.size();
	
    // Show results
    	Mat3b res(img.size(), Vec3b(0,0,0));
    	vector<Vec3b> colors;
    	for(int i=0; i<k; ++i)
    	{
        	if(i == indx) {
            		colors.push_back(Vec3b(255,255,255));
        	} else {
            		colors.push_back(Vec3b(0,0,0));
        		}
    	}

    	for(int r=0; r<img.rows; ++r)
    	{
        	for(int c=0; c<img.cols; ++c)
        	{
           		 res(r,c) = colors[bestLables[r*imlab.cols + c]];
        	}
    	}

	
    	imshow("Clustering", res);
//    	waitKey(0);
	return res;

	
}

vector<Mat> charExtraction::detectBlobs(Mat &src)
{
	Mat edges,LPlate;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	vector<Mat> chars,charMasks;
	vector <Rect> allPossChars;
        vector <Point> found_centers;
	if(LPlate.channels()>1)
	cvtColor( src, LPlate, CV_BGR2GRAY );
	else LPlate = src.clone();
	
	Canny( LPlate, edges, 70 , 210 , 3 );
//	imshow("char edges", edges);
//	waitKey(0);
	findContours( edges, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	std::sort(contours.begin(), contours.end(), contour_sorter());
	cout<<"num of possibilities"<<contours.size()<<endl;
	for(int i=0;i<contours.size();i++)
	{
		Moments mu;
		mu = moments(contours[i],false);
		Point poss_char_center = cv::Point(mu.m10/mu.m00,mu.m01/mu.m00);
		Rect poss_char = boundingRect(Mat(contours[i]));
		bool repeat = false;
		for(int z=0;z<found_centers.size();z++)
			if(sqrt(pow(found_centers[z].x - poss_char_center.x,2) + pow(found_centers[z].y - poss_char_center.y,2)) < 20 )
			{	
				repeat =true;
				cout<<"---------------repeat found---------------"<<endl;
				break;
			}
		if(!repeat && poss_char.height>poss_char.width && poss_char.height >= charPlateRatio*LPlate.size().height && poss_char.area()<areaTolerance*LPlate.size().height*LPlate.size().width)
			{
				Mat poss = src(poss_char).clone();
				//create a uniform square frame around possible character for CNN input
				Mat sq_mask = Mat(poss.rows*1.3,poss.rows*1.3,CV_8UC3,cvScalar(255,255,255));
				poss.copyTo(sq_mask(Rect(poss.rows*0.15+((poss.rows-poss.cols)/2),poss.rows*0.15,poss.cols,poss.rows)));
				//add the possible character
				chars.push_back(poss.clone());
				charMasks.push_back(sq_mask.clone());
				//add the roi of the character
				allPossChars.push_back(poss_char);
				imwrite("./res/char"+to_string(i)+".png",sq_mask);
//				cout<<"showing possibilities"<<poss.rows<<" "<<poss.cols<<" "<<poss.channels()<<endl;
//				imshow("extracted char",poss);
//				waitKey(0);
				found_centers.push_back(poss_char_center);
			}
	}

	src = cvScalar(255,255,255);
	for(int i=0;i<allPossChars.size();i++)
	{
		//copy characters back to their respective regions.
		chars[i].copyTo(src(allPossChars[i]));
	}
	return charMasks;
	
}

void charExtraction::enhanceImage()
{
	//cvtColor(src,src,CV_BGR2GRAY);
	//equalise(src);
	if(imgArea > minArea)
        {
		resize(src, src, Size(), 4,4, INTER_CUBIC );
		src=Cluster(src);
	}
	else
	{
		resize(src, src, Size(), 4,4, INTER_CUBIC );
	}
	//TODO Blob detection thresholding area and inertia for removal of non characters
	
//	src = cvScalar(255,255,255) - src;
	
	
	
   	//dilation helps in certain cases
	int dilation_size=1;
        Mat element = getStructuringElement( MORPH_ELLIPSE,
                                            Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                            Point( dilation_size, dilation_size ) );
	if(imgArea > minArea)
        dilate( src,src,element);

	cout<<"starting blob detection"<<endl;
	if(imgArea > minArea)
	charMasks = detectBlobs(src);

	//erode( src,src,element);
	cvtColor(src, src, CV_BGR2RGB);
	imwrite("./res/tmp.jpg",src);
//	imshow("ww",src);
//	waitKey(0);
}

Mat charExtraction::SWT()
{
	Mat swt_output = textDetection(src, 1);
	return swt_output;
} 

void charExtraction::ocr()
{
	//Stroke Width Transform
	Mat swt_output;//=SWT();
	Mat sub;
	if(imgArea<minArea)
	{
		sub=src;
		cout<<"area low res"<<endl;
	}
	else 
	{
		sub=src;
		cout<<"area high res"<<endl;
	}

	stringstream ss;
	ss<<idx;
	path= string("./res/swt_out")+ ss.str()+string(".jpg");
	imwrite(path.c_str(),sub);

	//tesseract OCR
	const char* out;
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	if (api->Init(NULL, "eng")) 
		{
        		fprintf(stderr, "Could not initialize tesseract.\n");
        		exit(1);
    		}

	//tesseract::TessBaseAPI tess; 
	//tess.SetImage((uchar*)src.data, src.size().width, src.size().height, src.channels(), src.step1());
	//tess.Recognize(0);

	Pix *image = pixRead(path.c_str());
    	api->SetImage(image);
    	// Get OCR result
    	out = api->GetUTF8Text();
	//const char* out = tess.GetUTF8Text();
	res=string(out);
	


}


string charExtraction::getResultString()
{
	if(!res.empty())
	return res;
	else return string("NA");
}

vector<Mat> charExtraction::getCharMasks()
{
	return charMasks;
}
