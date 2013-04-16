#ifndef ARtools_H
#define ARtools_H

#include <QtGui>
#include <QtGui/QMainWindow>
#include "ui_ARtools.h" 	// the file generated by the uic tools for ARtools.ui

// linux time
#if (UNIX)
	#include <sys/time.h>		// to measure update frequency
#endif
#include <time.h>

// include IOCCOMM communications library and Emmanuel's Haptic Library
#include "../libcomm/client.h"
#include "../haptic/haptic.h"
//mt library
#include <mt/mt.h>
// include common types
#include "common.h"
// include LICFs class
#include "LICFs.h"
// include EDlines class
#include "EDlines.h"
// openCV headers
//#include <opencv/cv.h>
//#include <opencv/cvaux.h>			
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include "pointer_3D.h"
// rendering types control
#define RENDERING_3D 1
#define RENDERING_OPENCV 2


// using multiple inheritance approach
class ARtools : public QMainWindow, private Ui::MainWindow
{
Q_OBJECT
public:
    ARtools();
    virtual ~ARtools();
    
  public slots:

  void show_fps();
  void show_haptic_data(mt::Transform);
  void get_image_points(imagePoints actualPoints);
  void get_IplImageStereo(IplImage *actualImageL,IplImage *actualImageR);
  void get_IplImageL(IplImage *actualImageL);
  void get_IplImageR(IplImage *actualImageR);
  float get_X_value();
  float get_Y_value();
  float get_Z_value();
signals:
  void SetWorkSpaceLimits(mt::Vector3 MinCubicLimits, mt::Vector3 MaxCubicLimits);
  void SetVisibility3Dpointer(Visibility_Status);
  
private slots:
	void SetupRemoteCameras();
	void ShowRemoteCamerasStart();
	void AboutAct();
	void ShowStereoVideo();
	void ShowLeftVideo();
	void ShowRightVideo();
	void SetCannyAperture(int hSldCannyAperture);
	void SetCannyLowThreshold(int hSldCannyLowValue);
	void SetCannyHighThreshold(int hSldCannyHighValue);
	void SetHoughThreshold(int HoughThreshold);
	void SetHoughMinLengthDetection(int minLengthDetection);
	void SetHoughMaxGapBetweenLines(int maxGapBetweenLines);
	void SetLICF_MaxDistanceBetweenLines(int maxDistance);
	void SetLICF_EpipolarErrorConstraintThreshold(int LimitValue);
	void GetLICF_EpipolarErrorConstraintValue(float ActualValue);
private:
   QAction *aboutApp;
   QAction *showleftCamera;
   QAction *configureRemoteCameras;
   QAction *calibratePTZCameras;
   QAction *startRemoteCameras;
   QAction *connectToHaptic;
   QAction *show3DPointer;
   QAction *showLICFs;
   QToolBar *mainToolbar;

   // methods
   float getTimeDiff();
   void createActions();
   void createMenus();   
   void createToolBars();

#ifdef UNIX
   timeval elapsedTimeFirst;	// timing structures
   timeval elapsedTimeLast;
   struct timezone timeZone;
#endif // UNIX
   // variables
   // OpenCV
   QMenu *VideoProcessing_Menu;
   IplImage *leftImage;	
   IplImage *rightImage;
   IplImage *leftWithHomography;
   IplImage *leftWithHomography2;
   IplImage *planeFound_onImage;
   IplImage *shiftedVerticalImage;
   IplImage *imgwithHomography_gray;
   IplImage *dstCmp; 
   pointer_3D ImageProcessing;
   pointer_3D SubWindows;
   imagePoints actualImages_Points;
   CvMat *H_alignment;
   double verticalShift;
   float verticalShiftSURF; 
   double CannyWindowSize;
   double thresholdCannyLow;
   double thresholdCannyHigh;
   int HoughThreshold;
   double HoughMinLengthDetection;
   double HoughMaxGapBetweenLines;
   int LICF_MaxDistanceBetweenLines;
   int LICF_EpipolarErrorConstraintLimit;
   float LICF_EpipolarErrorConstraintActualValue;
   int rendering_type;

   // help
   QMenu *help_Menu;
   float timeDiff; 
   float X_Haptic;
   float Y_Haptic;
   float Z_Haptic;
   float Xmin,Xmax;
   float Ymin,Ymax;
   float Zmin,Zmax;
   bool matrixF_calculated;
   bool images_alignment;
   
};

#endif // ARtools_H
