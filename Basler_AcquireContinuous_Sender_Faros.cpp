#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>

// Include files to use the PYLON API
#include <pylon/PylonIncludes.h>
using namespace Pylon;



#if defined( USE_1394 )
// Settings to use  Basler 1394 cameras
#include <pylon/1394/Basler1394Camera.h>
typedef
  Pylon::CBasler1394Camera
  Camera_t;
using namespace
  Basler_IIDC1394CameraParams;
using namespace
  Basler_IIDC1394StreamParams;
#elif defined ( USE_GIGE )
// settings to use Basler GigE cameras
#include <pylon/gige/BaslerGigECamera.h>
typedef
  Pylon::CBaslerGigECamera
  Camera_t;
using namespace
  Basler_GigECameraParams;
using namespace
  Basler_GigEStreamParams;
#else
#error Camera type is not specified. For example, define USE_GIGE for using GigE cameras
#endif

// Namespace for using cout
using namespace std;

extern "C"
{
#include "faros_polito_IMG_sender_basler.h"
}


double
get_now ()
{
  struct timeval timeval_now;
  gettimeofday (&timeval_now, NULL);
  double now = timeval_now.tv_sec + (double) timeval_now.tv_usec / 1000000.0;
  return now;
}




// This function can be used to wait for user input at the end of the sample program.
void
pressEnterToExit ()
{
  //comment the following two lines to disable wait on exit here
  cerr << endl << "Press enter to exit." << endl;
  while (cin.get () != '\n');
}

class CGrabBuffer
{
public:
  CGrabBuffer (const size_t ImageSize);
   ~CGrabBuffer ();
  uint8_t *GetBufferPointer (void)
  {
    return m_pBuffer;
  }
  StreamBufferHandle GetBufferHandle (void)
  {
    return m_hBuffer;
  }
  void SetBufferHandle (StreamBufferHandle hBuffer)
  {
    m_hBuffer = hBuffer;
  };

protected:
  uint8_t * m_pBuffer;
  StreamBufferHandle m_hBuffer;
};

// Constructor allocates the image buffer
CGrabBuffer::CGrabBuffer (const size_t ImageSize):
m_pBuffer (NULL)
{
  m_pBuffer = new uint8_t[ImageSize];
  if (NULL == m_pBuffer)
    {
      GenICam::
	GenericException e ("Not enough memory to allocate image buffer",
			    __FILE__, __LINE__);
      throw e;
    }
}

// Freeing the memory
CGrabBuffer::~CGrabBuffer ()
{
  if (NULL != m_pBuffer)
    delete[]m_pBuffer;
}


// Buffers for grabbing
static const uint32_t c_nBuffers = 2;	// CHANGED from 10
// Number of images to be grabbed
static const uint32_t c_ImagesToGrab = 1000000;	// CHANGED from 100

