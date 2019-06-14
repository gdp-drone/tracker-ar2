#include "TrackerARB.h"

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>

using namespace std;
using namespace cv;
using namespace aruco;


static bool readDetectorParameters(const string &fname, Ptr<DetectorParameters> &params) {
  FileStorage fs(fname, FileStorage::READ);
  if (!fs.isOpened())
    return false;
  fs["adaptiveThreshWinSizeMin"] >> params->adaptiveThreshWinSizeMin;
  fs["adaptiveThreshWinSizeMax"] >> params->adaptiveThreshWinSizeMax;
  fs["adaptiveThreshWinSizeStep"] >> params->adaptiveThreshWinSizeStep;
  fs["adaptiveThreshConstant"] >> params->adaptiveThreshConstant;
  fs["minMarkerPerimeterRate"] >> params->minMarkerPerimeterRate;
  fs["maxMarkerPerimeterRate"] >> params->maxMarkerPerimeterRate;
  fs["polygonalApproxAccuracyRate"] >> params->polygonalApproxAccuracyRate;
  fs["minCornerDistanceRate"] >> params->minCornerDistanceRate;
  fs["minDistanceToBorder"] >> params->minDistanceToBorder;
  fs["minMarkerDistanceRate"] >> params->minMarkerDistanceRate;
  fs["cornerRefinementMethod"] >> params->cornerRefinementMethod;
  fs["cornerRefinementWinSize"] >> params->cornerRefinementWinSize;
  fs["cornerRefinementMaxIterations"] >> params->cornerRefinementMaxIterations;
  fs["cornerRefinementMinAccuracy"] >> params->cornerRefinementMinAccuracy;
  fs["markerBorderBits"] >> params->markerBorderBits;
  fs["perspectiveRemovePixelPerCell"] >> params->perspectiveRemovePixelPerCell;
  fs["perspectiveRemoveIgnoredMarginPerCell"] >> params->perspectiveRemoveIgnoredMarginPerCell;
  fs["maxErroneousBitsInBorderRate"] >> params->maxErroneousBitsInBorderRate;
  fs["minOtsuStdDev"] >> params->minOtsuStdDev;
  fs["errorCorrectionRate"] >> params->errorCorrectionRate;
  return true;
}

TrackerARB::TrackerARB(CVCalibration &cvl, float markerLength, float markerSeparation, int markersX, int markersY,
                       int markerDictId, bool showFrame)
    : Tracker(cvl, showFrame) {
  markerDict = aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME(markerDictId));
  Ptr<GridBoard> gridboard = GridBoard::create(markersX, markersY, markerLength, markerSeparation, markerDict);
  board = gridboard.staticCast<Board>();
  
  offsetX = (markersX - 1) / 2. * markerSeparation + markersX / 2. * markerLength;
  offsetY = (markersY - 1) / 2. * markerSeparation + markersY / 2. * markerLength;
  
  if (!readDetectorParameters("detector_params.yml", detectorParams)) {
    cerr << "Invalid detector parameters file" << endl; //TODO: Fix this
  }
  detectorParams->cornerRefinementMethod = CORNER_REFINE_SUBPIX;
}

void TrackerARB::offsetPose(const Vec3d &rVec, const Vec3d &tVec, Vec3d &otVec) {
  Mat temp;
  Mat R_ct = Mat::eye(3, 3, CV_64F);
  Rodrigues(rVec, R_ct);
  Vec3d landingOffset = {offsetX, offsetY, 0};
  temp = R_ct * landingOffset;
  temp = temp + tVec;
  otVec[0] = temp.at<double>(0, 0);
  otVec[1] = temp.at<double>(1, 0);
  otVec[2] = temp.at<double>(2, 0);
}

int TrackerARB::getRawPose(Mat &frame, Vec3d &tVec, Vec3d &rVec) {
  int detectedBoard = 0;
  if (!markerIds.empty()) {
    detectedBoard = estimatePoseBoard(markerCorners, markerIds, board, cameraMatrix, distCoeffs, rVec, tVec);
    if (showFrame) {
      drawDetectedMarkers(frame, markerCorners, markerIds);
      if (detectedBoard > 0)
        drawAxis(frame, cameraMatrix, distCoeffs, rVec, tVec, 5);
      if (showRejected && !rejectedCorners.empty())
        drawDetectedMarkers(frame, rejectedCorners, noArray(), Scalar(100, 0, 255));
    }
  }
  return detectedBoard;
}

int TrackerARB::getPose(Mat &frame, Vec3d &tVec, Vec3d &rVec) {
  int detectedBoard = 0;
  if (!markerIds.empty()) {
    detectedBoard = estimatePoseBoard(markerCorners, markerIds, board, cameraMatrix, distCoeffs, rVec, tVec);
    offsetPose(rVec, tVec, tVec);
    if (showFrame) {
      drawDetectedMarkers(frame, markerCorners, markerIds);
      if (detectedBoard > 0)
        drawAxis(frame, cameraMatrix, distCoeffs, rVec, tVec, 5);
      if (showRejected && !rejectedCorners.empty())
        drawDetectedMarkers(frame, rejectedCorners, noArray(), Scalar(100, 0, 255));
    }
  }
  return detectedBoard;
}

bool TrackerARB::detectLandingPad(Mat &frame) {
  detectMarkers(frame, markerDict, markerCorners, markerIds, detectorParams, rejectedCorners);
  return (!markerIds.empty());
}
