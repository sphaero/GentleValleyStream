/*
 * ofGstVideoUtils.cpp
 *
 *  Created on: 16/01/2011
 *      Author: arturo
 */

#include "ofCustomGstVideoPlayer.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include "ofConstants.h"


ofCustomGstVideoPlayer::ofCustomGstVideoPlayer(){
	nFrames						= 0;
	internalPixelFormat			= OF_PIXELS_RGB;
	nativePixels				= false;
	bIsStream					= true;
	bIsAllocated				= false;
	threadAppSink				= false;
	bAsyncLoad					= false;
	videoUtils.setSinkListener(this);
	fps_d = 1;
	fps_n = 1;
}

ofCustomGstVideoPlayer::~ofCustomGstVideoPlayer(){
	close();
}

bool ofCustomGstVideoPlayer::setPixelFormat(ofPixelFormat pixelFormat){
	internalPixelFormat = pixelFormat;
	if(pixelFormat==OF_PIXELS_NATIVE){
		nativePixels = true;
	}
	return true;
}

ofPixelFormat ofCustomGstVideoPlayer::getPixelFormat() const {
	return internalPixelFormat;
}

bool ofCustomGstVideoPlayer::createPipeline(string name){
	
	GstElement * gstPipeline = gst_parse_launch("udpsrc port=5000 ! application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! autovideoconvert name=decode" , NULL);
	//GstElement * gstPipeline = gst_element_factory_make("playbin","player");
	//g_object_set(G_OBJECT(gstPipeline), "port", name.c_str(), (void*)NULL);

	// create the oF appsink for video rgb without sync to clock
	GstElement * gstSink = gst_element_factory_make("appsink", "app_sink");

	gst_base_sink_set_sync(GST_BASE_SINK(gstSink), true);
	gst_app_sink_set_max_buffers(GST_APP_SINK(gstSink), 8);
	gst_app_sink_set_drop (GST_APP_SINK(gstSink),true);
	gst_base_sink_set_max_lateness  (GST_BASE_SINK(gstSink), -1);

#if GST_VERSION_MAJOR==0
	ofLogError() << "Only Gstreamer1.0 is supported";
#else
	string mime="video/x-raw";

	GstCaps *caps;
	if(internalPixelFormat==OF_PIXELS_NATIVE){
		//caps = gst_caps_new_any();
		caps = gst_caps_from_string((mime + ",format={RGBA,BGRA,RGB,BGR,RGB16,GRAY8,YV12,I420,NV12,NV21,YUY2}").c_str());
		/*
		GstCapsFeatures *features = gst_caps_features_new (GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META, NULL);
		gst_caps_set_features (caps, 0, features);*/
	}else{
		string format = ofGstVideoUtils::getGstFormatName(internalPixelFormat);
		caps = gst_caps_new_simple(mime.c_str(),
											"format", G_TYPE_STRING, format.c_str(),
											NULL);
		ofLogWarning() << "caps: " << gst_caps_to_string(caps);
	}
#endif


	gst_app_sink_set_caps(GST_APP_SINK(gstSink), caps);
	gst_caps_unref(caps);

	if(threadAppSink){
		GstElement * appQueue = gst_element_factory_make("queue","appsink_queue");
		g_object_set(G_OBJECT(appQueue), "leaky", 0, "silent", 1, (void*)NULL);
		GstElement* appBin = gst_bin_new("app_bin");
		gst_bin_add(GST_BIN(appBin), appQueue);
		GstPad* appQueuePad = gst_element_get_static_pad(appQueue, "sink");
		GstPad* ghostPad = gst_ghost_pad_new("app_bin_sink", appQueuePad);
		gst_object_unref(appQueuePad);
		gst_element_add_pad(appBin, ghostPad);

		gst_bin_add(GST_BIN(appBin), gstSink);
		gst_element_link(appQueue, gstSink);

		g_object_set (G_OBJECT(gstPipeline),"video-sink",appBin,(void*)NULL);
	}else{
		//g_object_set (G_OBJECT(gstPipeline),"video-sink",gstSink,(void*)NULL);
		gst_bin_add(GST_BIN(gstPipeline), gstSink);
		GstElement* decbin = gst_bin_get_by_name(GST_BIN(gstPipeline),"decode");
		gst_element_link (decbin, gstSink);
		gst_object_unref(decbin);
	}

	return videoUtils.setPipelineWithSink(gstPipeline,gstSink,bIsStream);
}

