////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file	Z:\ThesisCode\ARtools\src\LICFs.cpp
///
/// @brief	Implements the lic file system class.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "LICFs.h"
// constructors

LICFs::LICFs(void){

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	LICFs::LICFs(IplImage *image)
///
/// @brief	Constructor.
///
/// @author	Henry.portilla
/// @date	15/05/2012
///
/// @param [in,out]	image	If non-null, the image.
////////////////////////////////////////////////////////////////////////////////////////////////////

LICFs::LICFs(IplImage *image)
{
	// Do a copy of the image to be analyzed
	lines = 0;
	HoughStorage = cvCreateMemStorage(0);
	imageOriginal = cvCloneImage(image);
	imgType = LEFT;
	LICFs_matchCounter = 0;
	EpipolarErrorValue = 100;
	 // matching
	minVal = 0;
	maxVal = 0;
	
	I_height = this->SubImageSize.height;
	I_width = this->SubImageSize.width;
	pt = cvPoint( I_height/2, I_width/2 );
	minLoc = &pt;
	maxLoc = &pt;
	// epipolar constraints
	EpilinesL = cvCreateMat(3,3,CV_32FC1);
	EpilinesR = cvCreateMat(3,3,CV_32FC1);
	cvSetIdentity(EpilinesL);
	cvSetIdentity(EpilinesR);
	

}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	LICFs::~LICFs(void)
///
/// @brief	Destructor.
///
/// @author	Henry.portilla
/// @date	15/05/2012
////////////////////////////////////////////////////////////////////////////////////////////////////

LICFs::~LICFs(void)
{
	// release memory
	
	cvReleaseImage(&imageOriginal);
	cvReleaseImage(&SubImage);		
	cvReleaseImage(&SubImageGray);	
	cvReleaseImage(&HoughSubImage);	

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// @fn	cvReleaseImage(&EdgeSubImage);
	///
	/// @brief	Constructor.
	///
	/// @author	Henry.portilla
	/// @date	15/05/2012
	///
	/// @param [in,out]	EdgeSubImage	The edge sub image.
	////////////////////////////////////////////////////////////////////////////////////////////////////

	cvReleaseImage(&EdgeSubImage);
	//cvReleaseImage(&LICF_feature);
	//cvReleaseImage(&LICF_featureOtherImage);
	cvReleaseMemStorage(&HoughStorage);
	//cvReleaseMat(&matchOnImageResults);
	//cvReleaseImage(&grayImageToMatch);
}

// Get a subImage from an Image to be analyzed
void LICFs::GetSubImage(imagePoints actualImages_Points, float percentage, IMAGE_TYPE imageType){
	
	try{
		// the value chosen was percentage % of image size
		imgType = imageType;
		CvPoint UpperLeft,LowerRight;
		// OpenCV mapping new_y = (h-actualImages_Points.yiL)
		// because OpenGL has the origin at the top left of the image
		// and OpenGL has the origin at the Bottom left of the image
		int new_yiL = imageOriginal->height - actualImages_Points.yiL;
		int new_yiR = imageOriginal->height - actualImages_Points.yiR;
		if (imgType == LEFT){
			UpperLeft = cvPoint((actualImages_Points.xiL - abs((percentage*imageOriginal->width/2))),new_yiL - abs((percentage*imageOriginal->height/2)));
			LowerRight = cvPoint((actualImages_Points.xiL + abs((percentage*imageOriginal->width/2))),new_yiL + abs((percentage*imageOriginal->height/2)));

			// set center of the area to analyze
			SubImageCenter.x = actualImages_Points.xiL;
			SubImageCenter.y = new_yiL;
		}
		else{
			UpperLeft = cvPoint((actualImages_Points.xiR - abs((percentage*imageOriginal->width/2))),new_yiR - abs((percentage*imageOriginal->height/2)));
			LowerRight = cvPoint((actualImages_Points.xiR + abs((percentage*imageOriginal->width/2))),new_yiR + abs((percentage*imageOriginal->height/2)));
			// set center of the area to analyze
			SubImageCenter.x = actualImages_Points.xiR;
			SubImageCenter.y = new_yiR;
		}
		// Rectangle size for cropping the image
		imgSize = cvGetSize(imageOriginal);
		SubImageSize = cvSize((LowerRight.x - UpperLeft.x),(LowerRight.y - UpperLeft.y));
		SubImage = cvCreateImage(SubImageSize,IPL_DEPTH_8U,3);	
		SubImageGray = cvCreateImage(SubImageSize,IPL_DEPTH_8U,1);
		HoughSubImage = cvCreateImage(SubImageSize,IPL_DEPTH_8U,3);	
	    EdgeSubImage = cvCreateImage(SubImageSize,IPL_DEPTH_8U,1);
		// get subImage
		cvGetRectSubPix(imageOriginal,SubImage,SubImageCenter);
		cvCvtColor(SubImage,SubImageGray,CV_BGR2GRAY);
	}
	catch(...){
	}
}

// Get the characteristics of the chosen subarea
SubArea_Structure LICFs::GetSubAreaBoundaries(void){
	
	try{
		// save size of area
		SubAreaLimits.heigh = SubImageSize.height;
		SubAreaLimits.width = SubImageSize.width;
		// save upper left corner
		SubAreaLimits.x_AreaCenter = SubImageCenter.x;
		SubAreaLimits.y_AreaCenter = SubImageCenter.y;
	
		return SubAreaLimits;
	}
	catch(...){
	}

}

// Get the Edges from the image
void LICFs::ApplyCannyEdgeDetector(double CannyWindowSize, double thresholdCannyLow, double thresholdCannyHigh){
	try{
		
		cvCanny(SubImageGray,EdgeSubImage,thresholdCannyLow,thresholdCannyHigh,CannyWindowSize);
		// Show image
		if (imgType == LEFT){
			cvNamedWindow("Edge subImage L");
			cvShowImage("Edge subImage L",EdgeSubImage);
		}else{
			cvNamedWindow("Edge subImage R");
			cvShowImage("Edge subImage R",EdgeSubImage);
		}
		
	}
	catch(...){
	}
}
// Get the Edges from the image using the Edge Drawing method
vector<lineParameters> LICFs::ApplyEdgeDrawingEdgeDetector(int MinLineLength){
	try{
		cv::Mat EdgeTmp;
		std::vector<lineParameters> LineSegments;
		int WindowSize = 7;//5
		float gradient_Threshold = 5.22;
		EDlines Edges(SubImageGray);
		EdgeTmp = Edges.EdgeDrawing(SubImageGray,WindowSize,gradient_Threshold);
		//EdgeSubImage = Edge
		LineSegments = Edges.EdgeFindLines(MinLineLength);
		// Show image
		if (imgType == LEFT){
			cv::imshow("Edge subImage L",EdgeTmp);
		}else{
			cv::imshow("Edge subImage R",EdgeTmp);
		}
		return LineSegments;
	}
	catch(...){
	}
}
// Get the lines from the image
CvSeq* LICFs::ApplyHoughLineDetection(int HoughThreshold, double HoughMinLengthDetection, double HoughMaxGapBetweenLines){
	try{
		
		lines = cvHoughLines2(EdgeSubImage,HoughStorage,CV_HOUGH_PROBABILISTIC,1,CV_PI/180,
			HoughThreshold,HoughMaxGapBetweenLines,HoughMaxGapBetweenLines);
		cvCvtColor(EdgeSubImage, HoughSubImage, CV_GRAY2BGR );

		for (int i=0;i<lines->total;i++){
			CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);
			cvLine(HoughSubImage,line[0],line[1],CV_RGB(255,0,0));
		}
		if (imgType == LEFT){
			cvNamedWindow("Hough subImage L");
			cvShowImage("Hough subImage L",HoughSubImage);
		}else{
			cvNamedWindow("Hough subImage R");
			cvShowImage("Hough subImage R",HoughSubImage);
		}
		return lines;
	}
	catch(...){
	}
}

// get the intersection point between two lines
lineIntersection LICFs::GetLineIntersection(lineParameters Line1, lineParameters Line2){
	try{
		lineIntersection crossPoint;
		// using line intersection method by Paul Borke's web page
		float U_a,U_b = 1;		
		float numerator_U_a = 1, numerator_U_b = 1;
		float denominator_U_a = 1, denominator_U_b = 1;
		float xi=1,yi=1;
		 // calculate intersection check 						 
		 numerator_U_a = (Line2.x2 - Line2.x1)*(Line2.y1 - Line1.y1) -
			 (Line2.x1 - Line1.x1)*(Line2.y2 - Line2.y1);
		 denominator_U_a = (Line2.x2 - Line2.x1)*(Line1.y2 - Line1.y1) -
			 (Line2.y2 - Line2.y1)*(Line1.x2 - Line1.x1);
		 numerator_U_b = (Line2.y1 - Line1.y1)*(Line1.x2 - Line1.x1) -
			 (Line2.x1 - Line1.x1)*(Line1.y2 - Line1.y1);
		 denominator_U_b = denominator_U_a;
		 U_a = numerator_U_a/denominator_U_a;
		 U_b = numerator_U_b/denominator_U_b;
		 // check line status: Parallel, Coincident, not intersecting, intersecting

		 // intersecting case
		 if ((denominator_U_a != 0.0f) & (numerator_U_a != 0.0f) & (numerator_U_b != 0.0f)){

			// intersecting point between line segments
			 //if (((U_a >= 0.0f) & (U_a <= 1.0f) & (U_b >= 0.0f) & (U_b <= 1.0f))){
				
				yi = (Line1.y1 + U_a*(Line1.y2 - Line1.y1));
				xi = (Line1.x1 + U_a*(Line1.x2 - Line1.x1));
			//}
		 }
		 crossPoint.xi = xi;
		 crossPoint.yi = yi;
		 return crossPoint;

	}
	catch(...){
	}


}

// Get the Cross ratio for the LICF

