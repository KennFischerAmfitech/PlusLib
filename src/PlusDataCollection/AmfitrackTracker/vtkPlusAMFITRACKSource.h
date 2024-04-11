/*=Plus=header=begin======================================================
  Progra  : Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkPlusAMFITRACKSource_h
#define __vtkPlusAMFITRACKSource_h

#include "vtkPlusDataCollectionExport.h"
#include "vtkPlusDevice.h"

/*!
  \class vtkPlusAMFITRACKSource
  \brief Interface class to Intel RealSense cameras
  \ingroup PlusLibDataCollection
*/
class vtkPlusDataCollectionExport vtkPlusAMFITRACKSource : public vtkPlusDevice
{
public:

	static vtkPlusAMFITRACKSource* New();
	vtkTypeMacro(vtkPlusAMFITRACKSource, vtkPlusDevice);

	void PrintSelf(ostream& os, vtkIndent indent);

	/*! Get an update from the tracking system and push the new transforms to the tools. */
	PlusStatus InternalUpdate();

	virtual PlusStatus Probe();

	/*! Read configuration from xml data */
	virtual PlusStatus ReadConfiguration(vtkXMLDataElement* config);

	virtual bool IsTracker() const
	{
		return true;
	}

protected:
	vtkPlusAMFITRACKSource();
	~vtkPlusAMFITRACKSource();

	/*! Connect to the tracker hardware */
	PlusStatus InternalConnect();

	/*! Disconnect from the tracker hardware */
	PlusStatus InternalDisconnect();

private:
	vtkPlusAMFITRACKSource(const vtkPlusAMFITRACKSource&);
	void operator=(const vtkPlusAMFITRACKSource&);

	bool useUnityCoordinateSystem;
};

#endif