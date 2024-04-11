/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"
#include "vtkPlusAMFITRACKSource.h"

// Local includes
#include "vtkPlusChannel.h"
#include "vtkPlusDataSource.h"

// AMFITRACK includes
#include <Amfitrack.hpp>

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkMath.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

vtkStandardNewMacro(vtkPlusAMFITRACKSource);

//----------------------------------------------------------------------------
vtkPlusAMFITRACKSource::vtkPlusAMFITRACKSource()
    : vtkPlusDevice()
{
    LOG_DEBUG("Amfitrack instance created");
    this->StartThreadForInternalUpdates = true;
    this->FrameNumber = 0;
    useUnityCoordinateSystem = false;
}

//----------------------------------------------------------------------------
vtkPlusAMFITRACKSource::~vtkPlusAMFITRACKSource()
{
    LOG_DEBUG("Amfitrack instance deleted");
}

//----------------------------------------------------------------------------
void vtkPlusAMFITRACKSource::PrintSelf(ostream& os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
    os << indent << "AMFITRACK Active" << std::endl;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusAMFITRACKSource::ReadConfiguration(vtkXMLDataElement* rootConfigElement)
{
    LOG_TRACE("vtkPlusAMFITRACKSource::ReadConfiguration");
    XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);

    XML_FIND_NESTED_ELEMENT_REQUIRED(dataSourcesElement, deviceConfig, "DataSources");
    for (int nestedElementIndex = 0; nestedElementIndex < dataSourcesElement->GetNumberOfNestedElements(); nestedElementIndex++)
    {
        vtkXMLDataElement* toolDataElement = dataSourcesElement->GetNestedElement(nestedElementIndex);

        if (toolDataElement->GetAttribute("UnityCoordinateSystem") != NULL)
        {
            // this tool has an associated rigid body definition
            const char* UnityCoordinateSystem = toolDataElement->GetAttribute("UnityCoordinateSystem");
            if (strcmp(UnityCoordinateSystem, "True") == 0 ||
                strcmp(UnityCoordinateSystem, "true") == 0 ||
                strcmp(UnityCoordinateSystem, "TRUE") == 0)
            {
                LOG_INFO("Amfitrack using unity coordinate system");
                useUnityCoordinateSystem = true;
            }
            else
            {
                LOG_INFO("Amfitrack using normal coordinate system");
                useUnityCoordinateSystem = false;
            }
        }
        else
        {
            LOG_INFO("No UnityCoordinateSystem found in config, using normal")
        }
    }

    return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusAMFITRACKSource::Probe()
{
    return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusAMFITRACKSource::InternalConnect()
{
    LOG_DEBUG("Amfitrack starting initializing");
    AMFITRACK& AMFITRACK = AMFITRACK::getInstance();
    AMFITRACK.initialize_amfitrack();
    LOG_DEBUG("Amfitrack starting main thread");
    AMFITRACK.start_amfitrack_task();

	return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusAMFITRACKSource::InternalDisconnect()
{
    LOG_DEBUG("Amfitrack stopping main thread");
    AMFITRACK& AMFITRACK = AMFITRACK::getInstance();
    AMFITRACK.stop_amfitrack_task();


	return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusAMFITRACKSource::InternalUpdate()
{
    AMFITRACK& AMFITRACK = AMFITRACK::getInstance();

    for (int deviceNumber = 0; deviceNumber < MAX_NUMBER_OF_DEVICES; deviceNumber++)
    {
        if (AMFITRACK.getDeviceActive(deviceNumber))
        {
            std::string str_DeviceNumber = std::to_string(deviceNumber);

            vtkPlusDataSource* AF_Source(nullptr);
            if (this->GetToolByPortName(str_DeviceNumber, AF_Source) == PLUS_SUCCESS)
            {
                lib_AmfiProt_Amfitrack_Pose_t pose;
                vtkNew<vtkMatrix4x4> matrix;
                matrix->Identity();
                AMFITRACK.getDevicePose(deviceNumber, &pose);

                // convert translation to mm
                double translation[3] = { pose.position_x_in_m * 1000, pose.position_y_in_m * 1000, pose.position_z_in_m * 1000 };
                double quaternion[4] = { pose.orientation_w, pose.orientation_x, pose.orientation_y, pose.orientation_z };

                if (useUnityCoordinateSystem)
                {
                    translation[0] = pose.position_x_in_m * 1000;
                    translation[1] = pose.position_z_in_m * 1000;
                    translation[2] = pose.position_y_in_m * 1000;

                    quaternion[0] = pose.orientation_w;
                    quaternion[1] = -pose.orientation_x;
                    quaternion[2] = -pose.orientation_z;
                    quaternion[3] = -pose.orientation_y;
                }

                // convert rotation from quaternion to 3x3 matrix
                double rotation[3][3] = { 0,0,0, 0,0,0, 0,0,0 };
                vtkMath::QuaternionToMatrix3x3(quaternion, rotation);

                // construct the transformation matrix from the rotation and translation components
                for (int i = 0; i < 3; ++i)
                {
                    for (int j = 0; j < 3; ++j)
                    {
                        matrix->SetElement(i, j, rotation[i][j]);
                    }
                    matrix->SetElement(i, 3, translation[i]);
                }

                const double unfilteredTimestamp = vtkIGSIOAccurateTimer::GetSystemTime();
                this->ToolTimeStampedUpdate(AF_Source->GetId(), matrix, ToolStatus(TOOL_OK), this->FrameNumber, unfilteredTimestamp);
                this->FrameNumber++;
            }
        }
    }

    return PLUS_SUCCESS;
}