float LICFs::GetCrossRatio(float AngleBetweenLines){
	try{
		// Idea is based on paper Feature Matching by cross ratio invariance by Branca, Pattern Recognition 33(2000)
		// we generate extra lines between the LICF lines to compute a cross ratio
		float alpha_14 = AngleBetweenLines*CV_PI/180;
		float alpha_13 = 0;
		float alpha_23 = 0;
		float alpha_24 = 0;
		float CrossRatio = 0;
		float s13,s24,s23,s14;
		// Algorithm
		// 1. Use the angle between LICF lines as basis angle alpha_14
		// 2. Set alpha_13 = 2/3(alpha_14), alpha_23 = 1/3(alpha_14), alpha_24 = 2/3(alpha_14)
		//		as line separations
		// 3. Find Cross ratio for the lines corresponding to these angles separations
		//		cr = sin(alpha_13)*sin(alpha_24)/sin(alpha_23)*sin(alpha_14)
		//		
		alpha_13 = 2*alpha_14/3;
		alpha_24 = 2*alpha_14/3;
		alpha_23 = alpha_14/3;
		s13 = sin(alpha_13);
		s24 = sin(alpha_24);
		s23 = sin(alpha_23);
		s14 = sin(alpha_14);

		CrossRatio = (s13*s24)/(s23*s14);
		return CrossRatio;
	}
	catch(...){
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	vector<LICFs_Structure> LICFs::ApplyLICF_Detection(std::vector<lineParameters> Actual_Lines,
/// 	int LICF_MaxDistanceBetweenLines)
///
/// @brief	Get the LICFs of the image using lines from the EDlines method
///
/// @date	17/05/2012
///
/// @param	Actual_Lines					The actual detected lines.
/// @param	LICF_MaxDistanceBetweenLines	The LICF maximum distance between lines.
///
/// @return	detected LICFs
////////////////////////////////////////////////////////////////////////////////////////////////////

vector<LICFs_Structure> LICFs::ApplyLICF_Detection(std::vector<lineParameters> Actual_Lines, int LICF_MaxDistanceBetweenLines){
	try{
		// Detect LICFs features		
		// minimum intersection distance in pixels
		// Do a symetric matrix to examine the combinational set of lines
		int LineSize = Actual_Lines.size();
		float isaNewCombination = 0;
		CvMat *LinesMatrix;
		
		float dth_threshold = LICF_MaxDistanceBetweenLines;	 
		lineParameters currentLine;
		LICFs_Structure currentLICF,tmp_currentLICF;

		float d_betweenLines1 = 10;
		float d_betweenLines2 = 10;
		float d_betweenLines3 = 10;
		float d_betweenLines4 = 10;
		float d_Min_betweenLines,d_Min1,d_Min2 = 10;
		lineParameters currentLine1;
		lineParameters currentLine2;
		float thetaAngle_L1 = 0, thetaAngle_L2 = 0;
		float length_L1 = 1, length_L2 = 1;
		
		// check number of lines, to avoid access errors and get at least 1 LICF
		if (LineSize > 1){			// we need at least two lines to form a LICF feature
			LinesMatrix = cvCreateMat(LineSize,LineSize,CV_32FC1);
			cvSetIdentity(LinesMatrix); // we put 1 to the combination already used or not possible
									// as when i = j or (i,j) = (i,j)Transposed at analyzing the for loops
									// 
		}else{
			return Actual_LICFs;
		}

		// Find LICFs intersections		
		// using line intersection method by Paul Borke's web page
		float U_a,U_b = 1;		
		float numerator_U_a = 1, numerator_U_b = 1;
		float denominator_U_a = 1, denominator_U_b = 1;
		float xi,yi;
		float d1_L1,d2_L1,d1_L2,d2_L2;

		for(int i=0; i < LineSize; i++){
			for(int j=0; j < LineSize; j++){
				// check that the checking lines are not the same line
				// at the vector structure, use a symetric matrix to no
				// get repeated values
				isaNewCombination = cvmGet(LinesMatrix,i,j);
				if(isaNewCombination != 1.0f){
					currentLine1 = Actual_Lines.at(i);
					currentLine2 = Actual_Lines.at(j);
					// mark already used combinations
					cvmSet(LinesMatrix,i,j,1.0);
					cvmSet(LinesMatrix,j,i,1.0);
					// check the distance among the 4 line end points of the 2 lines
					// using distance in 2D   
					d_betweenLines1 = sqrt(pow((currentLine1.x1 - currentLine2.x1),2)+ pow((currentLine1.y1 - currentLine2.y1),2));
					d_betweenLines2 = sqrt(pow((currentLine1.x1 - currentLine2.x2),2)+ pow((currentLine1.y1 - currentLine2.y2),2));
					d_betweenLines3 = sqrt(pow((currentLine1.x2 - currentLine2.x1),2)+ pow((currentLine1.y2 - currentLine2.y1),2));
					d_betweenLines4 = sqrt(pow((currentLine1.x2 - currentLine2.x2),2)+ pow((currentLine1.y2 - currentLine2.y2),2));
					// check min value
					d_Min1 = min(d_betweenLines1, d_betweenLines2);
					d_Min2 = min(d_betweenLines3, d_betweenLines4);
					d_Min_betweenLines = min(d_Min1,d_Min2);

					if (d_Min_betweenLines < dth_threshold ){
					 // calculate intersection check 						 
					 numerator_U_a = (currentLine2.x2 - currentLine2.x1)*(currentLine2.y1 - currentLine1.y1) -
						 (currentLine2.x1 - currentLine1.x1)*(currentLine2.y2 - currentLine2.y1);
					 denominator_U_a = (currentLine2.x2 - currentLine2.x1)*(currentLine1.y2 - currentLine1.y1) -
						 (currentLine2.y2 - currentLine2.y1)*(currentLine1.x2 - currentLine1.x1);
					 numerator_U_b = (currentLine2.y1 - currentLine1.y1)*(currentLine1.x2 - currentLine1.x1) -
						 (currentLine2.x1 - currentLine1.x1)*(currentLine1.y2 - currentLine1.y1);
					 denominator_U_b = denominator_U_a;
					 U_a = numerator_U_a/denominator_U_a;
					 U_b = numerator_U_b/denominator_U_b;

					 // check line status: Parallel, Coincident, not intersecting, intersecting

					 // intersecting case
					 if ((denominator_U_a != 0.0f) & (numerator_U_a != 0.0f) & (numerator_U_b != 0.0f)){

						// intersecting point between line segments
						 //if (((U_a >= 0.0f) & (U_a <= 1.0f) & (U_b >= 0.0f) & (U_b <= 1.0f))){
							
							yi = (currentLine1.y1 + U_a*(currentLine1.y2 - currentLine1.y1));
							xi = (currentLine1.x1 + U_a*(currentLine1.x2 - currentLine1.x1));
							// check for values inside the rectangle that defines the area of influence
							// of the 3D pointer
							if ((xi>0)&(xi<SubImageSize.width)&(yi>0)&(yi<SubImageSize.height)){

								// calculate the inclination of the lines
								thetaAngle_L1 = atan((currentLine1.y2 - currentLine1.y1)/(currentLine1.x2 - currentLine1.x1))*180/CV_PI;
								thetaAngle_L2 = atan((currentLine2.y2 - currentLine2.y1)/(currentLine2.x2 - currentLine2.x1))*180/CV_PI;
								currentLine1.thetaAngle = thetaAngle_L1;
								currentLine2.thetaAngle = thetaAngle_L2;
								// calculate the farthest end line points for each LICF line to the intersection point
								
								// for L1
								d1_L1 = sqrt(pow((xi-currentLine1.x1),2) + pow((yi-currentLine1.y1),2));
								d2_L1 = sqrt(pow((xi-currentLine1.x2),2) + pow((yi-currentLine1.y2),2));
								if (d1_L1 >= d2_L1){
									currentLine1.x_farthest = currentLine1.x1;
									currentLine1.y_farthest = currentLine1.y1;
								}
								if (d2_L1 > d1_L1){
									currentLine1.x_farthest = currentLine1.x2;
									currentLine1.y_farthest = currentLine1.y2;
								}
								// get the L1 line length
								length_L1 = sqrt(pow((xi - currentLine1.x_farthest),2)+ pow((yi - currentLine1.y_farthest),2));
								currentLine1.length = length_L1;

								// for L2
								d1_L2 = sqrt(pow((xi-currentLine2.x1),2) + pow((yi-currentLine2.y1),2));
								d2_L2 = sqrt(pow((xi-currentLine2.x2),2) + pow((yi-currentLine2.y2),2));
								if (d1_L2 >= d2_L2){
									currentLine2.x_farthest = currentLine2.x1;
									currentLine2.y_farthest = currentLine2.y1;
								}
								if (d2_L2 > d1_L2){
									currentLine2.x_farthest = currentLine2.x2;
									currentLine2.y_farthest = currentLine2.y2;
								}
								// get the L2 line length
								length_L2 = sqrt(pow((xi - currentLine2.x_farthest),2)+ pow((yi - currentLine2.y_farthest),2));
								currentLine2.length = length_L2;

								// define L1 as the line that always has in this order
								// 1. leftish points 2. lower points from LICF intersection(y axis is inverted)
								
								if(currentLine1.x_farthest < currentLine2.x_farthest){
									currentLICF.L_1 = currentLine1;
									currentLICF.L_2 = currentLine2;
								}else if(currentLine1.y_farthest < currentLine2.y_farthest){
									currentLICF.L_1 = currentLine1;
									currentLICF.L_2 = currentLine2;
								}else{
									currentLICF.L_1 = currentLine2;
									currentLICF.L_2 = currentLine1;
								}
								// get the Cross Ratio for this LICF
								float angleLimits = abs(currentLICF.L_1.thetaAngle - currentLICF.L_2.thetaAngle);
								currentLICF.crossRatio = GetCrossRatio(angleLimits);
								// save LICF
								currentLICF.x_xK = xi;
								currentLICF.y_xK = yi;
								currentLICF.angleBetweenLines = angleLimits;
								// save found LICF
								// check distances to intersection
								if ((d1_L1>0)&(d2_L1>0)&(d1_L2>0)&(d2_L2>0)){
									// limit angles between lines min = 10, max 170
									// to avoid tight lines that also should not
									// contain planes
									
									if ((angleLimits > 10)&(angleLimits < 170)){
										Actual_LICFs.push_back(currentLICF);
									}
								}
							}
						
						// }
					 }						 
		
					}
					// show last found LICF
				/*	if (Actual_LICFs.size() != 0){
						tmp_currentLICF = Actual_LICFs.back();
						cvLine(LICF_LeftSubImage, cvPoint(tmp_currentLICF.L_1.x1,tmp_currentLICF.L_1.y1), cvPoint(tmp_currentLICF.L_1.x2,tmp_currentLICF.L_1.y2), CV_RGB(0,128,255), 1, 8 );
						cvLine(LICF_LeftSubImage, cvPoint(tmp_currentLICF.L_2.x1,tmp_currentLICF.L_2.y1), cvPoint(tmp_currentLICF.L_1.x2,tmp_currentLICF.L_1.y2), CV_RGB(0,255,128), 1, 8 );
						cvCircle(LICF_LeftSubImage,cvPoint(xi,yi),2,CV_RGB(0,0,255),1,3,0);
						
					}*/
				

				}
			}
			
		}

				
		for(int i=0;i<Actual_Lines.size();i++){
			cvLine(HoughSubImage,cvPoint(Actual_Lines.at(i).x1,Actual_Lines.at(i).y1),cvPoint(Actual_Lines.at(i).x2,Actual_Lines.at(i).y2),CV_RGB(255,0,0));
		}
		// Show image
		if (imgType == LEFT){
			cvNamedWindow("EDlines L");
			cvShowImage("EDlines L",HoughSubImage);
		}else{
			cvNamedWindow("EDlines R");
			cvShowImage("EDlines R",HoughSubImage);
		}
 
		return Actual_LICFs;

	}
	catch(...){
	}
}
// get the LICFs of the image using hough lines
vector<LICFs_Structure> LICFs::ApplyLICF_Detection(CvSeq *imageHoughLines, int LICF_MaxDistanceBetweenLines){
	try{
		// Detect LICFs features		
		// minimum intersection distance in pixels
		float dth_threshold = LICF_MaxDistanceBetweenLines;	
		lineParameters currentLine;
		LICFs_Structure currentLICF,tmp_currentLICF;
		
		vector<lineParameters> Actual_Lines;	

		float d_betweenLines1 = 10;
		float d_betweenLines2 = 10;
		float d_betweenLines3 = 10;
		float d_betweenLines4 = 10;
		float d_Min_betweenLines,d_Min1,d_Min2 = 10;
		lineParameters currentLine1;
		lineParameters currentLine2;
		float thetaAngle_L1 = 0, thetaAngle_L2 = 0;

		for( int i = 0; i < imageHoughLines->total; i++ )
        {
            CvPoint* line = (CvPoint*)cvGetSeqElem(imageHoughLines,i);
          
			// save line parameters
			currentLine.x1 = line[1].x;
			currentLine.x2 = line[0].x;
			currentLine.y1 = line[1].y;
			currentLine.y2 = line[0].y;
			Actual_Lines.push_back(currentLine);
		
        }
		// Find LICFs intersections		
		// using line intersection method by Paul Borke's web page
		float U_a,U_b = 1;		
		float numerator_U_a = 1, numerator_U_b = 1;
		float denominator_U_a = 1, denominator_U_b = 1;
		float xi,yi;
		float d1_L1,d2_L1,d1_L2,d2_L2;

		for(int i=0; i < Actual_Lines.size(); i++){
			for(int j=0; j < Actual_Lines.size(); j++){
				// check that the checking lines are not the same line
				// at the vector structure
				if(i != j){
					currentLine1 = Actual_Lines.at(i);
					currentLine2 = Actual_Lines.at(j);
					// check the distance among the 4 line end points of the 2 lines
					// using distance in 2D   
					d_betweenLines1 = sqrt(pow((currentLine1.x1 - currentLine2.x1),2)+ pow((currentLine1.y1 - currentLine2.y1),2));
					d_betweenLines2 = sqrt(pow((currentLine1.x1 - currentLine2.x2),2)+ pow((currentLine1.y1 - currentLine2.y2),2));
					d_betweenLines3 = sqrt(pow((currentLine1.x2 - currentLine2.x1),2)+ pow((currentLine1.y2 - currentLine2.y1),2));
					d_betweenLines4 = sqrt(pow((currentLine1.x2 - currentLine2.x2),2)+ pow((currentLine1.y2 - currentLine2.y2),2));
					// check min value
					d_Min1 = min(d_betweenLines1, d_betweenLines2);
					d_Min2 = min(d_betweenLines3, d_betweenLines4);
					d_Min_betweenLines = min(d_Min1,d_Min2);

					if (d_Min_betweenLines < dth_threshold ){
					 // calculate intersection check 						 
					 numerator_U_a = (currentLine2.x2 - currentLine2.x1)*(currentLine2.y1 - currentLine1.y1) -
						 (currentLine2.x1 - currentLine1.x1)*(currentLine2.y2 - currentLine2.y1);
					 denominator_U_a = (currentLine2.x2 - currentLine2.x1)*(currentLine1.y2 - currentLine1.y1) -
						 (currentLine2.y2 - currentLine2.y1)*(currentLine1.x2 - currentLine1.x1);
					 numerator_U_b = (currentLine2.y1 - currentLine1.y1)*(currentLine1.x2 - currentLine1.x1) -
						 (currentLine2.x1 - currentLine1.x1)*(currentLine1.y2 - currentLine1.y1);
					 denominator_U_b = denominator_U_a;
					 U_a = numerator_U_a/denominator_U_a;
					 U_b = numerator_U_b/denominator_U_b;

					 // check line status: Parallel, Coincident, not intersecting, intersecting

					 // intersecting case
					 if ((denominator_U_a != 0.0f) & (numerator_U_a != 0.0f) & (numerator_U_b != 0.0f)){

						// intersecting point between line segments
						 if (((U_a >= 0.0f) & (U_a <= 1.0f) & (U_b >= 0.0f) & (U_b <= 1.0f))){
							
							yi = (currentLine1.y1 + U_a*(currentLine1.y2 - currentLine1.y1));
							xi = (currentLine1.x1 + U_a*(currentLine1.x2 - currentLine1.x1));
							// calculate the inclination of the lines
							thetaAngle_L1 = atan((currentLine1.y2 - currentLine1.y1)/(currentLine1.x2 - currentLine1.x1))*180/CV_PI;
							thetaAngle_L2 = atan((currentLine2.y2 - currentLine2.y1)/(currentLine2.x2 - currentLine2.x1))*180/CV_PI;
							currentLine1.thetaAngle = thetaAngle_L1;
							currentLine2.thetaAngle = thetaAngle_L2;
							// calculate the farthest end line points for each LICF line
							// for L1
							d1_L1 = sqrt(pow((xi-currentLine1.x1),2) + pow((yi-currentLine1.y1),2));
							d2_L1 = sqrt(pow((xi-currentLine1.x2),2) + pow((yi-currentLine1.y2),2));
							if (d1_L1 >= d2_L1){
								currentLine1.x_farthest = currentLine1.x1;
								currentLine1.y_farthest = currentLine1.y1;
							}
							if (d2_L1 > d1_L1){
								currentLine1.x_farthest = currentLine1.x2;
								currentLine1.y_farthest = currentLine1.y2;
							}
							// for L2
							d1_L2 = sqrt(pow((xi-currentLine2.x1),2) + pow((yi-currentLine2.y1),2));
							d2_L2 = sqrt(pow((xi-currentLine2.x2),2) + pow((yi-currentLine2.y2),2));
							if (d1_L2 >= d2_L2){
								currentLine2.x_farthest = currentLine2.x1;
								currentLine2.y_farthest = currentLine2.y1;
							}
							if (d2_L2 > d1_L2){
								currentLine2.x_farthest = currentLine2.x2;
								currentLine2.y_farthest = currentLine2.y2;
							}
							// save LICF
							currentLICF.x_xK = xi;
							currentLICF.y_xK = yi;
							currentLICF.L_1 = currentLine1;
							currentLICF.L_2 = currentLine2;
							// save found LICF
							if ((d1_L1>0)&(d2_L1>0)&(d1_L2>0)&(d2_L2>0)){
								Actual_LICFs.push_back(currentLICF);
							}
						
						 }
					 }						 
		
					}
					// show last found LICF
				/*	if (Actual_LICFs.size() != 0){
						tmp_currentLICF = Actual_LICFs.back();
						cvLine(LICF_LeftSubImage, cvPoint(tmp_currentLICF.L_1.x1,tmp_currentLICF.L_1.y1), cvPoint(tmp_currentLICF.L_1.x2,tmp_currentLICF.L_1.y2), CV_RGB(0,128,255), 1, 8 );
						cvLine(LICF_LeftSubImage, cvPoint(tmp_currentLICF.L_2.x1,tmp_currentLICF.L_2.y1), cvPoint(tmp_currentLICF.L_1.x2,tmp_currentLICF.L_1.y2), CV_RGB(0,255,128), 1, 8 );
						cvCircle(LICF_LeftSubImage,cvPoint(xi,yi),2,CV_RGB(0,0,255),1,3,0);
						
					}*/

				}
			}
			
		}
		return Actual_LICFs;

	}
	catch(...){
	}
}
// Get current subImageGray
IplImage* LICFs::GetSubImageGray(void){
	try{
		grayImageToMatch = cvCloneImage(SubImageGray);
		return grayImageToMatch;
	}
	catch(...){
	}
}
// Get Normalized cross correlation between two images
double LICFs::GetLICFs_NCC(CvMat *LICF_feature,CvMat *LICF_featureOtherImage){
	try{
		double correlationValue = 0;
		double sumNum = 0,tmpVal = 0,tmpVal2 = 0;
		double sumDen = 0,sumDen1 = 0, sumDen2 = 0;
		CvScalar I_Avg = cvAvg(LICF_feature,NULL);
		int size = LICF_feature->width;
		
		CvMat *L_subImage = cvCloneMat(LICF_feature);
		CvMat *R_subImage = cvCloneMat(LICF_featureOtherImage);
		// get NCC value
		double center,wSize = size/2;
		double centerFrac = modf(wSize,&center);
		CvPoint testPoint = cvPoint((int)center,(int)center);
		for (int i=0;i<size;i++){
			for (int j=0;j<size;j++){
				//
				tmpVal = cvmGet(L_subImage,testPoint.y,testPoint.x);
				tmpVal2 = cvmGet(R_subImage,i,j);
				sumNum = sumNum + tmpVal*tmpVal2;
				sumDen1 = sumDen1 + pow(tmpVal,2);
				sumDen2 = sumDen2 + pow(tmpVal2,2);
			}
		}
		// get final value of NCC for these images
		sumDen = sumDen1*sumDen2;
		correlationValue = sumNum/sqrt(sumDen);
		cvReleaseMat(&L_subImage);
		cvReleaseMat(&R_subImage);
		return correlationValue;
	}
	catch(...){
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	vector<Matching_LICFs> LICFs::ApplyMatchingLICFs(IplImage *SubImageToMatch,
/// 	vector<LICFs_Structure> LICFs_otherImage,float threshold, int Windowsize)
///
/// @brief	Get the matching between L and R LICFs.
///
/// @date	16/05/2012
///
/// @param [in,out]	SubImageToMatch	If non-null, a match specifying the sub image to.
/// @param	LICFs_otherImage	   	The LICF from other image.
/// @param	threshold			   	The threshold.
/// @param	Windowsize			   	The windowsize.
///
/// @return	A vector with the matched LICFs found
////////////////////////////////////////////////////////////////////////////////////////////////////

vector<Matching_LICFs> LICFs::ApplyMatchingLICFs(IplImage *SubImageToMatch,vector<LICFs_Structure> LICFs_otherImage,float threshold, int Windowsize){
	try{
		// for all the LICFS found
		CvSize LICF_imageSize = cvSize(Windowsize,Windowsize);
		LICF_feature = cvCreateMat(LICF_imageSize.height,LICF_imageSize.width,CV_32FC1);
		LICF_featureOtherImage = cvCreateMat(LICF_imageSize.height,LICF_imageSize.width,CV_32FC1);
		// Create a matrix for the correspond match results
		//int widthLICF = (SubImageGray->width - LICF_feature->width + 1);
		//int heighLICF =  (SubImageGray->height - LICF_feature->height +1);
		//matchOnImageResults = cvCreateMat(heighLICF,widthLICF,CV_32FC1);
		Matching_LICFs currentMatched_LICFs;
		LICFs_matchCounter = 0;

		// match via cross ratio first
		float minCrossRatio = 0.05;	
		float deltaCrossRatio = 0;
		float crossRatioL = 0, crossRatioR = 0;
		// line order
		float L1_discriminant = 0;
		float L1_order_Epsilon = 0.1;
		// line orientation
		float delta_L1_orientation = 0;
		float L1_orientationEpsilon = 0;
		float L1_L=0,L1_R=0;

		// find the number of features to compare and apply NCC
		for (int i=0;i < Actual_LICFs.size();i++){ 
			for (int j=0;j< LICFs_otherImage.size();j++){
				// get LICF position
				LICF_FeatureCenter.x = Actual_LICFs.at(i).x_xK;
				LICF_FeatureCenter.y = Actual_LICFs.at(i).y_xK;
				// get LICF position of features in the other image
				LICF_FeatureCenterOtherImage.x = LICFs_otherImage.at(j).x_xK;
				LICF_FeatureCenterOtherImage.y = LICFs_otherImage.at(j).y_xK;
				// crop the LICF as an subimage to look for a match in the other image
				cvGetRectSubPix(SubImageGray,LICF_feature,LICF_FeatureCenter);
				cvGetRectSubPix(SubImageToMatch,LICF_featureOtherImage,LICF_FeatureCenterOtherImage);
				//cvMatchTemplate(LICF_featureOtherImage,LICF_feature,matchOnImageResults,CV_TM_CCORR_NORMED);
				// find NCC between the two LICF subimages
				maxVal = GetLICFs_NCC(LICF_feature,LICF_featureOtherImage);
				// find place of matching in the other image
				//cvMinMaxLoc(matchOnImageResults,&minVal,&maxVal,minLoc,maxLoc);
				
				// match via cross ratio
				crossRatioL = Actual_LICFs.at(i).crossRatio;
				crossRatioR = LICFs_otherImage.at(j).crossRatio; 
				deltaCrossRatio = abs(crossRatioL - crossRatioR); 
				// check correct orientation
				L1_L = Actual_LICFs.at(i).L_1.thetaAngle;
				L1_R = LICFs_otherImage.at(j).L_1.thetaAngle;
				delta_L1_orientation = abs((L1_L + L1_R) - 2*L1_L);
				L1_orientationEpsilon = abs(0.5*(L1_L + L1_R));

				// save matching points
				
				if (deltaCrossRatio < minCrossRatio){
					if (maxVal > threshold){
						if(delta_L1_orientation < L1_orientationEpsilon ){

							LICFs_matchCounter = LICFs_matchCounter + 1;
							// check line order
							L1_discriminant = Actual_LICFs.at(i).L_1.thetaAngle - LICFs_otherImage.at(j).L_1.thetaAngle;
							L1_order_Epsilon = 0.1*Actual_LICFs.at(i).angleBetweenLines;
							if(abs(L1_discriminant) < abs(L1_order_Epsilon)){
								// right LICF
								currentMatched_LICFs.MatchLICFs_R.L_1 = LICFs_otherImage.at(j).L_1;	
								currentMatched_LICFs.MatchLICFs_R.L_2 = LICFs_otherImage.at(j).L_2;
								// left LICF
								currentMatched_LICFs.MatchLICFs_L.L_1 = Actual_LICFs.at(i).L_1;
								currentMatched_LICFs.MatchLICFs_L.L_2 = Actual_LICFs.at(i).L_2;

							}else{
								// right LICF
								currentMatched_LICFs.MatchLICFs_R.L_1 = LICFs_otherImage.at(j).L_2;
								currentMatched_LICFs.MatchLICFs_R.L_2 = LICFs_otherImage.at(j).L_1;	
								// left LICF
								currentMatched_LICFs.MatchLICFs_L.L_1 = Actual_LICFs.at(i).L_2;
								currentMatched_LICFs.MatchLICFs_L.L_2 = Actual_LICFs.at(i).L_1;							
							}
						
							
							// right LICF
							currentMatched_LICFs.MatchLICFs_R.x_xK = LICFs_otherImage.at(j).x_xK;
							currentMatched_LICFs.MatchLICFs_R.y_xK = LICFs_otherImage.at(j).y_xK;
							//currentMatched_LICFs.MatchLICFs_R.L_1 = LICFs_otherImage.at(j).L_1;
							//currentMatched_LICFs.MatchLICFs_R.L_2 = LICFs_otherImage.at(j).L_2;
							currentMatched_LICFs.MatchLICFs_R.angleBetweenLines = LICFs_otherImage.at(j).angleBetweenLines;
							currentMatched_LICFs.MatchLICFs_R.crossRatio = LICFs_otherImage.at(j).crossRatio;
							// left LICF
							currentMatched_LICFs.MatchLICFs_L.x_xK = Actual_LICFs.at(i).x_xK;
							currentMatched_LICFs.MatchLICFs_L.y_xK = Actual_LICFs.at(i).y_xK;
							//currentMatched_LICFs.MatchLICFs_L.L_1 = Actual_LICFs.at(i).L_1;
							//currentMatched_LICFs.MatchLICFs_L.L_2 = Actual_LICFs.at(i).L_2;
							currentMatched_LICFs.MatchLICFs_L.angleBetweenLines = Actual_LICFs.at(i).angleBetweenLines;
							currentMatched_LICFs.MatchLICFs_L.crossRatio = Actual_LICFs.at(i).crossRatio;

							// save on vector
							Actual_Matched_LICFs.push_back(currentMatched_LICFs);
						}
					}
				}
			}// end for j
		}
		return Actual_Matched_LICFs;
	}
	catch(...){
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	vector<Matching_LICFs> LICFs::RefineMatchingLICFs(cv::Mat F_matrix,
/// 	vector<Matching_LICFs> actualMatchingLICFs, SubArea_Structure SubAreaImageL,
/// 	SubArea_Structure SubAreaImageR, float maxError)
///
/// @brief	 refine Matching LICFS using: 
/// 		 1. Epipolar constraint relationship to test real 3D intersection points 
/// 		 2. Cross ratio for LICFs lines matching  
/// 		 3. Remember that the Homography and the Fundamental matrix must agree
///		     with the relation HtransposeF + FtransposeH = 0 according to eq 13.3
///          from Multiple View Geometry,Zisserman Book
///
/// @date	16/05/2012
///
/// @param	F_matrix		   	The F matrix.
/// @param	actualMatchingLICFs	The actual matched LICFs.
/// @param	SubAreaImageL	   	The sub area on image L.
/// @param	SubAreaImageR	   	The sub area on image R.
/// @param	maxError		   	The maximum error.
///
/// @return	A refined vector with LICFs matches
////////////////////////////////////////////////////////////////////////////////////////////////////

vector<Matching_LICFs> LICFs::RefineMatchingLICFs(cv::Mat F_matrix, vector<Matching_LICFs> actualMatchingLICFs,
												  SubArea_Structure SubAreaImageL, SubArea_Structure SubAreaImageR, float maxError){
try{
		// variables
		vector<Matching_LICFs> refinedMatches;
		vector<Matching_LICFs> refinedMatchesSorted;
		Matching_LICFs swap1,swap2;
		float ConstraintError = 0;
		float tmpConstraintError = 0;
		int Ntrans = actualMatchingLICFs.size();
		if (actualMatchingLICFs.empty()){
			//refinedMatches = vector<Matching_LICFs>();
			return refinedMatches;
		}
		// matching LIFCs variables
		int refinedNtrans = 0 ;
		double minThetaAngle = 10;
		double currentAngleDifference_L1 = 15;
		double currentAngleDifference_L2 = 15;
		float minCrossRatio = 0.05;
		float crossRatio_LICF_L = 0,crossRatio_LICF_R = 0;

		int Xo_L,Yo_L,Xo_R,Yo_R;
		CvPoint upperLeft_L,upperLeft_R;
		// origin points of the subimages respect to the original images		
		upperLeft_L = cvPoint(SubAreaImageL.x_AreaCenter - abs(0.5*SubAreaImageL.width),
			SubAreaImageL.y_AreaCenter - abs(0.5*SubAreaImageL.heigh));
		upperLeft_R = cvPoint(SubAreaImageR.x_AreaCenter - abs(0.5*SubAreaImageR.width),
			SubAreaImageR.y_AreaCenter - abs(0.5*SubAreaImageR.heigh));
		Xo_L = upperLeft_L.x;
		Yo_L = upperLeft_L.y;
		Xo_R = upperLeft_R.x;
		Yo_R = upperLeft_R.y;
		// check number of matches
		// minimum are 3 to get the epipolar lines
		vector<cv::Point3f> EpilinesL;
		vector<cv::Point3f> EpilinesR;
		vector<cv::Point2f> LICFs_pointsL;
		vector<cv::Point2f> LICFs_pointsR;
		cv::Point2f pointL,pointR;
		// line equations vectors x = xo + vt 
		float xL1,yL1,xL2,yL2;
		float xR1,yR1,xR2,yR2;
		float xK_L = 0,yK_L = 0,xK_R = 0,yK_R = 0;
		float v_L1_L_Mag = 1,v_L2_L_Mag = 1;
		float v_L1_R_Mag = 1,v_L2_R_Mag = 1;
		cv::Vec2f vLine1_L,vLine2_L,vLine1_R,vLine2_R;
		cv::Vec2f v_L1_L,v_L2_L,v_L1_R,v_L2_R;
		cv::Vec2f v_xoL,v_xoR;
		float length_L_L1 = 0,length_L_L2 = 0, finalLength = 0;
		float length_R_L1 = 0,length_R_L2 = 0;
		float minLength = 10;

		if (Ntrans >= 3){

			// fill the matching points
			CvScalar Value;
			Value.val[2] = 0;
			Value.val[3] = 0;
			for (int i =0;i < Ntrans;i++){

				pointL.x = actualMatchingLICFs.at(i).MatchLICFs_L.x_xK + Xo_L;
				pointL.y = actualMatchingLICFs.at(i).MatchLICFs_L.y_xK + Yo_L;
				LICFs_pointsL.push_back(pointL);

				pointR.x = actualMatchingLICFs.at(i).MatchLICFs_R.x_xK + Xo_R;
				pointR.y = actualMatchingLICFs.at(i).MatchLICFs_R.y_xK + Yo_R;
				LICFs_pointsR.push_back(pointR);

			}

			// Get epipolar lines for the right image
			cv::computeCorrespondEpilines(LICFs_pointsL,1,F_matrix,EpilinesR);
			// Get epipolar lines for the left image
			cv::computeCorrespondEpilines(LICFs_pointsR,2,F_matrix,EpilinesL);
			
			// APPLY THE EPIPOLAR CONSTRAINT TO ALL MATCHED POINTS

			// lines parameters ax + by + c =0
			double aL,bL,cL,aR,bR,cR;
			double xoL,yoL,xoR,yoR;
			for (int i=0; i < Ntrans; i++){

				aL = EpilinesL.at(i).x;	aR = EpilinesR.at(i).x;
				bL = EpilinesL.at(i).y;	bR = EpilinesR.at(i).y;
				cL = EpilinesL.at(i).z;	cR = EpilinesR.at(i).z;

				xoL = actualMatchingLICFs.at(i).MatchLICFs_L.x_xK + Xo_L;
				xoR = actualMatchingLICFs.at(i).MatchLICFs_R.x_xK + Xo_R;
				yoL = actualMatchingLICFs.at(i).MatchLICFs_L.y_xK + Yo_L;
				yoR = actualMatchingLICFs.at(i).MatchLICFs_R.y_xK + Yo_R;
				// Test if intersection points corespond to a real
				// intersection in the real world
				// d(x',Fx)^2 + d(x,Ftx')^2 where d are the distances (point,Line)

				tmpConstraintError = pow((abs(aL*xoL + bL*yoL + cL)/sqrt(pow(aL,2) + pow(bL,2))),2) 
					+ pow((abs(aR*xoR + bR*yoR + cR)/sqrt(pow(aR,2)+ pow(bR,2))),2);
				
				// check epipolar error
				if (tmpConstraintError < maxError){
					// save matching epipolar error
					actualMatchingLICFs.at(i).epipolar_error = tmpConstraintError;
					// check line matching 
					currentAngleDifference_L1 = actualMatchingLICFs.at(i).MatchLICFs_L.L_1.thetaAngle - actualMatchingLICFs.at(i).MatchLICFs_R.L_1.thetaAngle;
					currentAngleDifference_L2 = actualMatchingLICFs.at(i).MatchLICFs_L.L_2.thetaAngle - actualMatchingLICFs.at(i).MatchLICFs_R.L_2.thetaAngle;
					actualMatchingLICFs.at(i).line_matching_error = abs(currentAngleDifference_L1) +  abs(currentAngleDifference_L2);
					// check line matching via cross ratio invariance
					crossRatio_LICF_L = actualMatchingLICFs.at(i).MatchLICFs_L.crossRatio;
					crossRatio_LICF_R = actualMatchingLICFs.at(i).MatchLICFs_R.crossRatio;

					//if ((abs(currentAngleDifference_L1) < minThetaAngle)&(abs(currentAngleDifference_L2) < minThetaAngle)){
					if (abs(crossRatio_LICF_L - crossRatio_LICF_R ) < minCrossRatio){
						// change line ends = normalize line ends to a minimum distance from the
						// line intersection point
						length_L_L1 = actualMatchingLICFs.at(i).MatchLICFs_L.L_1.length; 
						length_L_L2 = actualMatchingLICFs.at(i).MatchLICFs_L.L_2.length; 
						finalLength = min(abs(length_L_L1),abs(length_L_L2));
						if (finalLength < minLength ){
							finalLength = minLength;
						}
						//line vector parameters
					
						// Left Match
						xK_L = actualMatchingLICFs.at(i).MatchLICFs_L.x_xK;
						yK_L = actualMatchingLICFs.at(i).MatchLICFs_L.y_xK;
						xL1 = actualMatchingLICFs.at(i).MatchLICFs_L.L_1.x_farthest - xK_L;
						yL1 = actualMatchingLICFs.at(i).MatchLICFs_L.L_1.y_farthest - yK_L;
						xL2 = actualMatchingLICFs.at(i).MatchLICFs_L.L_2.x_farthest - xK_L;
						yL2 = actualMatchingLICFs.at(i).MatchLICFs_L.L_2.y_farthest - yK_L;

						// Rigth Match
						xK_R = actualMatchingLICFs.at(i).MatchLICFs_R.x_xK;
						yK_R = actualMatchingLICFs.at(i).MatchLICFs_R.y_xK;
						xR1 = actualMatchingLICFs.at(i).MatchLICFs_R.L_1.x_farthest - xK_R;
						yR1 = actualMatchingLICFs.at(i).MatchLICFs_R.L_1.y_farthest - yK_R;
						xR2 = actualMatchingLICFs.at(i).MatchLICFs_R.L_2.x_farthest - xK_R;
						yR2 = actualMatchingLICFs.at(i).MatchLICFs_R.L_2.y_farthest - yK_R;
						
						length_R_L1 = actualMatchingLICFs.at(i).MatchLICFs_R.L_1.length; 
						length_R_L2 = actualMatchingLICFs.at(i).MatchLICFs_R.L_2.length;

						// set new end line points using vectors v = xo + vt for Left Match
						v_xoL = cv::Vec2f((float)xK_L,(float)yK_L);
						vLine1_L = cv::Vec2f((float)xL1/length_L_L1,(float)yL1/length_L_L1);
						vLine2_L = cv::Vec2f((float)xL2/length_L_L2,(float)yL2/length_L_L2);
						//	L1
						v_L1_L = v_xoL + finalLength*vLine1_L;
						//	L2
						v_L2_L = v_xoL + finalLength*vLine2_L;
						// new lengths
						v_L1_L_Mag = sqrt(pow(v_L1_L[0] - xK_L,2) + pow(v_L1_L[1] - yK_L,2));
						v_L2_L_Mag = sqrt(pow(v_L2_L[0] - xK_L,2) + pow(v_L2_L[1] - yK_L,2));

						// set new end line points using vectors v = xo + vt for Right Match
						v_xoR = cv::Vec2f((float)xK_R,(float)yK_R);
						vLine1_R = cv::Vec2f((float)xR1/length_R_L1,(float)yR1/length_R_L1);
						vLine2_R = cv::Vec2f((float)xR2/length_R_L2,(float)yR2/length_R_L2);
						//	L1
						v_L1_R = v_xoR + finalLength*vLine1_R;
						//	L2
						v_L2_R = v_xoR + finalLength*vLine2_R;
						// new lengths
						v_L1_R_Mag = sqrt(pow(v_L1_R[0] - xK_R,2) + pow(v_L1_R[1] - yK_R,2));
						v_L2_R_Mag = sqrt(pow(v_L2_R[0] - xK_R,2) + pow(v_L2_R[1] - yK_R,2));

						
						// save new end line points
						actualMatchingLICFs.at(i).MatchLICFs_L.L_1.x_farthest = v_L1_L[0];
						actualMatchingLICFs.at(i).MatchLICFs_L.L_1.y_farthest = v_L1_L[1];
						actualMatchingLICFs.at(i).MatchLICFs_L.L_2.x_farthest = v_L2_L[0];
						actualMatchingLICFs.at(i).MatchLICFs_L.L_2.y_farthest = v_L2_L[1];
						actualMatchingLICFs.at(i).MatchLICFs_L.L_1.length = v_L1_L_Mag;
						actualMatchingLICFs.at(i).MatchLICFs_L.L_2.length = v_L2_L_Mag;

						actualMatchingLICFs.at(i).MatchLICFs_R.L_1.x_farthest = v_L1_R[0];
						actualMatchingLICFs.at(i).MatchLICFs_R.L_1.y_farthest = v_L1_R[1];
						actualMatchingLICFs.at(i).MatchLICFs_R.L_2.x_farthest = v_L2_R[0];
						actualMatchingLICFs.at(i).MatchLICFs_R.L_2.y_farthest = v_L2_R[1];
						actualMatchingLICFs.at(i).MatchLICFs_R.L_1.length = v_L1_R_Mag;
						actualMatchingLICFs.at(i).MatchLICFs_R.L_2.length = v_L2_R_Mag;


						// save refined matches
						refinedMatches.push_back(actualMatchingLICFs.at(i));
						ConstraintError = ConstraintError + tmpConstraintError;
						refinedNtrans = refinedNtrans + 1;
					}
				}
			}
			
			ConstraintError = ConstraintError/refinedNtrans;
		
		}
		else{// Ntrans <3
				ConstraintError = -1; // not enough matching found
		}

		// emit the corresponding signal
		//emit SendEpipolarErrorConstraint(ConstraintError);
		
		// return results
		// Get the best matching point using the insertion sorting algorithm
		// so the best match is the first value and so on
		int k,j = 0;
		float error1,error2;
		for (int i = 1;i <refinedMatches.size();i++){
			k = i;
 			j = k-1;			
			for(k;k>0;k--){
				error1 = refinedMatches.at(k).epipolar_error;
				error2 = refinedMatches.at(j).epipolar_error;
				if (error1 < error2){
					swap1 = refinedMatches.at(k);
					swap2 = refinedMatches.at(j);
					swap<Matching_LICFs>(refinedMatches.at(k),refinedMatches.at(j));
				
				}				
			}
		}
		return refinedMatches;


}
catch(...){
}

}

// Draw LICFs Matches
void LICFs::DrawLICF_Matches(cv::Mat leftImage, cv::Mat rightImage, std::vector<Matching_LICFs> matchedPoints){
	try{
		// points and matches
		cv::Mat LICFsMatches;
		vector<cv::KeyPoint> KeyPointsL;
		vector<cv::KeyPoint> KeyPointsR;
		vector<cv::DMatch> matches;
		cv::KeyPoint pointL,pointR;
		cv::DMatch currentMatch;

		int size = matchedPoints.size();
		int counter = 0;
		for (int i=0;i < size;i++){
				
				//	left xk point
				pointL.pt.x = matchedPoints.at(i).MatchLICFs_L.x_xK;
				pointL.pt.y = matchedPoints.at(i).MatchLICFs_L.y_xK;
				KeyPointsL.push_back(pointL);
				//	right xk 
				pointR.pt.x = matchedPoints.at(i).MatchLICFs_R.x_xK;
				pointR.pt.y = matchedPoints.at(i).MatchLICFs_R.y_xK;
				KeyPointsR.push_back(pointR);
				// match xk
				currentMatch.imgIdx = counter;
				currentMatch.distance = matchedPoints.at(i).epipolar_error;
				currentMatch.trainIdx = counter;
				currentMatch.queryIdx = counter;
				counter = counter + 1;
				// matches 
				matches.push_back(currentMatch);
				//	left ex1 points
				pointL.pt.x = matchedPoints.at(i).MatchLICFs_L.L_1.x_farthest;
				pointL.pt.y = matchedPoints.at(i).MatchLICFs_L.L_1.y_farthest;
				KeyPointsL.push_back(pointL);
				//	right ex1 
				pointR.pt.x = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest;
				pointR.pt.y = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest;
				KeyPointsR.push_back(pointR);
				// match ex1
				currentMatch.imgIdx = counter;
				currentMatch.distance = matchedPoints.at(i).epipolar_error;
				currentMatch.trainIdx = counter;
				currentMatch.queryIdx = counter;
				counter = counter + 1;
				// matches
				matches.push_back(currentMatch);
				//	left ex2 points
				pointL.pt.x = matchedPoints.at(i).MatchLICFs_L.L_2.x_farthest;
				pointL.pt.y = matchedPoints.at(i).MatchLICFs_L.L_2.y_farthest;
				KeyPointsL.push_back(pointL);
				//	right ex2
				pointR.pt.x = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest;
				pointR.pt.y = matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest;
				KeyPointsR.push_back(pointR);
				// match ex2
				currentMatch.imgIdx = counter;
				currentMatch.distance = matchedPoints.at(i).epipolar_error;
				currentMatch.trainIdx = counter;
				currentMatch.queryIdx = counter;
				counter = counter + 1;
				// matches
				matches.push_back(currentMatch);
		
		}
		// draw LICFS matches
		cv::drawMatches(leftImage,KeyPointsL,rightImage,KeyPointsR,matches,LICFsMatches,cv::Scalar::all(-1),
			cv::Scalar::all(-1),vector<char>(),cv::DrawMatchesFlags::DEFAULT);
		cv::imshow("LICF Matches",LICFsMatches);

	}
	catch(...){
	}
}



// Draw detected planes
// using x' - Hx <= epsilon as valid points
void LICFs::DrawLICF_detectedPlane(cv::Mat x_prim,cv::Mat Hx, cv::Mat H, double epsilon){
	try{
		double Xdiff = 0,Ydiff = 0, Scale = 1;
		int distance_toHx = 0;
		float SSDvalue = 0;
		float diffSSD = 0;
		double delta_x = 0,delta_y = 0;
		cv::Mat detectedPlane = cv::Mat(x_prim.size(),CV_8UC3,cv::Scalar::all(0));
		cv::Mat detectedPlaneTmp = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(0));
		cv::Mat CIE_difference = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(0));
		cv::Mat disparityMap = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(0));
		cv::Mat x_tmp = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(255));
		cv::Mat x_prim_tmp = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(255));
		cv::Mat SSD = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(0));
		
		// convert to gray images
		cv::Mat x_prim_Gray,Hx_Gray;
		cv::cvtColor(x_prim,x_prim_Gray,CV_BGR2GRAY);
		cv::cvtColor(Hx,Hx_Gray,CV_BGR2GRAY);

		// draw planes using B G R channels
		cv::Mat ImageComparison;
		cv::Mat detectedPlaneOn_RGBChannels;
		vector<cv::Mat> channelImage1,channelImage2;
		cv::Mat ImageR = cv::Mat(x_prim.size(),CV_8UC3,cv::Scalar::all(0));
		cv::Mat ImageG = cv::Mat(x_prim.size(),CV_8UC3,cv::Scalar::all(0));
		cv::Mat fillChannel = cv::Mat(x_prim.size(),CV_8UC1,cv::Scalar::all(0));

		// built the G channel image

		channelImage1.push_back(fillChannel);	// B
		channelImage1.push_back(Hx_Gray);		// G
		channelImage1.push_back(fillChannel);	// R

		// built the R channel image

		channelImage2.push_back(fillChannel);	// B
		channelImage2.push_back(fillChannel);	// G
		channelImage2.push_back(x_prim_Gray);	// R

		cv::merge(channelImage1,ImageR);
		cv::merge(channelImage2,ImageG);

		detectedPlaneOn_RGBChannels = ImageR + ImageG;
		cv::compare(x_prim,Hx,ImageComparison,cv::CMP_EQ);

		// Find transformed points that are equal
		//cv::warpPerspective(x_tmp,x_prim_tmp,H,x_prim.size());
		//cv::compare(x_tmp,x_prim_tmp,detectedPlaneTmp,cv::CMP_EQ);
		detectedPlaneTmp = x_prim - Hx;
		// Find plane using H
		float pixelValue = 0;
		cv::Vec3b RBG_Value,BGR_Value1,BGR_Value2;
		float DeltaE_CIE=0,Delta_R=0,Delta_G=0,Delta_B=0;
		for(int y=0;y < x_prim.rows;y++){
			for (int x=0; x < x_prim.cols;x++){			
				
				// color difference using CIE76 adaptation
				BGR_Value1 = x_prim.ptr<cv::Vec3b>(y)[x];
				BGR_Value2 = Hx.ptr<cv::Vec3b>(y)[x];
				Delta_R = BGR_Value2.val[2] - BGR_Value1.val[2];
				Delta_G = BGR_Value2.val[1] - BGR_Value1.val[1];
				Delta_B = BGR_Value2.val[0] - BGR_Value1.val[0];

				DeltaE_CIE = sqrt(pow(Delta_R,2.0f)+ pow(Delta_G,2.0f) + pow(Delta_B,2.0f))/sqrt(3*pow(255,2.0f));
				CIE_difference.ptr<uchar>(y)[x] = 255 - int(DeltaE_CIE*255);
				// homography difference x' - Hx, it is equivalent to AD measure
				distance_toHx = abs(x_prim_Gray.ptr<uchar>(y)[x] - Hx_Gray.ptr<uchar>(y)[x]);
				diffSSD = 0;
				SSDvalue = 0;
				for (int i=-1;i<2;i++){
					for(int j=-1;j<2;j++){
						if ((y+j < x_prim.rows)&(x+i < x_prim.cols)&(y+j >= 0)&(x+i >= 0)){
							diffSSD = x_prim_Gray.ptr<uchar>(y+j)[x+i] - Hx_Gray.ptr<uchar>(y+j)[x+i]; 
							SSDvalue = pow(diffSSD,2) + SSDvalue;
						}
					}
				}
				SSD.ptr<uchar>(y)[x] = SSDvalue;
				
				switch(distance_toHx){
					case 0:
						RBG_Value.val[0]= 255;
						RBG_Value.val[1]= 0;
						RBG_Value.val[2]= 0;

						detectedPlane.ptr<cv::Vec3b>(y)[x] = RBG_Value;
						break;
					case 1:
						RBG_Value.val[0]= 0;
						RBG_Value.val[1]= 255;
						RBG_Value.val[2]= 0;
						detectedPlane.ptr<cv::Vec3b>(y)[x] = RBG_Value;
						break;
					case 2:
						RBG_Value.val[0]= 0;
						RBG_Value.val[1]= 0;
						RBG_Value.val[2]= 255;
						detectedPlane.ptr<cv::Vec3b>(y)[x] = RBG_Value;
						break;
					default:
						RBG_Value.val[0]= 255;
						RBG_Value.val[1]= 255;
						RBG_Value.val[2]= 0;
						detectedPlane.ptr<cv::Vec3b>(y)[x] = RBG_Value;
						break;
				}

			}
		}
		// draw Plane detected
		cv::imshow("Detected Planes RGB Channels",detectedPlaneOn_RGBChannels);
//		cv::imshow("x' vs Hx",ImageComparison);
//		cv::imshow("Detected Plane",detectedPlane);
//		cv::imshow("Detected Plane Tmp",detectedPlaneTmp);
		//cv::normalize(SSD,SSD,1,255,cv::NORM_MINMAX);
//		cv::imshow("SSD (x',Hx)",SSD);
		
		// census transform
		/*
		cv::Mat x_primTransformImage = CensusTransform(x_prim_Gray,3,3);
		cv::Mat Hx_TransformImage = CensusTransform(Hx_Gray,3,3);
		cv::imshow("Census Transform x'",x_primTransformImage);
		cv::imshow("Census Transform Hx",Hx_TransformImage);
		/*cv::Mat Img1 = x_primTransformImage.clone();
		cv::Mat Img2 = Hx_TransformImage.clone();
		cv::Mat disparityCensus = HammingDistance(Img1,Img2,cv::Size(3,3),16);
		cv::imshow("disparity Census",disparityCensus);*/

		
		//cv::imshow("CIE difference Test",CIE_difference);
	}
	catch(...){
	}
}
// Get the epipolar constraint value
// this is an indicator of the level of coplanarity of the 
// intersection LICFs found
LICFs_EpipolarConstraintResult LICFs::GetEpipolarConstraintError(vector<Matching_LICFs> matchedPoints,CvMat* F_matrix,SubArea_Structure SubAreaImageL, SubArea_Structure SubAreaImageR){
	try{
		// variables
		LICFs_EpipolarConstraintResult finalResults;
		float ConstraintError = 0;
		float tmpConstraintError = 0;
		int Ntrans = matchedPoints.size();
		int Xo_L,Yo_L,Xo_R,Yo_R;
		CvPoint upperLeft_L,upperLeft_R;
		// origin points of the subimages respect to the original images		
		upperLeft_L = cvPoint(SubAreaImageL.x_AreaCenter - abs(0.5*SubAreaImageL.width),
			SubAreaImageL.y_AreaCenter - abs(0.5*SubAreaImageL.heigh));
		upperLeft_R = cvPoint(SubAreaImageR.x_AreaCenter - abs(0.5*SubAreaImageR.width),
			SubAreaImageR.y_AreaCenter - abs(0.5*SubAreaImageR.heigh));
		Xo_L = upperLeft_L.x;
		Yo_L = upperLeft_L.y;
		Xo_R = upperLeft_R.x;
		Yo_R = upperLeft_R.y;

		// check number of matches
		// minimum are 3 to get the epipolar lines
		if (Ntrans >= 3){
			EpilinesL = cvCreateMat(3,Ntrans,CV_32FC1);
			EpilinesR = cvCreateMat(3,Ntrans,CV_32FC1);
			CvMat *LICFs_pointsL = cvCreateMat(1,Ntrans,CV_32FC2);
			CvMat *LICFs_pointsR = cvCreateMat(1,Ntrans,CV_32FC2);
			// fill the matching points
			CvScalar Value;
			Value.val[2] = 0;
			Value.val[3] = 0;
			for (int i =0;i < Ntrans;i++){

				Value.val[0] = matchedPoints.at(i).MatchLICFs_L.x_xK + Xo_L;
				Value.val[1] = matchedPoints.at(i).MatchLICFs_L.y_xK + Yo_L;
				cvSet2D(LICFs_pointsL,0,i,Value);
				Value.val[0] = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Value.val[1] = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				cvSet2D(LICFs_pointsR,0,i,Value);
			}

			// Get epipolar lines for the right image
			cvComputeCorrespondEpilines(LICFs_pointsL,1,F_matrix,EpilinesR);
			// Get epipolar lines for the left image
			cvComputeCorrespondEpilines(LICFs_pointsR,2,F_matrix,EpilinesL);

			// APPLY THE EPIPOLAR CONSTRAINT TO ALL MATCHED POINTS

			// lines parameters ax + by + c =0
			double aL,bL,cL,aR,bR,cR;
			double xoL,yoL,xoR,yoR;
			for (int i=0; i < Ntrans; i++){

				aL = cvmGet(EpilinesL,0,i);aR = cvmGet(EpilinesR,0,i);
				bL = cvmGet(EpilinesL,1,i);bR = cvmGet(EpilinesR,1,i);
				cL = cvmGet(EpilinesL,2,i);cR = cvmGet(EpilinesR,2,i);
				xoL = matchedPoints.at(i).MatchLICFs_L.x_xK + Xo_L;
				xoR = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				yoL = matchedPoints.at(i).MatchLICFs_L.y_xK + Yo_L;
				yoR = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				// find the error according to the equation:

				tmpConstraintError = pow((abs(aL*xoL + bL*yoL + cL)/sqrt(pow(aL,2) + pow(bL,2))),2) 
					+ pow((abs(aR*xoR + bR*yoR + cR)/sqrt(pow(aR,2)+ pow(bR,2))),2);
				ConstraintError = ConstraintError + tmpConstraintError;
			}
			
			ConstraintError = ConstraintError/Ntrans;
		
		}
		else{// Ntrans <3
				ConstraintError = -1; // not enough matching found
		}

		// emit the corresponding signal
		//emit SendEpipolarErrorConstraint(ConstraintError);
		// return results
		finalResults.errorValue = ConstraintError;
		finalResults.EpilineL = EpilinesL;
		finalResults.EpilineR = EpilinesR;
		return finalResults;
	}
	catch(...)
	{
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	double LICFs::CheckHomographyMatrix(vector<cv::Point2f> x, vector<cv::Point2f> x_prim,
/// 	cv::Mat H_matrix)
///
/// @brief	 check symetric error for Homography Matrix
///			 see page 95 Zissserman book
///			 x' = Hx,
///			 x = H-1x', so it is Sum (distance(x',Hx)2 + distance(x,H-1x')2)
///
/// @date	11/06/2012
///
/// @param	x			x.
/// @param	x_prim  	x'.
/// @param	H_matrix	The matrix H.
///
/// @return	.
////////////////////////////////////////////////////////////////////////////////////////////////////

double LICFs::CheckHomographyMatrix(vector<cv::Point2f> x, vector<cv::Point2f> x_prim, cv::Mat H_matrix){
	try{
		
		double Error = 0;
		double tmpError = 0;
		double Epsilon = 0.01;
		double distance_toHx = 0;
		double distance_toHinvXprim = 0;
		double distanceAccum = 0;
		double delta_x = 0,delta_y = 0;
		double a,b,c;
		int correctCount = 0;
		
		cv::Mat H_inv;
		cv::Mat Hx = cv::Mat(3,1,H_matrix.type(),cv::Scalar::all(0));
		cv::Mat H_inv_x_prim = cv::Mat(3,1,H_matrix.type(),cv::Scalar::all(0));
		cv::Mat tmpMat = cv::Mat(3,1,H_matrix.type(),cv::Scalar::all(0));
		cv::Mat tmpMat2 = cv::Mat(3,1,H_matrix.type(),cv::Scalar::all(0));

		int size = x.size();
		cv::Point2f leftPoint,rightPoint;
		cv::Point3f Hx_Point,Hinv_xprim_Point;

		H_inv = H_matrix.inv();
		/*vector<cv::Point3f> HomogeneousLeftPoints;
		vector<cv::Point3f> HomogeneousRightPoints;
		cv::convertPointsToHomogeneous(LeftPoints,HomogeneousLeftPoints);
		cv::convertPointsToHomogeneous(RightPoints,HomogeneousRightPoints);*/
		
		// the last point are the epipoles
		// that not are used for Homography calculus
		for (int i=0; i <size-1; i++){

			rightPoint = x_prim.at(i);
			leftPoint = x.at(i);
			tmpMat.ptr<double>(0)[0] = leftPoint.x;
			tmpMat.ptr<double>(1)[0] = leftPoint.y;
			tmpMat.ptr<double>(2)[0] = 1;
			tmpMat2.ptr<double>(0)[0] = rightPoint.x;
			tmpMat2.ptr<double>(1)[0] = rightPoint.y;
			tmpMat2.ptr<double>(2)[0] = 1;
			
			Hx = H_matrix*tmpMat;
			Hx_Point.z = Hx.ptr<double>(2)[0];
			Hx_Point.x = Hx.ptr<double>(0)[0]/Hx_Point.z;
			Hx_Point.y = Hx.ptr<double>(1)[0]/Hx_Point.z;
			
			// check distance between (x',Hx)
			delta_x = rightPoint.x - Hx_Point.x;
			delta_y = rightPoint.y - Hx_Point.y;
			distance_toHx = sqrt(pow(delta_x,2)+ pow(delta_y,2));			

			H_inv_x_prim = H_inv*tmpMat2;
			Hinv_xprim_Point.z = H_inv_x_prim.ptr<double>(2)[0];
			Hinv_xprim_Point.x = H_inv_x_prim.ptr<double>(0)[0]/Hinv_xprim_Point.z;
			Hinv_xprim_Point.y = H_inv_x_prim.ptr<double>(1)[0]/Hinv_xprim_Point.z;

			// check distance between (x,H-1x')
			delta_x = leftPoint.x - Hinv_xprim_Point.x;
			delta_y = leftPoint.y - Hinv_xprim_Point.y;
			distance_toHinvXprim = sqrt(pow(delta_x,2)+ pow(delta_y,2));

			// sum current match error
			distanceAccum = pow(distance_toHx,2) + pow(distance_toHinvXprim,2) + distanceAccum;
			printf("match %i (x',Hx)  distance: %E\n",i,distance_toHx);
			printf("match %i (x,Hinv*x') distance: %E\n",i,distance_toHinvXprim);
		
		}
		// printf error
		Error = distanceAccum/(size-1); // Ideal error = 0 max = # of matches = 1 pixel per match
		Error = sqrt(Error);
		std::cout<<"d(x'- Hx)^2 + d(x -H-1x')^2 pixel average error per match:"<<endl<<Error<<endl;
		return Error;

	}
	catch(...){
	}
}
// Check Homography conformity
// Htrans*F + Ftrans*H = 0  according to Zisserman Chapter 13.3
double LICFs::CheckHomographyConformity(cv::Mat H_matrix, cv::Mat F_matrix){
	try{
		cv::Mat Comformity = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Scalar Suma;
		double value = 1;
		Comformity = H_matrix.t()*F_matrix + F_matrix.t()*H_matrix;
		Suma = cv::sum(Comformity);
		std::cout<<"Comformity value:"<< Suma.val[0]<<std::endl;
		std::cout<<"Htrans*F + Ftrans*H matrix"<<endl<<Comformity<<endl<<endl;
		value = Suma.val[0];
		return value;
	}
	catch(...){
	}
}

// Get the homography between the left and right images
cv::Mat LICFs::FindLICF_BasedHomography(vector<Matching_LICFs> matchedPoints,cv::Mat F_matrix, cv::Mat epipole, cv::Mat epipole_prim,
								  SubArea_Structure SubAreaImageL, SubArea_Structure SubAreaImageR){
	try{
		// Homography matrix
		cv::Mat H_matrix = cv::Mat(3,3,CV_32FC1,cv::Scalar::all(1));
		// matrices for saving epilolar lines corresponding to points ex1 and ex2
		// L1' = Fex1 ; L2' = Fex2
		cv::Mat x = cv::Mat::zeros(3,1,CV_32FC1);
		cv::Mat x_prim = cv::Mat::zeros(3,1,CV_32FC1);
		cv::Mat EpilinesL = cv::Mat(4,3,CV_32FC1);
		cv::Mat EpilinesR = cv::Mat(4,3,CV_32FC1);
		lineParameters Line1,Line1_prim;
		lineParameters Line2,Line2_prim;
		lineIntersection lineCrossPointLine1,lineCrossPointLine2;
		float aR1,bR1,cR1,aR2,bR2,cR2;
		CvPoint ex1_prim, ex2_prim;
		float dL1,dL2,dL3,dL4;
		// check number of matches
		int matchSize = matchedPoints.size();
		double Xo_L,Yo_L,Xo_R,Yo_R;
		CvPoint upperLeft_L,upperLeft_R;
		// origin points of the subimages respect to the original images		
		upperLeft_L = cvPoint(SubAreaImageL.x_AreaCenter - abs(0.5*SubAreaImageL.width),
			SubAreaImageL.y_AreaCenter - abs(0.5*SubAreaImageL.heigh));
		upperLeft_R = cvPoint(SubAreaImageR.x_AreaCenter - abs(0.5*SubAreaImageR.width),
			SubAreaImageR.y_AreaCenter - abs(0.5*SubAreaImageR.heigh));
		Xo_L = upperLeft_L.x;
		Yo_L = upperLeft_L.y;
		Xo_R = upperLeft_R.x;
		Yo_R = upperLeft_R.y;

		// we need 4 matches to calculate the homography matrix
		// here we use the limit points from each LICF to get the 4 points
		// so with one LICF match we have enough information to calculate
		// the searched homography
		vector<cv::Point2f> LICFs_pointsL;
		vector<cv::Point2f> LICFs_pointsR;
		CvScalar Value;
		cv::Point2f   pointL,pointR;

		if (matchSize >=1){					
		
			Value.val[2] = 0;
			Value.val[3] = 0;
			for (int i =0;i < 1;i++){

				//Value.val[0] = matchedPoints.at(i).MatchLICFs_L.x_xK + Xo_L;
				//Value.val[1] = matchedPoints.at(i).MatchLICFs_L.y_xK + Yo_L;
				//cvSet2D(LICFs_pointsL,0,0,Value);
				pointL.x = matchedPoints.at(i).MatchLICFs_L.x_xK + Xo_L;
				pointL.y = matchedPoints.at(i).MatchLICFs_L.y_xK + Yo_L;
				LICFs_pointsL.push_back(pointL);
				
				//cvmSet(ex1,0,0,Value.val[0]);
				//cvmSet(ex1,1,0,Value.val[1]);
								
				//Value.val[0] = matchedPoints.at(i).MatchLICFs_L.L_1.x_farthest + Xo_L;
				//Value.val[1] = matchedPoints.at(i).MatchLICFs_L.L_1.y_farthest + Yo_L;
				//cvSet2D(LICFs_pointsL,0,1,Value);
				pointL.x = matchedPoints.at(i).MatchLICFs_L.L_1.x_farthest + Xo_L;
				pointL.y = matchedPoints.at(i).MatchLICFs_L.L_1.y_farthest + Yo_L;
				LICFs_pointsL.push_back(pointL);

				//cvmSet(ex1,0,0,Value.val[0]);
				//cvmSet(ex1,1,0,Value.val[1]);
				//cvmSet(ex1,2,0,1.0f);
				
				//Value.val[0] = matchedPoints.at(i).MatchLICFs_L.L_2.x_farthest + Xo_L;
				//Value.val[1] = matchedPoints.at(i).MatchLICFs_L.L_2.y_farthest + Yo_L;
				//cvSet2D(LICFs_pointsL,0,2,Value);
				//cvmSet(ex2,0,0,Value.val[0]);
				//cvmSet(ex2,1,0,Value.val[1]);
				//cvmSet(ex2,2,0,1.0f);
				pointL.x = matchedPoints.at(i).MatchLICFs_L.L_2.x_farthest + Xo_L;
				pointL.y = matchedPoints.at(i).MatchLICFs_L.L_2.y_farthest + Yo_L;
				LICFs_pointsL.push_back(pointL);
			
				// epipole value: e // normalize this values
				//float norm = epipole.ptr<double>(0)[2];
				Value.val[0] = epipole.ptr<double>(0)[0];
				Value.val[1] = epipole.ptr<double>(0)[1];
				pointL.x = Value.val[0];
				pointL.y = Value.val[1];
				LICFs_pointsL.push_back(pointL);
				//cvSet2D(LICFs_pointsL,0,3,Value);

				/*Value.val[2] = cvmGet(&epipole,0,2);
				Value.val[0] = cvmGet(&epipole,0,0)/Value.val[2];
				Value.val[1] = cvmGet(&epipole,0,1)/Value.val[2];
				Value.val[2] = 0.0f;*/				

				//cvmGet(epipole,0,2));			

				// find the corresponding lines on the right image
				cv::computeCorrespondEpilines(LICFs_pointsL,1,F_matrix,EpilinesR);
				std::cout<<"Epilines R"<<endl<<EpilinesR<<endl<<endl;
				//L1_prim = F_matrix.mul(ex1);
				//L2_prim = F_matrix.mul(ex2);

				//cvComputeCorrespondEpilines(LICFs_pointsL,1,F_matrix,Epilines);
				//cvMatMul(&F_matrix,ex1,L1_prim);
				//cvMatMul(&F_matrix,ex2,L2_prim);

				// find line intersections that correspond to ex1' and ex2'
				
				/*aR1 = cvmGet(Epilines,0,1);aR2 = cvmGet(Epilines,0,2);
				bR1 = cvmGet(Epilines,1,1);bR2 = cvmGet(Epilines,1,2);
				cR1 = cvmGet(Epilines,2,1);cR2 = cvmGet(Epilines,2,2);*/

				// get the coefficients for each line ax + by + c = 0
				// remember l'i = Fxi, only for ex1 and ex2
				aR1 = EpilinesR.ptr<float>(1)[0];aR2 = EpilinesR.ptr<float>(2)[0];
				bR1 = EpilinesR.ptr<float>(1)[1];bR2 = EpilinesR.ptr<float>(2)[1];
				cR1 = EpilinesR.ptr<float>(1)[2];cR2 = EpilinesR.ptr<float>(2)[2];

				Line1.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				Line1.y1 = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				Line1.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Line1.y2 = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				Line1_prim.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest;// + Xo_R;
				Line1_prim.y1 = -(aR1*Line1_prim.x1)/bR1 - cR1/bR1;// + Yo_R;
				Line1_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK;// + Xo_R;
				Line1_prim.y2 = -(aR1*Line1_prim.x2)/bR1 - cR1/bR1 ;//+ Yo_R;

				Line2.x1 = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest + Xo_R;
				Line2.y1 = matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest + Yo_R;
				Line2.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Line2.y2 = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				Line2_prim.x1 = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest;// + Xo_R;
				Line2_prim.y1 = -(aR2*Line2_prim.x1)/bR2 - cR2/bR2;// + Yo_R;
				Line2_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK;// + Xo_R;
				Line2_prim.y2 = -(aR2*Line2_prim.x2)/bR2 - cR2/bR2;// + Yo_R;

				lineCrossPointLine1 = GetLineIntersection(Line1,Line1_prim);
				lineCrossPointLine2 = GetLineIntersection(Line2,Line2_prim);

				// calculus using distance between l' and ex2' instead of intersections
				// for ex'1
				dL1 = abs(aR1*Line1.x1 + bR1*Line1.y1 + cR1)/sqrt(pow(aR1,2)+ pow(bR1,2));
				dL3 = abs(aR1*lineCrossPointLine1.xi + bR1*lineCrossPointLine1.yi + cR1)/sqrt(pow(aR1,2)+ pow(bR1,2));
				// for ex'2
				dL2 = abs(aR2*Line2.x1 + bR2*Line2.y1 + cR2)/sqrt(pow(aR2,2)+ pow(bR2,2));
				dL4 = abs(aR2*lineCrossPointLine2.xi + bR2*lineCrossPointLine2.yi + cR2)/sqrt(pow(aR2,2)+ pow(bR2,2)); 


				// Corresponding points on the other image

				/*Value.val[0] = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Value.val[1] = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				cvSet2D(LICFs_pointsR,0,0,Value); */
				pointR.x = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				pointR.y = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				LICFs_pointsR.push_back(pointR);

				/*Value.val[0] = lineCrossPointLine1.xi;//matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				Value.val[1] = lineCrossPointLine1.yi;//matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				cvSet2D(LICFs_pointsR,0,1,Value);*/
				
				pointR.x = lineCrossPointLine1.xi;
				pointR.y = lineCrossPointLine1.yi;
				LICFs_pointsR.push_back(pointR);

				/*Value.val[0] = lineCrossPointLine2.xi;//matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest + Xo_R;
				Value.val[1] = lineCrossPointLine2.yi;//matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest + Yo_R;
				cvSet2D(LICFs_pointsR,0,2,Value);*/
				
				pointR.x = lineCrossPointLine2.xi;
				pointR.y = lineCrossPointLine2.yi;
				LICFs_pointsR.push_back(pointR);

				// epipole_prim value: e'

				pointR.x = epipole_prim.ptr<double>(0)[0];
				pointR.y = epipole_prim.ptr<double>(1)[0];
				LICFs_pointsR.push_back(pointR);

				/*Value.val[2] = cvmGet(epipole_prim,2,0);  
				Value.val[0] = cvmGet(epipole_prim,0,0)/Value.val[2];
				Value.val[1] = cvmGet(epipole_prim,1,0)/Value.val[2];
				Value.val[2] = 0.0f;*/
				
				//cvmGet(epipole_prim,0,2));
			}
			// find the homography
			double maxPixelErr = 3;
			H_matrix = cv::findHomography(LICFs_pointsL,LICFs_pointsR,cv::RANSAC,maxPixelErr);
			// check H matrix error
			double H_error = CheckHomographyMatrix(LICFs_pointsL,LICFs_pointsR,H_matrix);
			std::cout<<"H matrix"<<endl<<H_matrix<<endl<<endl;
			

		}
		return H_matrix;
	}
	catch(...){
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	cv::Mat LICFs::FindLICF_BasedHomographyZissermman(vector<Matching_LICFs> matchedPoints,
/// 	cv::Mat F_matrix, cv::Mat epipole, cv::Mat epipole_prim, SubArea_Structure SubAreaImageL,
/// 	SubArea_Structure SubAreaImageR)
///
/// @brief	Get the homography using the method proposed in the chapter 13 (algorithm 13.1) of the book
///			multiple view geometry by Hartley and Zisserman
///
/// @date	16/05/2012
///
/// @param	matchedPoints	The matched points.
/// @param	F_matrix	 	The matrix.
/// @param	epipole		 	The epipole e .
/// @param	epipole_prim 	The epipole e'.
/// @param	SubAreaImageL	The sub area image l.
/// @param	SubAreaImageR	The sub area image r.
///
/// @return	The found LICF based homography by zissermman method.
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::FindLICF_BasedHomographyZissermman(vector<Matching_LICFs> matchedPoints, cv::Mat F_matrix, cv::Mat epipole, cv::Mat epipole_prim, 
												  SubArea_Structure SubAreaImageL, SubArea_Structure SubAreaImageR){
	try{
		// Matrices
		cv::Mat emptyH = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Mat H = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(1));
		cv::Mat A = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Mat M = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));		
		cv::Mat v = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Mat vT = cv::Mat(1,3,cv::DataType<double>::type,cv::Scalar::all(0));//(1,3)
		cv::Mat e_prim_vT = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Mat b = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
		cv::Mat xi_prim = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
		
		// find matching points using intersections between Lines of LICFS and epipolar lines
		// matrices for saving epilolar lines corresponding to points ex1 and ex2
		// L1' = Fex1 ; L2' = Fex2
		vector<cv::Point3f> Epilines; 
		lineParameters Line_Xk,Line_Xk_prim;
		lineParameters Line1,Line1_prim;
		lineParameters Line2,Line2_prim;
		lineIntersection lineCrossPointLine0,lineCrossPointLine1,lineCrossPointLine2;
		float aR0,bR0,cR0,aR1,bR1,cR1,aR2,bR2,cR2;
		float do_L1,do_L2,dL1,dL2,dL3,dL4;		

		// Homography Calculus
		double maxTransferError = 4;			// 2 pixel error using Transfer error for H, see pag 95 Zisserman Book 
		int matchSize = matchedPoints.size();
		double Xo_L,Yo_L,Xo_R,Yo_R;
		CvPoint upperLeft_L,upperLeft_R;
		// origin points of the subimages respect to the original images		
		upperLeft_L = cvPoint(SubAreaImageL.x_AreaCenter - abs(0.5*SubAreaImageL.width),
			SubAreaImageL.y_AreaCenter - abs(0.5*SubAreaImageL.heigh));
		upperLeft_R = cvPoint(SubAreaImageR.x_AreaCenter - abs(0.5*SubAreaImageR.width),
			SubAreaImageR.y_AreaCenter - abs(0.5*SubAreaImageR.heigh));
		Xo_L = upperLeft_L.x;
		Yo_L = upperLeft_L.y; 
		Xo_R = upperLeft_R.x;
		Yo_R = upperLeft_R.y;

		// set [e']x = cross product matrix representation
		double e1,e2,e3;
		e1 = epipole_prim.ptr<double>(0)[0];
		e2 = epipole_prim.ptr<double>(1)[0];
		e3 = epipole_prim.ptr<double>(2)[0];

		double skew_symetric_data[3][3] =  {{  0,-e3, e2},
											{ e3,  0,-e1},
											{-e2, e1,  0}};
		cv::Mat e_prim_skew_symetric = cv::Mat(3,3,F_matrix.type(),skew_symetric_data);
		
		// set A = [e']xF
		A = e_prim_skew_symetric*F_matrix;
		//// Get the best matching point using the insertion sorting algorithm
		//// the best is the first value
		//Matching_LICFs swap1,swap2;
		//for (int i = 1;i <matchSize;i++){
		//	for(int k = i;(k>0)&(matchedPoints.at(k).epipolar_error < matchedPoints.at(k-1).epipolar_error);k--){
		//		
		//		swap<Matching_LICFs>(matchedPoints.at(k),matchedPoints.at(k-1));
		//	}
		//}
		vector<cv::Point2f> LICFs_pointsL;
		vector<cv::Point2f> LICFs_pointsR;
		vector<cv::Point2f> LICFs_pointsR_Original;

		// Ask for matching LICFs
		if(matchSize >= 1){ // we need at least 1 LICF feature = 3 matched points xk,L1 and L2 ends
			// First at all, Get intersection points
			
			cv::Point2f PointL,PointR;

			CvScalar Value;
			Value.val[2] = 0;
			Value.val[3] = 0;
			for (int i =0;i < 1;i++){

				PointL.x = matchedPoints.at(i).MatchLICFs_L.x_xK + Xo_L;
				PointL.y = matchedPoints.at(i).MatchLICFs_L.y_xK + Yo_L;
				LICFs_pointsL.push_back(PointL);
												
				PointL.x = matchedPoints.at(i).MatchLICFs_L.L_1.x_farthest + Xo_L;
				PointL.y = matchedPoints.at(i).MatchLICFs_L.L_1.y_farthest + Yo_L;
				LICFs_pointsL.push_back(PointL);
				
				PointL.x = matchedPoints.at(i).MatchLICFs_L.L_2.x_farthest + Xo_L;
				PointL.y = matchedPoints.at(i).MatchLICFs_L.L_2.y_farthest + Yo_L;
				LICFs_pointsL.push_back(PointL);
				
				// epipole value: e // normalize this values
				PointL.x = epipole.ptr<float>(0)[0];
				PointL.y = epipole.ptr<float>(0)[1];
				LICFs_pointsL.push_back(PointL);

				// find the corresponding lines on the right image
				cv::computeCorrespondEpilines(LICFs_pointsL,1,F_matrix,Epilines);
				
				// find line intersections that correspond to xk' , ex1' and ex2'
				aR0 = Epilines.at(0).x;aR1 = Epilines.at(1).x;aR2 = Epilines.at(2).x;
				bR0 = Epilines.at(0).y;bR1 = Epilines.at(1).y;bR2 = Epilines.at(2).y;
				cR0 = Epilines.at(0).z;cR1 = Epilines.at(1).z;cR2 = Epilines.at(2).z;

				Line_Xk.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				Line_Xk.y1 = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				Line_Xk.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Line_Xk.y2 = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				Line_Xk_prim.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest;// + Xo_R;
				Line_Xk_prim.y1 = -(aR0*Line_Xk_prim.x1)/bR0 - cR0/bR0;// + Yo_R;
				Line_Xk_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK;// + Xo_R;
				// check if the x value is the same, avoid this situation
				if(Line_Xk_prim.x1 == Line_Xk_prim.x2){
					Line_Xk_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + 1;
				}
				Line_Xk_prim.y2 = -(aR0*Line_Xk_prim.x2)/bR0 - cR0/bR0 ;//+ Yo_R;

				Line1.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				Line1.y1 = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				Line1.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Line1.y2 = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				Line1_prim.x1 = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest;// + Xo_R;
				Line1_prim.y1 = -(aR1*Line1_prim.x1)/bR1 - cR1/bR1;// + Yo_R;
				Line1_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK;// + Xo_R;
				// check if the x value is the same, avoid this situation
				if(Line1_prim.x1 == Line1_prim.x2){
					Line1_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + 1;
				}
				Line1_prim.y2 = -(aR1*Line1_prim.x2)/bR1 - cR1/bR1 ;//+ Yo_R;

				Line2.x1 = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest + Xo_R;
				Line2.y1 = matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest + Yo_R;
				Line2.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Line2.y2 = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				Line2_prim.x1 = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest;// + Xo_R;
				Line2_prim.y1 = -(aR2*Line2_prim.x1)/bR2 - cR2/bR2;// + Yo_R;
				Line2_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK;// + Xo_R;
					// check if the x value is the same, avoid this situation
				if(Line2_prim.x1 == Line2_prim.x2){
					Line2_prim.x2 = matchedPoints.at(i).MatchLICFs_R.x_xK + 1;
				}
				Line2_prim.y2 = -(aR2*Line2_prim.x2)/bR2 - cR2/bR2;// + Yo_R;

				lineCrossPointLine0 = GetLineIntersection(Line_Xk,Line_Xk_prim);
				lineCrossPointLine1 = GetLineIntersection(Line1,Line1_prim);
				lineCrossPointLine2 = GetLineIntersection(Line2,Line2_prim);

				// calculus using distance between l' and ex2' instead of intersections
				// for xk'
				Value.val[0] = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				Value.val[1] = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;

				do_L1 = abs(aR0*Value.val[0] + bR0*Value.val[1] + cR0)/sqrt(pow(aR0,2)+ pow(bR0,2));
				do_L2 = abs(aR0*lineCrossPointLine0.xi + bR0*lineCrossPointLine0.yi + cR0)/sqrt(pow(aR0,2)+ pow(bR0,2));
				// for ex'1
				dL1 = abs(aR1*Line1.x1 + bR1*Line1.y1 + cR1)/sqrt(pow(aR1,2)+ pow(bR1,2));
				dL3 = abs(aR1*lineCrossPointLine1.xi + bR1*lineCrossPointLine1.yi + cR1)/sqrt(pow(aR1,2)+ pow(bR1,2));
				// for ex'2
				dL2 = abs(aR2*Line2.x1 + bR2*Line2.y1 + cR2)/sqrt(pow(aR2,2)+ pow(bR2,2));
				dL4 = abs(aR2*lineCrossPointLine2.xi + bR2*lineCrossPointLine2.yi + cR2)/sqrt(pow(aR2,2)+ pow(bR2,2)); 


				// Original matching points without apply intersection with Epipolar Lines

				PointR.x = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				PointR.y = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				LICFs_pointsR_Original.push_back(PointR); 

				PointR.x = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				PointR.y = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				LICFs_pointsR_Original.push_back(PointR);

				PointR.x = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest + Xo_R;
				PointR.y = matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest + Yo_R;
				LICFs_pointsR_Original.push_back(PointR);

				PointR.x = epipole_prim.ptr<double>(0)[0];
				PointR.y = epipole_prim.ptr<double>(1)[0];
				LICFs_pointsR_Original.push_back(PointR);	

				// Corresponding points on the other image using intersection with epipolar lines

				PointR.x = lineCrossPointLine0.xi;
				PointR.y = lineCrossPointLine0.yi;
				LICFs_pointsR.push_back(PointR); 

				PointR.x = lineCrossPointLine1.xi;
				PointR.y = lineCrossPointLine1.yi;
				LICFs_pointsR.push_back(PointR);

				PointR.x = lineCrossPointLine2.xi;
				PointR.y = lineCrossPointLine2.yi;
				LICFs_pointsR.push_back(PointR);

				// epipole_prim value: e'				  
				PointR.x = epipole_prim.ptr<double>(0)[0];
				PointR.y = epipole_prim.ptr<double>(1)[0];
				LICFs_pointsR.push_back(PointR);				

				// set M = xi transpose, which correspond to left image points
				
				M.ptr<double>(0)[0] = matchedPoints.at(0).MatchLICFs_L.x_xK + Xo_L;
				M.ptr<double>(1)[0] = matchedPoints.at(0).MatchLICFs_L.L_1.x_farthest + Xo_L;
				M.ptr<double>(2)[0] = matchedPoints.at(0).MatchLICFs_L.L_2.x_farthest + Xo_L;
				
				M.ptr<double>(0)[1] = matchedPoints.at(0).MatchLICFs_L.y_xK + Yo_L;
				M.ptr<double>(1)[1] = matchedPoints.at(0).MatchLICFs_L.L_1.y_farthest + Yo_L;
				M.ptr<double>(2)[1] = matchedPoints.at(0).MatchLICFs_L.L_2.y_farthest + Yo_L;
				// set 3rd value to 1 as homogeneous coordinates, isn't it?
				M.ptr<double>(0)[2] = 1;
				M.ptr<double>(1)[2] = 1;
				M.ptr<double>(2)[2] = 1;

				// set xi' right image matching points vector matrix
				// 
				xi_prim.ptr<double>(0)[0] = matchedPoints.at(i).MatchLICFs_R.x_xK + Xo_R;
				xi_prim.ptr<double>(1)[0] = matchedPoints.at(i).MatchLICFs_R.y_xK + Yo_R;
				xi_prim.ptr<double>(2)[0] = 1;
				
				xi_prim.ptr<double>(0)[1] = matchedPoints.at(i).MatchLICFs_R.L_1.x_farthest + Xo_R;
				xi_prim.ptr<double>(1)[1] = matchedPoints.at(i).MatchLICFs_R.L_1.y_farthest + Yo_R;
				xi_prim.ptr<double>(2)[1] = 1;
	
				xi_prim.ptr<double>(0)[2] = matchedPoints.at(i).MatchLICFs_R.L_2.x_farthest + Xo_R;
				xi_prim.ptr<double>(1)[2] = matchedPoints.at(i).MatchLICFs_R.L_2.y_farthest + Yo_R;
				xi_prim.ptr<double>(2)[2] = 1;

				/*
				xi_prim.ptr<double>(0)[0] = lineCrossPointLine0.xi;
				xi_prim.ptr<double>(1)[0] = lineCrossPointLine0.yi;
				xi_prim.ptr<double>(2)[0] = 1;
				
				xi_prim.ptr<double>(0)[1] = lineCrossPointLine1.xi;
				xi_prim.ptr<double>(1)[1] = lineCrossPointLine1.yi;
				xi_prim.ptr<double>(2)[1] = 1;
	
				xi_prim.ptr<double>(0)[2] = lineCrossPointLine2.xi;
				xi_prim.ptr<double>(1)[2] = lineCrossPointLine2.yi;
				xi_prim.ptr<double>(2)[2] = 1;				
				*/
			}
			// Now set b vector for complete equation knowledgement Mv = b

			cv::Mat xi = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat Axi = cv::Mat(3,3,cv::DataType<double>::type,cv::Scalar::all(0));
			
			cv::Mat vectorProduct1 = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat vectorProduct2 = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat vectorProduct2copy = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));

			cv::Mat transpose_vectorProduct1 = cv::Mat(1,3,cv::DataType<double>::type,cv::Scalar::all(0));
			
			cv::Mat vector_xi = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat vector_Axi = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat vector_xi_prim = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));
			cv::Mat vector_e_prim = cv::Mat(3,1,cv::DataType<double>::type,cv::Scalar::all(0));//(3,1)
			
		
			double Magnitude_vectorProduct2 = 1;
			double bi = 1;
	
			// e'
			vector_e_prim.ptr<double>(0)[0] = e1;
			vector_e_prim.ptr<double>(1)[0] = e2;
			vector_e_prim.ptr<double>(2)[0] = e3;

			xi = M.t();
			Axi = A*xi;
				
			// get b finally
			for (int i = 0;i<3;i++){
				// x'i
				vector_xi_prim.ptr<double>(0)[0] = xi_prim.ptr<double>(0)[i];
				vector_xi_prim.ptr<double>(1)[0] = xi_prim.ptr<double>(1)[i];
				vector_xi_prim.ptr<double>(2)[0] = xi_prim.ptr<double>(2)[i];
				// Axi
				vector_Axi.ptr<double>(0)[0] = Axi.ptr<double>(0)[i];
				vector_Axi.ptr<double>(1)[0] = Axi.ptr<double>(1)[i];
				vector_Axi.ptr<double>(2)[0] = Axi.ptr<double>(2)[i];
				// find cross product  (x'i x Axi) and its transposed vector
				vectorProduct1 = vector_xi_prim.cross(vector_Axi);
				transpose_vectorProduct1 = vectorProduct1.t();
				// find second cross product (x'i x e')
				vectorProduct2 = vector_xi_prim.cross(vector_e_prim);
				vectorProduct2copy = vector_xi_prim.cross(vector_e_prim);
				// get the magnitude of cross product (x'i x e')
				Magnitude_vectorProduct2 = vectorProduct2copy.dot(vectorProduct2);
				// get the value for bi
				bi = vectorProduct1.dot(vectorProduct2);
				bi = bi/Magnitude_vectorProduct2;
				b.ptr<double>(i)[0] = bi;
				// set Zero values to matrices
				double factor = 0;
				vector_xi_prim = vector_xi_prim*factor;
				vector_Axi = vector_Axi*factor;
				vectorProduct1 = vectorProduct1*factor;
				vectorProduct1 = vectorProduct1*factor;

			}
			// Solve Mv = b
			int solving_vFlag = 0;
			solving_vFlag = cv::solve(M,b,v,cv::DECOMP_LU);
			// Finally solve for Homography matrix H = A - e'vT
			if (solving_vFlag == 1){// valid solution
				vT = v.t();
				e_prim_vT = vector_e_prim*vT;
				H = A - e_prim_vT;		
			}
		}
		// return homography found for this points
		double normFactor = H.ptr<double>(2)[2];
		H = H*1/normFactor;
		// check H matrix error
		double H_error = CheckHomographyMatrix(LICFs_pointsL,LICFs_pointsR,H);
		std::cout<<"H matrix"<<endl<<H<<endl<<endl;
		// check H_error using only LICF matching
		double H_error_only_LICFS = CheckHomographyMatrix(LICFs_pointsL,LICFs_pointsR_Original,H);
		std::cout<<"H matrix LICF only error"<<endl<<H_error_only_LICFS<<endl<<endl;
		// return Homography
		if (H_error_only_LICFS <= maxTransferError){
			return H;
		}else{
			return emptyH;
		}
	
	}
	catch(...){
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	Visibility_Status LICFs::visibility(IplImage *leftGrayImage,
/// 	imagePoints &actualImages_Points,IplImage *OcclusionSubImage)
///
/// @brief	Occlusion methods analysis
//			Duchesne and Herve'05
//			visibility test
///
/// @date	30/05/2012
///
/// @param [in,out]	leftGrayImage	   	If non-null, the left gray image.
/// @param [in,out]	actualImages_Points	The actual images points.
/// @param [in,out]	OcclusionSubImage  	If non-null, the occlusion sub image.
///
/// @return	.
////////////////////////////////////////////////////////////////////////////////////////////////////

Visibility_Status LICFs::visibility(IplImage *leftGrayImage,imagePoints &actualImages_Points,IplImage *OcclusionSubImage){
	try{
		int WindowsSizeW = 15;//9;
		int WindowsSizeH = 15;//7;
		double minVal = 0;
		double maxVal = 0;
		CvPoint* minLoc = &pt;
		CvPoint* maxLoc = &pt;
		float threshold = 0.95;// max difference in pixels
		float disparity_Shifment = actualImages_Points.disparityShifment;
		Visibility_Status vertexStatus = UNKNOWN;
		CvPoint2D32f currentVertexPosition,ImageToMatchCenter,CurrentLeftPoint;
		// Get the current points coordinates
		CurrentLeftPoint.x = actualImages_Points.xiL;
		CurrentLeftPoint.y = actualImages_Points.yiL; 
		

		CvMat *Pl = cvCreateMat(WindowsSizeH,WindowsSizeW,CV_32FC1);
		CvMat *ImageToMatch = cvCreateMat(0.1*OcclusionSubImage->height,0.2*OcclusionSubImage->width,CV_32FC1);
		CvMat *MatchResults = cvCreateMat(ImageToMatch->height - Pl->height + 1,
			ImageToMatch->width - Pl->width + 1,CV_32FC1);

		// OpenCV mapping new_y = (h-actualImages_Points.yiL)
		// because OpenGL has the origin at the top left of the image
		// and OpenGL has the origin at the Bottom left of the image
		int new_yiL = imageOriginal->height - actualImages_Points.yiL;
		int new_yiR = imageOriginal->height - actualImages_Points.yiR;

		// check if point are inside the image frontiers
		if ((actualImages_Points.xiR > imageOriginal->width)||(actualImages_Points.xiR < 0)){
			// vertex is outside
			vertexStatus = UNKNOWN;
			return vertexStatus;
		}else{
			// find match using template match and NCC within a 15x15 window
			// left point is the control point
			currentVertexPosition.x = actualImages_Points.xiL;
			currentVertexPosition.y = new_yiL;
			ImageToMatchCenter.x = actualImages_Points.xiR;
		    ImageToMatchCenter.y = new_yiR;

			cvGetRectSubPix(leftGrayImage,Pl,currentVertexPosition);
			cvGetRectSubPix(OcclusionSubImage,ImageToMatch,ImageToMatchCenter);
			cvMatchTemplate(ImageToMatch,Pl,MatchResults,CV_TM_CCORR_NORMED);
			//MatchCensusTemplate(ImageToMatch,Pl,MatchResults);
			//cvNormalize(MatchResults,MatchResults,1);
			cvMinMaxLoc(MatchResults,&minVal,&maxVal,minLoc,maxLoc);

			if (maxVal > threshold){
				// match is found, find out if this point is visible or invisible
				float VisibilityLimit = (actualImages_Points.xiR - 0.5*MatchResults->width) + maxLoc->x;
 				if ( VisibilityLimit > actualImages_Points.xiR){	// VISIBLE, outward window
					vertexStatus = VISIBLE;		// YELLOW
					return vertexStatus;	
				}else if (VisibilityLimit < actualImages_Points.xiR){	// INVISIBLE, inward window
					vertexStatus = INVISIBLE;	// ORANGE
					return vertexStatus;
				}

			}else{
				// match not found the set to unknown
				vertexStatus = UNKNOWN;			// GREEN
				return vertexStatus;
			}
		}
	
	}
	catch(...){
	}
}
/*
// visibility match using a 9x7 Census Transform
void LICFs::MatchCensusTemplate(CvMat *ImageToSearch,CvMat *Feature,CvMat *Results){
	try{
		CvPoint2D32f currentCenter;
		int hammingDistance = 0;
		int sizeString = Feature->width*Feature->height - 1;
		CvMat *bitStringFeature =  cvCreateMat(1,sizeString,CV_32FC1);
		CvMat *bitStringTemplate =  cvCreateMat(1,sizeString,CV_32FC1);
		CvMat *bitStringCurrentHamming =  cvCreateMat(1,sizeString,CV_32FC1);
		CvMat *tmpResult = cvCreateMat(Results->height, Results->width,CV_32FC1);
		CvMat *Template = cvCreateMat(Feature->height,Feature->width,CV_32FC1);
		// find census transform
		int shiftX = 5;//(int)floor(Feature->width/2)) + 1;
		int shiftY = 4;//(int)floor(Feature->heigh/2)) + 1;
		cvSetZero(Results);
		bitStringFeature = CensusTransform(Feature);
		for(int i=0;i<Results->width;i++){
			for(int j=0;j<Results->height;j++){
				// get current analysis area
				currentCenter.x = i + shiftX;
				currentCenter.y = j + shiftY;
				cvGetRectSubPix(ImageToSearch,Template,currentCenter);
				bitStringTemplate = CensusTransform(Template);
				// compare with feature and save the hamming distance
				cvXor(bitStringFeature,bitStringTemplate,bitStringCurrentHamming);
				hammingDistance = cvCountNonZero(bitStringCurrentHamming);
				cvmSet(Results,j,i,hammingDistance);
			}
		}
	}
	catch(...){
	
	}
}

*/

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	cv::Mat LICFs::SegmentImage(cv::Mat InputImage)
///
/// @brief	Segment an image
///
/// @date	12/06/2012
///
/// @param	InputImage	The input color image to segment
///
/// @return	The segmented image
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::SegmentImageWaterShed(cv::Mat InputImage){
	try{
		cv::Mat SegmentedImage;		
		cv::Mat bg,fg;
		
		cv::Mat markers = cv::Mat(InputImage.size(),CV_8UC3,cv::Scalar::all(0));	
		cv::Mat markersImage = cv::Mat(InputImage.size(),CV_8U,cv::Scalar::all(0));		
		cv::Mat GrayImage;
		cv::Mat markers2,markersTmp;
		cv::Mat colorImage = InputImage.clone();
		// get gray image
		cv::cvtColor(colorImage,GrayImage,CV_BGR2GRAY);
 		cv::Mat BinaryImage = cv::Mat(InputImage.size(),GrayImage.type(),cv::Scalar::all(0));
		// get a binary image
		cv::adaptiveThreshold(GrayImage,BinaryImage,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY,31,5);
//		cv::imshow("Binary Image",BinaryImage);
		// apply morphological operators to separate background and foreground
		// 
	    cv::erode(BinaryImage,fg,cv::Mat(),cv::Point(-1,-1),3);		
//		cv::morphologyEx(fg,fg,cv::MORPH_CLOSE,cv::Mat(),cv::Point(-1,-1),1);
		cv::imshow("foreground",fg);

		/*cv::Mat BinaryInv = 255 - BinaryImage;
		cv::erode(BinaryInv,bg,cv::Mat(),cv::Point(-1,-1),3);
		cv::imshow("background",bg);
		cv::threshold(bg,bg,1,128,cv::THRESH_BINARY_INV);		
		/*markersImage = fg + bg;
		*/
		
		// set markers using contours
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		cv::findContours(fg,contours,hierarchy,CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		int idx = 0;
		for(; idx >= 0; idx = hierarchy[idx][0]){
			cv::Scalar color( rand()&255, rand()&255, rand()&255 );
    		cv::drawContours(markers, contours, idx, color, -1, 8, hierarchy, INT_MAX);
        }
		cv::dilate(markers,markers,cv::Mat(),cv::Point(-1,-1),3);
		markersTmp = markers.clone();
		cv::imshow("markers",markers);
		cv::cvtColor(markersTmp,markers2,CV_BGR2GRAY);
		markers2.convertTo(markers2,CV_32S);
		
		cv::watershed(InputImage,markers2);
		markers2.convertTo(SegmentedImage,CV_8U);		
		return SegmentedImage;
	}
	catch(...){
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	cv::Mat LICFs::SegmentImageGrabCut(cv::Mat InputImage, cv::Rect rectangleMarker)
///
/// @brief	Segment an image using the grab cut algorithm
///
/// @date	13/06/2012
///
/// @param	InputImage	   	The input color  image.
/// @param	rectangleMarker	The rectangle marker that mark the foreground objects.
///
/// @return	the segmented  objects
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::SegmentImageGrabCut(cv::Mat InputImage, cv::Rect rectangleMarker){
	try{
		cv::Mat SegmentedImage;
		cv::Mat fgModel,bgModel;
		cv::Mat bg;
		cv::Mat fg = cv::Mat(InputImage.size(),CV_8UC3,cv::Scalar(255,255,255));
		// apply GrabCut algorithm
		cv::grabCut(InputImage,SegmentedImage,rectangleMarker,bgModel,fgModel,5,cv::GC_INIT_WITH_RECT);
		// get the probably pixels that belong to foreground
		cv::compare(SegmentedImage,cv::GC_PR_FGD,SegmentedImage,cv::CMP_EQ);
		InputImage.copyTo(fg,SegmentedImage);
		return fg;
	}
	catch(...){
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	cv::Mat LICFs::FindCurrentDetectedPlane(cv::Mat SegmentedImage, LICFs_Structure currentLICF, planeEquation &foundPlane)
///
/// @brief	Segment a plane in the image where a LICF finds a plane
///
/// @date	14/06/2012
///
/// @param	SegmentedImage	   	A segmented image that contains planes. 
/// @param	currentLICF			current LICFS used to find the plane.
/// @param  foundPlane			plane equation from LICF points.
///
/// @return	the segmented  plane
////////////////////////////////////////////////////////////////////////////////////////////////////
cv::Mat LICFs::FindCurrentDetectedPlane(cv::Mat SegmentedImage,SubArea_Structure AreatoSearch, LICFs_Structure currentLICF, planeEquation &foundPlane){
	try{
		cv::Mat detectedPlane = cv::Mat(SegmentedImage.size(),CV_8UC1,cv::Scalar::all(0));
		cv::Moments Momentos;
		int x_center=1,y_center=1;
		int Xo = 0, Yo = 0;
		float PixelSegmented=0;
		cv::Size imgSize = SegmentedImage.size();
		vector<cv::Point2f> LICF_trianglePoints;
		cv::Point2f PointA,PointB,PointC;
		// read points
		Xo = AreatoSearch.x_AreaCenter - abs(0.5*AreatoSearch.width);
		Yo = AreatoSearch.y_AreaCenter - abs(0.5*AreatoSearch.heigh);

		PointA.x = Xo + currentLICF.x_xK;
		PointA.y = Yo + currentLICF.y_xK;
		LICF_trianglePoints.push_back(PointA);

		PointB.x = Xo + currentLICF.L_1.x_farthest;
		PointB.y = Yo + currentLICF.L_1.y_farthest;
		LICF_trianglePoints.push_back(PointB);

		PointC.x = Xo + currentLICF.L_2.x_farthest;
		PointC.y = Yo + currentLICF.L_2.y_farthest;
		LICF_trianglePoints.push_back(PointC);

		// find center of mass of LICF, use this point as reference point to represent the current plane
		Momentos = cv::moments(LICF_trianglePoints);
		x_center = (int)Momentos.m10/Momentos.m00;
		y_center = (int)Momentos.m01/Momentos.m00;

		// read pixel value from segmented image using the found center
		if ((x_center>=0)&(x_center<imgSize.width)&(y_center>=0)&(y_center<imgSize.height)){

			PixelSegmented = SegmentedImage.ptr<uchar>(y_center)[x_center];
		}

		// compare pixel value with segmented image
		cv::Scalar valueToSearch = cv::Scalar::all(0);
		valueToSearch.val[0] = PixelSegmented;
		cv::compare(SegmentedImage,valueToSearch,detectedPlane,cv::CMP_EQ);

		// find plane equation
		foundPlane.A = 1;
		foundPlane.B = 1;
		foundPlane.C = 1;
		foundPlane.D = 1;

		return detectedPlane;
	}
	catch(...){
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	cv::Mat LICFs::CensusTransform(cv::Mat Image)
///
/// @brief	Census transform. 9x7 max size 
/// 		This method calculates the census transform for one image
///			according to Humenberger paper
///			"A census-based stereo vision algorithm using modified
///			semi-global matching and plane-fitting to improve matching quality"
///			E(p1,p2) = 1 if p1 <= p2
///					 = 0 if p1 > p2
///			where p1 = current pixel
///				  p2 = central pixel
///
///			@date	23/05/2012
///
///			@param [in,out]	Gray Image	If non-null, the image.
///
///			@return	Census transformed Image.
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::CensusTransform(cv::Mat GrayImage, int CensusWidth, int CensusHeight){
	try{
		int sizeString = CensusWidth*CensusHeight - 1; 
		cv::Size imgSize = GrayImage.size();
	
		cv::Mat StringBits = cv::Mat(1,sizeString,CV_32FC1,cv::Scalar::all(0));
		cv::Mat CensusImage = cv::Mat(GrayImage.size(),CV_64FC1,cv::Scalar::all(0));

		int center_x,center_y;
		int x=0,y=0;
		double valueString = 0;
		float p1=0,p2=0;
		cv::Scalar p2Scalar;
		double PowerValue = 0;
					
		center_x = int(CensusWidth/2) + 1;
		center_y = int(CensusHeight/2) + 1;
		cv::Point2f centerPixel;
		cv::Size windowsSize = cv::Size(CensusWidth,CensusHeight); 
		cv::Mat SubImage = cv::Mat(windowsSize,GrayImage.type());
		// Image Scanning
		for(int k=CensusWidth;k < imgSize.width - CensusWidth; k++ ){
			for(int m=CensusHeight;m < imgSize.height - CensusHeight; m++ ){
				// Find Average of window to do the census
				// transform more robust to high frequency noise
				centerPixel.x = k + center_x;
				centerPixel.y = m + center_y;
				cv::getRectSubPix(GrayImage,windowsSize,centerPixel,SubImage);
				p2Scalar = cv::mean(SubImage);
				p2 = p2Scalar.val[0];
				// Apply census transform		
				for(int j=0;j<CensusHeight-1;j++){
					for (int i=0;i<CensusWidth-1;i++){
						// Get central pixel value for normal census
						//p2 = GrayImage.ptr<uchar>(m + center_y)[k + center_x];
						p1 = GrayImage.ptr<uchar>(m + j)[k + i];
						if (p1 <= p2){
							// MSB (most significant bit) is at the upper left 2^n, n = W*H - 1
							// LSB is at the lower right 2^0 = 1
							// the string follow the rows of the CensusWindow
							PowerValue = (CensusWidth*CensusHeight-1) - j*CensusWidth - i;
							valueString = 1*pow(2,PowerValue) + valueString;
						}else if(p1 > p2){
							valueString = 0;
						}

					}
				}
				// save value of transform
				CensusImage.ptr<double>(m)[k] = valueString;

			}
		}

		return CensusImage;
	}
	catch(...){
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	LICFs::DecimalToBinary(double value,int stringSize)
///
/// @brief	Function adapted from the Matlab Central 
/// 		Efficient convertors between binary and decimal numbers
///
/// @date	01/06/2012
///
/// @param	value	The value.
/// @param  stringSize  size of the binary string 				
/// 				
/// @return Matrix with the corresponding binary string of value
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::DecimalToBinary(double value){
	try {
		vector<int> binaryString;
		cv::Mat BinaryMat = cv::Mat(32,1,CV_8UC1,cv::Scalar::all(0));
		unsigned long decimal = value;
		int stringChar[32];
		if (decimal != 0){
			for(int i=0;i<32;i++){
			 stringChar[i]= (decimal>>i)&0x1;
			 BinaryMat.ptr<uchar>(i)[0] = stringChar[i];
			}
		}
		
		return BinaryMat;

	}
	catch(...){
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @fn	LICFs::HammingDistance(cv::Mat Img1, cv::Mat Img2, cv::Size censusWindow,
/// 	int disparityLevel)
///
/// @brief	Constructor.
///
/// @date	01/06/2012
///
/// @param	Img1		  	The first image.
/// @param	Img2		  	The second image.
/// @param	censusWindow  	The census window.
/// @param	disparityLevel	The disparity level.
/// 						
/// @return	HammingDistance map for these image at disparity Level.
////////////////////////////////////////////////////////////////////////////////////////////////////

cv::Mat LICFs::HammingDistance(cv::Mat Img1, cv::Mat Img2, cv::Size censusWindow, int disparityLevel){
	try{
		double value1=0,value2=0;
		int hammingDistance = 0;
		int MinHammingDistance = 100;
		int StringSize1 = 1,StringSize2 = 1;
		cv::Mat disparityMap = cv::Mat(Img1.size(),CV_8UC1,cv::Scalar::all(0));
		
		cv::Size imgSize = Img1.size();
		cv::Mat string1;
		cv::Mat string2;
		cv::Mat HammingMat; 

		for (int i=0;i < imgSize.width;i++){
			for(int j=0;j < imgSize.height;j++){

				value1 = Img1.ptr<double>(j)[i];
				string1 = DecimalToBinary(value1);
				MinHammingDistance = 100;
				for (int d=0;d<disparityLevel;d++){
					if (i+d < imgSize.width){
     					value2 = Img2.ptr<double>(j)[i + d];
	     				string2 = DecimalToBinary(value2);
						cv::bitwise_xor(string1,string2,HammingMat);
						hammingDistance = cv::countNonZero(HammingMat);
						if (hammingDistance < MinHammingDistance){
							MinHammingDistance = hammingDistance;
						}
						
					}
				}				
				
				if (MinHammingDistance > 0){
					disparityMap.ptr<uchar>(j)[i] = MinHammingDistance;
					
				}
				
			}
		}
		// Return difference between images
		cv::normalize(disparityMap,disparityMap,1,255,cv::NORM_MINMAX);
		return disparityMap;
	}
	catch(...){
	}


}
//#include "LICFs.moc"
