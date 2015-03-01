#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

    gst.setPipeline("udpsrc port=5000 ! application/x-rtp,encoding-name=H264,payload=96 ! rtpjitterbuffer latency=200 ! rtph264depay ! h264parse ! vaapidecode ! glcolorscale", OF_PIXELS_RGB, true);
    //gst.setPipeline("udpsrc port=5000 ! application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! decodebin ! videoconvert", OF_PIXELS_RGB, true);
    gst.startPipeline();
    gst.play();
    tex.allocate(1920,1080,GL_RGB);
}

//--------------------------------------------------------------
void ofApp::update(){
    gst.update();
    if(gst.isFrameNew()){
        if (tex.getWidth() != gst.getWidth())
        {
            tex.allocate(gst.getWidth(), gst.getHeight(), GL_RGB);
        }
        tex.loadData(gst.getPixels(),gst.getWidth(), gst.getHeight(), GL_RGB);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    tex.draw(20,20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
