/*  ********************************************************************************************************
//			RTSP CLIENT PROGRAM

//	Author:		Henry Portilla
//	Date:		june/2006
//	modified:	august/2006
//	Thanks

//	Description:	This program use the live555 libraries for make a RTSP
			client
										*/
// RFC 2326 RTSP protocol

/** M�odos soportados: OPTIONS
// Options permite obtener los m�odos soportados por el servidor de video
// DESCRIBE: Describe el tipo de videostream que se est�transmitiendo( tipo codec, protocolo de transmisi�(RTP),...)
//SETUP: Configura los puertos tanto en el cliente como en el servidor para efectuar la comunicaci� para efectuar la transmisi�
//TEARDOWN: Termina la sesi�
//PLAY:Reproducci� del video
//PAUSE:Pause del video
*/
//************************************************************************************************************
//	includes header files

#include "client.h"	

////////////////////////////////////////////////////////////////////////////////////////////////////
//	init libavcodec
int initCodecs()
{
	pCodecCtx=NULL;
	
//	initialize the codecs
	av_register_all();

//	Find codec for decode the video, here the MP4 codec
	pCodec =avcodec_find_decoder(CODEC_ID_MPEG4);
	if(pCodec==NULL)
		{
		printf("%s\n","codec not found");
		return -1;	//	codec not found
		}
//	allocate memory for AVodecCtx

	pCodecCtx = avcodec_alloc_context();
//	initialize width and height by now
	pCodecCtx->width=768;//768
	pCodecCtx->height=576;//576
	pCodecCtx->bit_rate = 1000000;

//	look for truncated bitstreams

//	if(pCodec->capabilities&&CODEC_CAP_TRUNCATED)
//	{
//		pCodecCtx->flags|= CODEC_FLAG_TRUNCATED;  /* we dont send complete frames */
//	}
	
//	open the codec
	if(avcodec_open(pCodecCtx,pCodec)<0)
		{
		printf("%s\n","cannot open the codec");
		return -1;	//	cannot open the codec
		}
//	allocate memory to the save a raw video frame
	pFrame = avcodec_alloc_frame();

//	allocate memory to save a RGB frame
	pFrameRGBA= avcodec_alloc_frame();
	if(pFrameRGBA==NULL)
	return -1;	//	cannot allocate memory

//	determine required buffer size for allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB24,512,512);
	buffer=new uint8_t[numBytes];	
	printf(" size of frame in bytes : %d\n",numBytes);

//	get data from video
	
//	fps = (pFormatCtx->bit_rate/1000)*(pFormatCtx->duration) / numBytes;
	//printf(" fps  : %f\n",fps);
//	assign appropiate parts of buffer to image planes in pFrameRGB
	avpicture_fill((AVPicture*)pFrameRGBA,buffer,PIX_FMT_RGB24,512,512);
	return 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////
//	this function calculates the time arrival of the frames

timeval timeNow() // 
{
	//unsigned long RTPtime;
	timeval t;
	int time;
	//time = ntp_gettime(&ntpTime);
	time = gettimeofday(&t,&tz);
	if (time==0)
	{
		//printf("time of arrival was:%li.%06li\n",ntpTime.time.tv_sec,ntpTime.time.tv_usec);
		//printf("time arrival of frame %d was:%li.%06li\n",frameCounter,t.tv_sec,t.tv_usec);
		//printf("time was:%li.%06li\n",t.tv_sec,t.tv_usec);
		//return t;
	}else{
		printf("error was:%i\n",errno);
		
	}
		
	//t = time(NULL);
	//RTPtime = converter->convertToRTPTimestamp(Timevint size;al::Timeval(50,20));
	//printf("seconds was %li\n",t);
	//return RTPtime;
	return t;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int timeNow2()
{
	timeval t;
	int time;
	//time = ntp_gettime(&ntpTime);
	time = gettimeofday(&t,&tz);
	if (time==0)
	{
		//printf("time of arrival was:%li.%06li\n",ntpTime.time.tv_sec,ntpTime.time.tv_usec);
		//printf("time of timer was:%li.%06li\n",t.tv_sec,t.tv_usec);
		//printf("time was:%li.%06li\n",t.tv_sec,t.tv_usec);
		//return t;
	}else{
		printf("error was:%i\n",errno);
		
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//	function to execute after Reading data from RTP source
//void *clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds
void afterReading(void *clientData,unsigned framesize,unsigned /*numTruncatedBytes*/,
				struct timeval presentationTime,unsigned /*durationInMicroseconds*/)
{
	

	//	test size of frame
if(framesize>=maxRTPDataSize)
{
	printf("framesize: %d is greater that maxRTPDATASize: %i\n",framesize,maxRTPDataSize);
	}else{
	       //printf("Data read is: %d\n",framesize);
	       }
	
	//unsigned char *Data = new unsigned char[maxRTPDataSize];
	//unsigned char *Data = new unsigned char[framesize];
	dataBuffer.data = (unsigned char*)clientData;
	dataBuffer.size = framesize;
	
	//	we get the data // 
	data_RTP.size =  MP4Hsize + dataBuffer.size;				//	save MP4Header
	//	resize  buffer to save data
   	memcpy(data_RTP.data,MP4H,MP4Hsize);				//	save frame in memory
	memmove(data_RTP.data + MP4Hsize, dataRTP, dataBuffer.size);
	//memcpy(data_RTP.data, dataRTP, dataBuffer.size);
										//	save timestamp
	data_RTP.timestamp = Subsession->rtpSource()->curPacketRTPTimestamp();
 	data_RTP.time = timeNow();
	data_RTP.index= frameCounter;					//	time arrival
	//actualRTPtimestamp = 
	//printf("current RTP timestamp %lu\n",data_RTP.timestamp);
	//printf("Subsession ID: %s\n",Subsession->sessionId);
	if (dataBuffer.data == NULL)
	{ 
		printf("%s \n","data was not read");
	}else{
		
	//	printf("Data size: %i\n",dataBuffer.size);
		//printf("%i\n",strlen(dataBuffer.data));
	}
		
	
	       readOKFlag = ~0;	//	set flag to new data
	      // delete Data;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//	function to execute when the RTP is closed
void onClose(void *clientData)
{

	readOKFlag = ~0;	//	set flag to new data
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/*	this function calculates the skew  between sender/receiver
//	to know if the sender or  the receiver is fastest that other
//	see chapter 6 C. perkins book page 176
			
		s = (timestampN) - timestampN-1 / (timearrivalN - timearrivalN-1)

	here, we use double instead of unsigned long that is used to timestamps
*/
double skew(dataFrame N, dataFrame N_1)
{
	double d,d1,d2,s,tstampN,tstampN_1;
	//	differences between 2 frames
	d1 = (N.time.tv_sec + (N.time.tv_usec/1000000.0)); 
	d2 = (N_1.time.tv_sec + (N_1.time.tv_usec/1000000.0));

	//printf("time arrival 1: %06f\n",d);	
	//printf("time arrival 2: %06f\n",d2);
	d = d1-d2;
	//printf("time arrival difference: %06f\n",d);
	d = 90000.0*d;	//	multiplies by timestamp frequency, here 90000 for video
	//printf("time arrival difference: %06f\n",d);
	tstampN = N.timestamp;
	tstampN_1 = N_1.timestamp;
	s = (tstampN - tstampN_1);
	//printf("skew: %06f\n",s);
	//	calculates skew
	s = s/d;
	//printf("skew: %06f\n",s);
	return s;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//		this function start a connection with a RTSP source data
//		description:
//			input: 	URL as "rtsp://sonar:7070/cam1"
//			output: A subsession associated to the URL
int rtsp_Init(int camNumber)// 
 {
	const char *name = "RTSP";
	char const *URL;	//	to save address to access
//	LIST OF CAMERAS TO ACCESS

	switch(camNumber)	//	update image
	{
		case 0:
			URL = URL0;
			break;
		case 1:
			URL = URL1;
			break;
		case 2:
			URL = URL2;
			break;
		case 3:
			URL = URL3;
			break;
		default:
			URL = URL0;
			break;
	}


/////////////////	RTSP START	/////////////////

	//	assign an environment to work
	TaskScheduler *scheduler = BasicTaskScheduler::createNew();				
	if (scheduler==NULL)
		{printf("error %s\n"," scheduler was not created");
		return -1;}
	env = BasicUsageEnvironment::createNew(*scheduler);	
	if (env==NULL)
		{printf("error %s\n"," environment was not created");
		return -1;}
	//	create a RTSP client
	client =  RTSPClient::createNew(*env,verbosityLevel,name,tunnelOverHTTPPortNum); 	
	;
	if (client==NULL)
		{printf("error %s\n"," rtsp client was not created");
		return -1;}

////////////////////////////////////////////////////////
	//	RTSP PROTOCOL
	
	//	Send OPTIONS method
	
	getOptions = client->sendOptionsCmd(URL,NULL,NULL,NULL);	//	connect to server
	printf("OPTIONS are: %s\n",getOptions);				//	print the response
		
	//	send DESCRIBE method
	
	getDescription = client->describeURL(URL);		//	get the Session description
	printf("DESCRIBE is: \n%s\n",getDescription);			//	print this description

	//	setup live libraries for send SETUP metunsigned long timestamp;hod
	
	Session = MediaSession::createNew(*env,getDescription);		//	create session
	if (Session==NULL)
		{printf("error %s\n","Session was not created");
		return -1;}
	
	//	create a subsession for the RTP receiver (only video for now)
	
	
	MediaSubsessionIterator iter(*Session);// iter;
	//iter(*Session);

	if((Subsession= iter.next())!=NULL)		//	 is there a valid subsession?
	{
	
	//	look for video subsession

		if(strcmp(Subsession->mediumName(),"video")==0)		//	if video subsession
		{
			//ReceptionBufferSize =2000000;			//	set size of buffer reception
		}//else{
		//continue;}
		
		//	assign RTP port,  it must be even number    RFC3550?
	
		//Subsession->setClientPortNum(RTPport);
	
		//	initiate  subsession
		if(!Subsession->initiate())
		{
		//	 print error when subsession was not initiated
		printf("Failed to initiate RTP subsession %s %s %s \n",Subsession->mediumName(),Subsession->codecName(),env->getResultMsg());
		}else{
		//	print info about RTP session type, codec,port number
		printf("RTP %s/%s subsession  initiated on port %d\n",Subsession->mediumName(),Subsession->codecName(),Subsession->clientPortNum());
		}
		//	timestamp clock
		timestampFreq = Subsession->rtpSource()->timestampFrequency();
		printf("timestamp frequency %u\n",timestampFreq);

		//	config line, VOL header or  VOP header?
		MP4Header = Subsession->fmtp_config();
		MP4Hsize = strlen(MP4Header);
		//	convert to unsigned char
		MP4H = reinterpret_cast<unsigned char*>(const_cast<char*>(MP4Header));
		printf("Mp4 header: %s\n",MP4H);			//	print MPEG4 header
		
		//	send SETUP command

		if(client!=NULL)
		{
		SetupResponse = client->setupMediaSubsession(*Subsession,False,False,False);
		if(SetupResponse==False)
			{
				printf("%s \n","SETUP was not sent");
				return -1;
			}
		//	review status code
		printf("Subsession  ID: %s\n",Subsession->sessionId);
		//printf("URL %u\n",Subsession->rtpInfo.trackId);
		printf("URL %u\n",Subsession->rtpChannelId);
		//printf("SSRC ID: %u\n",Subsession->rtpSource()->lastReceivedSSRC());
		}	

		//	now send PLAY method
		if(client!=NULL)
		{
	
			if(client->playMediaSession(*Session)==0)
			{
				printf("%s\n","PLAY command was not sent");
				return -1;
			}
		}
	}
	//iter.reset();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//	this function close a rtsp connection
int rtsp_Close()
{
	//	CLOSE RTSP CONNECTION
	if(client !=NULL)
	{
	//	send TEARDOWN command
		client->teardownMediaSession(*Session);
		
	}
return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//int rtsp_getFrame(unsigned char *image)
int rtsp_getFrame()
{
	
	unsigned Var;
	//cod = pthread_mutex_lock(&mutexFrame);
	//	get the data from rtp //readSource
	Subsession->readSource()->getNextFrame(dataRTP,maxRTPDataSize,
								afterReading,(void*)Var,
								onClose,(void*)Var);
	//	wait until data is available, it allows for other task to be performed
	//	while we wait data from rtp source					                                                        	
	readOKFlag = 0;								                               //	schedule read                         	
	TaskScheduler& scheduler = Subsession->readSource()->envir().taskScheduler();
	scheduler.doEventLoop(&readOKFlag);
	//printf("readOKFlag is: %i\n",readOKFlag );
	//timeNow();
	frameCounter++;
	//usleep(delay);
	//cod = pthread_mutex_unlock(&mutexFrame);
	
	return 0;	//	exit with sucess
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
int rtsp_decode(dataFrame dataCompress)
{
	//	decode the video frame using libavcodec

	decodeOK = avcodec_decode_video(pCodecCtx,pFrame,&frameFinished,dataCompress.data,dataCompress.size);
		
	//if(decodeOK!=0)
		//{printf("%s\n","error in decoding");}
	//else{
	//	if we get a frame then convert to RGB
		if(frameFinished)
		{
		//	convert the image from his format to RGB
			img_convert((AVPicture*)pFrameRGBA,PIX_FMT_RGB24,(AVPicture*)pFrame,pCodecCtx->pix_fmt,512,512);
					
			//pCodecCtx->width,pCodecCtx->height);
			//static unsigned char image[]={pFrameRGB->data[0]};
			//	save to a buffer
			data_RTP.image = pFrameRGBA->data[0];	//	save RGB frame
			//memcpy(data_RTP.image,image,1327104);
			//Temp.image = image;
			//frameCounter++;
			//printf("frame %d was decoded\n",frameCounter);
			//	get the timestamp of the frame
			//break;
		}
		else{
			//frameCounter++;	
			printf("there was an error while decoding the frame %d in the camera\n",frameCounter);
			//image = pFrame->data[0];
		}
		
	//}
	return 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////
//		this function start to get n frames: the compressed frames
//		this acts like a buffer to save tha last frame get it and to read and decode 
//		one appropiate frame

//		At start it can save n frames to be used like a buffer to let smooth the rate
//		of showinf frames  and too to decrease the network jitter of packets
//		here, this decouples the reception part of the showing part
int rtsp_Buffering(int n)
{
	double s;
	for (int i=0; i<n;i++)	//	save n compressed frames
	{
		rtsp_getFrame();
		rtsp_decode(data_RTP);
		InputBuffer.push_back(data_RTP);		//	save to the queue
		
		if (i>1)
			{
			dataFrame N = InputBuffer.at(i);
			dataFrame N_1 = InputBuffer.at(i-1);
			s = skew(N,N_1);
			//printf("skew : %06f\n",s);	
			}
		
		//T.push(i);
	}
return 0;
}
// ///////////////////////////////////////////////////////
                           //////////////////////////////////////////
//	Getting data thread
void* rtsp_getData(void *)
{
	double s;
	//int CAMERA;
	//CAMERA = *(int*)arg;		//	cast to integer
	int cod;
	while(1)	//	threads is always in execution
	{
		//usleep(delay);	//	delay
	
		//	LOCK THE RESOURCE
		
		cod = pthread_mutex_lock(&mutexBuffer);
                           
		if (cod!=0)	
			{
				//printf("%s\n","Error locking get RTP data");
			}
		else{
		//condition
			
			rtsp_getFrame();
			rtsp_decode(data_RTP);
			//sem_post(&sem);
			
			if(!InputBuffer.empty())
				{
				
				dataFrame N = data_RTP;
				dataFrame N_1 = InputBuffer.back();
				s = skew(N,N_1);
				//printf("skew : %06f\n",s);
				//s= skew(Temp,InputBuffer.back());
				
				}
			InputBuffer.push_back(data_RTP);
			//counter++;
			//T.push(counter);
			printf("writing frame %d from the camera \n",frameCounter);
			//printf("FIFO size: %d\n",InputBuffer.size());
		

		//	WAKE UP THE OTHER THREAD: SHOW DATA THREAD
			//cod = pthread_cond_signal(&cond[1]);
			sem_post(&sem);
			
		}

		cod = pthread_mutex_unlock(&mutexBuffer);
		if (cod!=0)	
			{printf("%s\n","Error unlocking get RTP data");}

		
	}
	return 0;

}

//	update function show the frame from cameras

void update(void *data,SoSensor*)	//	this function updates the texture based on the frame got
					//	from the video stream
{
	//	get the video frames
	
	//int CAM = 1;
	SoTexture2 *rightImage = (SoTexture2*)data;
	sem_wait(&sem);	//	update image
	//	WAIT FOR SEMAPHORE
	
	
	//pthread_mutex_trylock(&mutexBuffer);
	if(!InputBuffer.empty())
	{
		//int t = T.front();
                //T.pop();
		//printf("decoding: %d\n",t);
			
		ReceivedFrame = InputBuffer.front();	//	get the frame from the FIFO buffer

//sem_wait(&sem);	//	update image

		rightImage->image.setValue(SbVec2s(512,512),3,ReceivedFrame.image);
		//ReceivedFrame = IB.front();
		//rtsp_decode(ReceivedFrame);
		//	INCREASE SEMAPHORE: DATA IS DECODED AND READY TO SHOW
		//ShowBuffer.push(data_RTP);		//	save decoded frame
		printf("update image No: %d: from camera \n",ReceivedFrame.index);
		timeNow2();
		//sem_post(&sem);				
		InputBuffer.pop_front();		//	delete frame from the FIFO buffer
		//IB.pop_front();
		printf("FIFO size: %d\n",InputBuffer.size());
		
		
	}else
	{
		printf("empty buffer %d\n", InputBuffer.size());
	}

	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//		MAIN PROGRAM
int main(int argc,char **argv)
{
	
	int K,cod,status;
	
	//delay = atoi(argv[1]);
	cam = atoi(argv[1]);		// choose the camera to see
	//cam[1] = 3;//atoi(argv[2]);
		
	//******************************************************************************************
	//	Allocation of  memory to save the compressed and uncompressed frames
	
	dataRTP = new unsigned char[70000];			
	//dataRTP = new unsigned char[70000];
	
	data_RTP.data = new unsigned char[70000];		//	uncompressed frames with MPEG4 Headers
	//data_RTP.data = new unsigned char[70000];


	//******************************************************************************************
	//	INIT THE VIDEO CODECS	

	
	if (initCodecs()==-1)					//	init libavcodec
		{
			printf("codecs for camera  was not initialized");	//	codec not initialized
			return -1;
		}
	
	//******************************************************************************************
	//	Start threads and semaphores

		sem_init(&sem,0,0);				//	start semaphores with a  value of 0 = blocked	
		cod = pthread_mutex_init(&mutexBuffer,0);	//	start mutex exclusion of buffers
		if (cod!=0)	
			{printf(" Error initilization on mutex %d\n",K);}
		
	//	connect to RTSP server
		status = rtsp_Init(cam);			//	Init RTSP Clients for left and right cameras
	
	// ******************************************************************************************** 
	//		Graphics Scene
	
	// Initializes SoQt library (and implicitly also the Coin and Qt
    	// libraries). Returns a top-level / shell Qt window to use.
    	 QWidget * mainwin = SoQt::init(argc, argv, argv[0]);

	//SoDB::init();
	//SoDB::setRealTimeInterval(1/120.0);
	SoSeparator *root = new SoSeparator;
    	root->ref();
	
	//	MAKE A PLANE FOR IMAGE 

	SoGroup *plane = new SoGroup;
	
	SoCoordinate3 *Origin = new SoCoordinate3;
	plane->addChild(Origin);			//	set plane origin at (0,0,0)
	
	Origin->point.set1Value(3,SbVec3f( 512/2,  512/2, 0));
	Origin->point.set1Value(2,SbVec3f( 512/2, -512/2, 0));
	Origin->point.set1Value(1,SbVec3f(-512/2, -512/2, 0));
	Origin->point.set1Value(0,SbVec3f(-512/2,  512/2, 0));
	
	//	define the square's normal
	SoNormal *normal = new SoNormal;				//	start buffering n frames;
	plane->addChild(normal);
	normal->vector.set1Value(0,SbVec3f(0,0,1));	//	normal to z	
	
	//	define the texture coordinates

	SoTextureCoordinate2 *TextCoord = new SoTextureCoordinate2;
	plane->addChild(TextCoord);
	TextCoord->point.set1Value(0,SbVec2f(0,0));
	TextCoord->point.set1Value(1,SbVec2f(0,1));
	TextCoord->point.set1Value(2,SbVec2f(1,1));
	TextCoord->point.set1Value(3,SbVec2f(1,0));
	
	// define normal and texture coordinate binding
	
	SoNormalBinding *nBind = new SoNormalBinding;
	SoTextureCoordinateBinding *TextBind = new SoTextureCoordinateBinding;
	plane->addChild(nBind);	
	plane->addChild(TextBind);

                           
	nBind->value.setValue(SoNormalBinding::OVERALL);
	TextBind->value.setValue(SoTextureCoordinateBinding::PER_VERTEX);

	//	define a faceset

	SoFaceSet *Face = new SoFaceSet;
	plane->addChild(Face);
	Face->numVertices.set1Value(0,4);

	// MAKE LEFT AND RIGHT PLANES	

	//	LEFT PLANE
	
	SoSeparator *leftPlane = new SoSeparator;
	
	//	ADD THE IMAGE FROM THE FIRST CAMERA
	SoTexture2  *leftImage = new SoTexture2;
	leftImage->filename.setValue("");	// this set is for use an image from memory in place of a file */

	SoTransform *leftTransform = new SoTransform;
	leftTransform->translation.setValue(-512/2,0.0,0.0);
	leftPlane->addChild(leftTransform);
	leftPlane->addChild(leftImage);
	leftPlane->addChild(plane);
	
	//	RIGHT PLANE

	SoSeparator *rightPlane = new SoSeparator;

	//	ADD THE IMAGE FROM THE SECOND CAMERA
	SoTexture2 *rightImage = new SoTexture2;
	rightImage->filename.setValue("");

	SoTransform *rightTransform = new SoTransform;
	rightTransform->translation.setValue(512/2,0.0,0.0);
		
	rightPlane->addChild(rightTransform);
	rightPlane->addChild(rightImage);
	rightPlane->addChild(plane);
	
	//	ADD THE TWO PLANES

	root->addChild(leftPlane);		// 
	root->addChild(rightPlane);

	//	START TO READ THE DATA FROM CAMERAS
                           

	if(client==NULL)
		{
		printf(" client is not available\n");
		return -1;

		}
	
	if(Subsession->readSource()!=NULL)	//	valid data source
		{
		if(strcmp(Subsession->mediumName(),"video")==0)// before while  // if
			{
			rtsp_Buffering(4);	//	start buffering n frames
			//pthread_create(&camera[0],0,ShowData,(SoTexture2*)robot);
			cod = pthread_create(&camera[0],0,rtsp_getData,0);
			//pthread_create(&camera[2],0,updateImage,(SoTexture2*)robot);
			if (cod!=0)
				{printf("error creating thread No %d\n",K);}
			else{
				printf("creating thread No %d\n",K);
				}
			}
	}

	//****************************************************************************
	//	setup timer sensor for recursive image updating 

			
	SoTimerSensor *timer = new SoTimerSensor(update,rightImage);
	//
	timer->setBaseTime(SbTime::getTimeOfDay());//atoi(argv[3])
	timer->setInterval(1.0/25.0);//fps	//	set interval 40 ms
	timer->schedule();				//	enable

     	SoTransform *myTrans = new SoTransform;
	root->addChild(myTrans);
   	myTrans->translation.setValue(0.0,0.0,200.0);
	
	// Use one of the convenient SoQt viewer classes.
	SoQtExaminerViewer * eviewer = new SoQtExaminerViewer(mainwin);
    	eviewer->setSceneGraph(root);
    	eviewer->show();
	// Pop up the main window.
    	SoQt::show(mainwin);
    	// Loop until exit.
    	SoQt::mainLoop();

	//***************************************************************************
	//			wait until the threads finish
	
	pthread_join(camera[0],0);
	//pthread_join(camera[0],0);
	//pthread_join(camera[2],0);
	//pthread_join(camera[3],0);
	//****************************************************************************
	// 			Clean up resources.				  
    	delete eviewer;
    	root->unref();
	delete dataRTP;
	delete data_RTP.data;

		
return 0;
}	