bool ofCustomGstVideoPlayer::load(string name){
	if( name.find( "file://",0 ) != string::npos){
		bIsStream = bAsyncLoad;
	}else if( name.find( "://",0 ) == string::npos){
		GError * err = NULL;
		gchar* name_ptr = gst_filename_to_uri(ofToDataPath(name).c_str(),&err);
		name = name_ptr;
		g_free(name_ptr);
		if(err) g_free(err);
		bIsStream = bAsyncLoad;
	}else{
		bIsStream = true;
	}
	ofLogVerbose("ofCustomGstVideoPlayer") << "loadMovie(): loading \"" << name << "\"";

	if(isInitialized()){
		gst_element_set_state (videoUtils.getPipeline(), GST_STATE_READY);
		if(!bIsStream){
			gst_element_get_state (videoUtils.getPipeline(), NULL, NULL, -1);
		}
		internalPixelFormat = OF_PIXELS_NATIVE;
		bIsAllocated = false;
		videoUtils.reallocateOnNextFrame();
		g_object_set(G_OBJECT(videoUtils.getPipeline()), "uri", name.c_str(), (void*)NULL);
		gst_element_set_state (videoUtils.getPipeline(), GST_STATE_PAUSED);
		if(!bIsStream){
			gst_element_get_state (videoUtils.getPipeline(), NULL, NULL, -1);
			return allocate();
		}else{
			return true;
		}
	}else{
		ofGstUtils::startGstMainLoop();
		return createPipeline(name) &&
				videoUtils.startPipeline() &&
				(bIsStream || allocate());
	}
}

void ofCustomGstVideoPlayer::setThreadAppSink(bool threaded){
	threadAppSink = threaded;
}


bool ofCustomGstVideoPlayer::allocate(){
	ofLogWarning() << "allocate";
	gst_element_set_state(videoUtils.getPipeline(), GST_STATE_PLAYING);
	if(bIsAllocated){
		return true;
	}

	guint64 durationNanos = videoUtils.getDurationNanos();

	nFrames		  = 0;
	ofLogWarning() << "Sink Element Name: " << gst_element_get_name(videoUtils.getSink()) << " numPads:" << videoUtils.getSink()->numpads;
	if(GstPad* pad = gst_element_get_static_pad(videoUtils.getSink(), "sink")){
		if(GstCaps *caps = gst_pad_get_current_caps (GST_PAD (pad))){
			GstVideoInfo info;
			gst_video_info_init (&info);
			if (gst_video_info_from_caps (&info, caps)){
				ofPixelFormat format = ofGstVideoUtils::getOFFormat(GST_VIDEO_INFO_FORMAT(&info));
				if(format!=internalPixelFormat){
					ofLogVerbose("ofCustomGstVideoPlayer") << "allocating as " << info.width << "x" << info.height << " " << info.finfo->description << " " << info.finfo->name;
					internalPixelFormat = format;
				}
				if(!videoUtils.allocate(info.width,info.height,format)) return false;
			}else{
				ofLogError("ofCustomGstVideoPlayer") << "allocate(): couldn't query width and height";
				return false;
			}

			fps_n = info.fps_n;
			fps_d = info.fps_d;
			nFrames = (float)(durationNanos / (float)GST_SECOND) * (float)fps_n/(float)fps_d;
			gst_caps_unref(caps);
			bIsAllocated = true;
		}else{
			ofLogError("ofCustomGstVideoPlayer") << "allocate(): cannot get pipeline caps";
			bIsAllocated = false;
		}
		gst_object_unref(GST_OBJECT(pad));
	}else{
		ofLogError("ofCustomGstVideoPlayer") << "allocate(): cannot get sink pad";
		bIsAllocated = false;
	}
	return bIsAllocated;
}

void ofCustomGstVideoPlayer::on_stream_prepared(){
	ofLogWarning() << "on_stream_prepared called";
	if(!bIsAllocated) allocate();
}