int
main (int argc, char *argv[])
{

  int width = 658, height = 492;
  int Pwidth = 640, Pheight = 480;
  int color = 0;

  uint8_t *Pimg = (uint8_t *) malloc (sizeof (uint8_t) * Pwidth * Pheight * 2);	// Assume YUYV ...

  main_sender (Pwidth, Pheight, argc, argv);

  // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
  // is initialized during the lifetime of this object
  Pylon::PylonAutoInitTerm autoInitTerm;

  try
  {
    // Get the transport layer factory
    CTlFactory & TlFactory = CTlFactory::GetInstance ();

    // Create the transport layer object needed to enumerate or
    // create a camera object of type Camera_t::DeviceClass()
    ITransportLayer *pTl = TlFactory.CreateTl (Camera_t::DeviceClass ());

    // Exit application if the specific transport layer is not available
    if (!pTl)
      {
	cerr << "Failed to create transport layer!" << endl;
	pressEnterToExit ();
	return 1;
      }

    // Get all attached cameras and exit application if no camera is found
    DeviceInfoList_t devices;
    if (0 == pTl->EnumerateDevices (devices))
      {
	cerr << "No camera present!" << endl;
	pressEnterToExit ();
	return 1;
      }

    // Create the camera object of the first available camera.
    // The camera object is used to set and get all available
    // camera features.
    Camera_t Camera (pTl->CreateDevice (devices[0]));

    // Open the camera
    Camera.Open ();

    // Get the first stream grabber object of the selected camera
    Camera_t::StreamGrabber_t StreamGrabber (Camera.GetStreamGrabber (0));

    // Open the stream grabber
    StreamGrabber.Open ();

    // Set the image format and AOI
    Camera.PixelFormat.SetValue (PixelFormat_Mono8);	// use monocromatic gray scale
    Camera.OffsetX.SetValue (0);
    Camera.OffsetY.SetValue (0);
    Camera.Width.SetValue (Camera.Width.GetMax ());
    Camera.Height.SetValue (Camera.Height.GetMax ());

    //Disable acquisition start trigger if available
    {
      GenApi::IEnumEntry * acquisitionStart =
	Camera.TriggerSelector.GetEntry (TriggerSelector_AcquisitionStart);
      if (acquisitionStart && GenApi::IsAvailable (acquisitionStart))
	{
	  Camera.TriggerSelector.SetValue (TriggerSelector_AcquisitionStart);
	  Camera.TriggerMode.SetValue (TriggerMode_Off);
	}
    }

    //Disable frame start trigger if available
    {
      GenApi::IEnumEntry * frameStart =
	Camera.TriggerSelector.GetEntry (TriggerSelector_FrameStart);
      if (frameStart && GenApi::IsAvailable (frameStart))
	{
	  Camera.TriggerSelector.SetValue (TriggerSelector_FrameStart);
	  Camera.TriggerMode.SetValue (TriggerMode_Off);
	}
    }

    //Set acquisition mode
    Camera.AcquisitionMode.SetValue (AcquisitionMode_Continuous);

    //Set exposure settings
    Camera.ExposureMode.SetValue (ExposureMode_Timed);

    //Camera.ExposureAuto.SetValue(ExposureAuto_Off);
    //Camera.ExposureTimeAbs.SetValue(80000.0);
    //Camera.ExposureAuto.SetValue(ExposureAuto_Once);
    Camera.ExposureTimeRaw.SetValue (5000);	// CHANGED from 100
    //Camera.ExposureTimeRaw.SetValue(3000);
    Camera.AcquisitionFrameRateEnable.SetValue (true);
    Camera.AcquisitionFrameRateAbs.SetValue (60.0);
    //Camera.AcquisitionFrameRateAbs.SetValue(50.0);

    //Stream. IP Config. Resend False
    //Stream. IP Config. Packet Timeout 1  // Anche se non dovrebbe essere usato
    //Stream. IP Config. Max Buff Size 323xxx   // 658*492
    //Stream. IP Config. Max Num Buf 1

    //Camera.GevSCPSPacketSize.SetValue(1400);
    Camera.GevSCPSPacketSize.SetValue (4076);	// multiple of 4  // Jumbo max 4078 su Intel 82577LM
    //Camera.GevSCFJM.SetValue(0); // Max delay of next frame
    //Camera.FrameTimeoutEnable.SetValue(true);
    //Camera.FrameTimeoutAbs.SetValue(12000);  // in us


    // Get the image buffer size
    const size_t ImageSize = (size_t) (Camera.PayloadSize.GetValue ());
    // cout << "ImageSize: " << ImageSize << endl;

    // We won't use image buffers greater than ImageSize
    StreamGrabber.MaxBufferSize.SetValue (ImageSize);

    // We won't queue more than c_nBuffers image buffers at a time
    StreamGrabber.MaxNumBuffer.SetValue (c_nBuffers);

    // ADDED: from Basler guide
    StreamGrabber.EnableResend.SetValue (false);

    //StreamGrabber.GevSCPD(0);  // in ticks, 1 tick = 8ns
    //StreamGrabber.GevSCFTD(0);

    // Allocate all resources for grabbing. Critical parameters like image
    // size now must not be changed until FinishGrab() is called.
    StreamGrabber.PrepareGrab ();

    // Buffers used for grabbing must be registered at the stream grabber.
    // The registration returns a handle to be used for queuing the buffer.
    std::vector < CGrabBuffer * >BufferList;
    for (uint32_t i = 0; i < c_nBuffers; ++i)
      {
	CGrabBuffer *pGrabBuffer = new CGrabBuffer (ImageSize);
	pGrabBuffer->SetBufferHandle (StreamGrabber.
				      RegisterBuffer (pGrabBuffer->
						      GetBufferPointer (),
						      ImageSize));

	// Put the grab buffer object into the buffer list
	BufferList.push_back (pGrabBuffer);
      }

    for (std::vector < CGrabBuffer * >::const_iterator x =
	 BufferList.begin (); x != BufferList.end (); ++x)
      {
	// Put buffer into the grab queue for grabbing
	StreamGrabber.QueueBuffer ((*x)->GetBufferHandle (), NULL);
      }

    // Let the camera acquire images continuously ( Acquisiton mode equals
    // Continuous! )
    Camera.AcquisitionStart.Execute ();

    // Grab c_ImagesToGrab times
    for (int n = 0; n < c_ImagesToGrab; n++)
      {
	double time_before = get_now ();

	// Wait for the grabbed image with timeout of 3 seconds
	if (StreamGrabber.GetWaitObject ().Wait (3000))
	  {

	    double time_after = get_now ();
	    //printf("Time in StreamGrabber.GetWaitObject().Wait(3000): %.6f s\n", time_after-time_before);

	    // Get the grab result from the grabber's result queue
	    GrabResult Result;
	    StreamGrabber.RetrieveResult (Result);

	    //printf("%d\n", (enum Pylon::GrabStatus)Result.Status());

	    if (1)		// CHANGED from: if ( Grabbed == Result.Status())
	      {
		// Grabbing was successful, process image
		const uint8_t *pImageBuffer = (uint8_t *) Result.Buffer ();
		//    printf("Image # %d acquired! Size: %d x %d.  Gray value of first pixel: %d\n", n, Result.GetSizeX(), Result.GetSizeY(), (uint32_t) pImageBuffer[0]);

		double now = get_now ();
		static double prev_now = 0;
		double diff = now - prev_now;
		//printf("time: %.6f diff %.6f\n", now, diff);
		prev_now = now;



		// ADDED read_frame((void *)pImageBuffer);
		// width*2 because YUYV
		{
		  int i, j;
		  int yoffx = (width * 2) * ((height - Pheight) / 2);
		  int xoff = (width - Pwidth) / 2;
		  uint8_t *pImBuf = (uint8_t *) pImageBuffer;
		  for (j = 0; j < Pheight; j++)
		    {
		      for (i = 0; i < Pwidth; i++)
			{
			  Pimg[j * (Pwidth * 2) + (i * 2)] =
			    pImBuf[yoffx + (j * width) + (xoff + i)];
			}
		    }
		}
		read_frame ((void *) Pimg);

		// Reuse the buffer for grabbing the next image
		if (n < c_ImagesToGrab - c_nBuffers)
		  StreamGrabber.QueueBuffer (Result.Handle (), NULL);
	      }
	    //else if (Failed == Result.Status())
	    else if (1)
	      {
		// Error handling
		cerr << "No image acquired!" << endl;
		cerr << "Error code : 0x" << hex
		  << Result.GetErrorCode () << endl;
		cerr << "Error description : "
		  << Result.GetErrorDescription () << endl;

		// Reuse the buffer for grabbing the next image
		if (n < c_ImagesToGrab - c_nBuffers)
		  StreamGrabber.QueueBuffer (Result.Handle (), NULL);
	      }
	  }
	else
	  {
	    // Timeout
	    cerr << "Timeout occurred!" << endl;

	    // Get the pending buffer back (You are not allowed to deregister
	    // buffers when they are still queued)
	    StreamGrabber.CancelGrab ();

	    // Get all buffers back
	    for (GrabResult r; StreamGrabber.RetrieveResult (r););

	    // Cancel loop
	    break;
	  }
      }

    // Stop acquisition
    Camera.AcquisitionStop.Execute ();

    // Clean up

    // You must deregister the buffers before freeing the memory
    for (std::vector < CGrabBuffer * >::iterator it = BufferList.begin ();
	 it != BufferList.end (); it++)
      {
	StreamGrabber.DeregisterBuffer ((*it)->GetBufferHandle ());
	delete *it;
	*it = NULL;
      }

    // Free all resources used for grabbing
    StreamGrabber.FinishGrab ();

    // Close stream grabber
    StreamGrabber.Close ();

    // Close camera
    Camera.Close ();

  }
  catch (GenICam::GenericException & e)
  {
    // Error handling
    cerr << "An exception occurred!" << endl << e.GetDescription () << endl;
    pressEnterToExit ();
    return 1;
  }
  // Quit the application
  pressEnterToExit ();
  return 0;
}
