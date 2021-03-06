#include "resource.h"
#include "NuiApi.h"

#ifndef Connect2Kinect_H
#define Connect2Kinect_H

class Connect2Kinect {
	private:
		// Current Kinect
		INuiSensor * m_pNuiSensor;
		// Handles
		HANDLE m_pSkeletonStreamHandle;
		HANDLE m_hNextSkeletonEvent;
	public:
		int Initialize(void);
		void Update(int);
		void ProcessData(void);
		void getData(NUI_SKELETON_FRAME* pf);
		int getDataRGB(char* pdata);
};
#endif