int	ofCustomGstVideoPlayer::getCurrentFrame() const {
	int frame = 0;

	// zach I think this may fail on variable length frames...
	float pos = getPosition();
	if(pos == -1) return -1;


	float  framePosInFloat = ((float)getTotalNumFrames() * pos);
	int    framePosInInt = (int)framePosInFloat;
	float  floatRemainder = (framePosInFloat - framePosInInt);
	if (floatRemainder > 0.5f) framePosInInt = framePosInInt + 1;
	//frame = (int)ceil((getTotalNumFrames() * getPosition()));
	frame = framePosInInt;

	return frame;
}

int	ofCustomGstVideoPlayer::getTotalNumFrames() const {
	return nFrames;
}

void ofCustomGstVideoPlayer::firstFrame(){
	setFrame(0);
}

void ofCustomGstVideoPlayer::nextFrame(){
	gint64 currentFrame = getCurrentFrame();
	if(currentFrame!=-1) setFrame(currentFrame + 1);
}

void ofCustomGstVideoPlayer::previousFrame(){
	gint64 currentFrame = getCurrentFrame();
	if(currentFrame!=-1) setFrame(currentFrame - 1);
}

void ofCustomGstVideoPlayer::setFrame(int frame){ // frame 0 = first frame...
	float pct = (float)frame / (float)nFrames;
	setPosition(pct);
}

bool ofCustomGstVideoPlayer::isStream() const {
	return bIsStream;
}

void ofCustomGstVideoPlayer::update(){
	videoUtils.update();
}

void ofCustomGstVideoPlayer::play(){
	videoUtils.play();
}

void ofCustomGstVideoPlayer::stop(){
	videoUtils.stop();
}

void ofCustomGstVideoPlayer::setPaused(bool bPause){
	videoUtils.setPaused(bPause);
}

bool ofCustomGstVideoPlayer::isPaused() const {
	return videoUtils.isPaused();
}

bool ofCustomGstVideoPlayer::isLoaded() const {
	return videoUtils.isLoaded();
}

bool ofCustomGstVideoPlayer::isPlaying() const {
	return videoUtils.isPlaying();
}

float ofCustomGstVideoPlayer::getPosition() const {
	return videoUtils.getPosition();
}

float ofCustomGstVideoPlayer::getSpeed() const {
	return videoUtils.getSpeed();
}

float ofCustomGstVideoPlayer::getDuration() const {
	return videoUtils.getDuration();
}

bool ofCustomGstVideoPlayer::getIsMovieDone() const {
	return videoUtils.getIsMovieDone();
}

void ofCustomGstVideoPlayer::setPosition(float pct){
	videoUtils.setPosition(pct);
}

void ofCustomGstVideoPlayer::setVolume(float volume){
	videoUtils.setVolume(volume);
}

void ofCustomGstVideoPlayer::setLoopState(ofLoopType state){
	videoUtils.setLoopState(state);
}

ofLoopType ofCustomGstVideoPlayer::getLoopState() const {
	return videoUtils.getLoopState();
}

void ofCustomGstVideoPlayer::setSpeed(float speed){
	videoUtils.setSpeed(speed);
}

void ofCustomGstVideoPlayer::close(){
	bIsAllocated = false;
	videoUtils.close();
}

bool ofCustomGstVideoPlayer::isFrameNew() const {
	return videoUtils.isFrameNew();
}

ofPixels& ofCustomGstVideoPlayer::getPixels(){
	return videoUtils.getPixels();
}

const ofPixels& ofCustomGstVideoPlayer::getPixels() const {
	return videoUtils.getPixels();
}

float ofCustomGstVideoPlayer::getHeight() const {
	return videoUtils.getHeight();
}

float ofCustomGstVideoPlayer::getWidth() const {
	return videoUtils.getWidth();
}

ofGstVideoUtils * ofCustomGstVideoPlayer::getGstVideoUtils(){
	return &videoUtils;
}

void ofCustomGstVideoPlayer::setFrameByFrame(bool frameByFrame){
	videoUtils.setFrameByFrame(frameByFrame);
}

bool ofCustomGstVideoPlayer::isThreadedAppSink() const{
	return threadAppSink;
}

bool ofCustomGstVideoPlayer::isFrameByFrame() const{
	return videoUtils.isFrameByFrame();
}

void ofCustomGstVideoPlayer::setAsynchronousLoad(bool async){
	bAsyncLoad = async;
}
