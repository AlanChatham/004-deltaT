

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "OscListener.h"
#include "OscMessage.h"
#include <queue>
#include <vector>
#include <list>


using namespace ci;
using namespace ci::app;
using namespace std;



#define ROOM_WIDTH 300
#define ROOM_HEIGHT 200
#define ROOM_DEPTH 300

#define MAX_LINE_SIZE 100





class deltaTApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseMove( MouseEvent event );
	void update();
	void draw();
	void checkOSCMessage(const osc::Message * message);
	float constrain(float input, float min, float max);
	void setupGLLights(ColorA color);
	void drawRibbon(list<Vec3f> list0, list<Vec3f> list1);
	
	CameraPersp mCam;
	params::InterfaceGlRef mParams;
	list<Vec3f> pointList;
	vector<list<Vec3f>> pointListArray;
	Quatf mSceneRotation;
	Color mLineColor;

	osc::Listener oscListener;

	Vec2f			mMousePos;
	list<Vec3f> pList1;
	list<Vec3f> pList2;

	vector<ColorA> mColorList;

	int frameCounter;
};

void deltaTApp::mouseMove( MouseEvent event )
{
	mMousePos.x = event.getX() - getWindowWidth() * 0.5f;
	mMousePos.y = getWindowHeight() * 0.5f - event.getY();
}

ColorA colorFrom255(int r, int g, int b, int a){
	return ColorA(r/255.0, g/255.0, b/255.0, a/255.0);
}

void deltaTApp::setup()
{
	// Set up our camera. This probably won't move this time...
	mCam.setPerspective(60.0f, getWindowAspectRatio(), 1, 10000);
	Vec3f mEye = Vec3f(0,0,600);
	Vec3f mCenter = Vec3f(0,0, 0);//ROOM_DEPTH / 2 );
	Vec3f mUp = Vec3f::yAxis();
	mCam.lookAt( mEye, mCenter, mUp );
	
	// Set up a listener for OSC messages
	oscListener.setup(7114);
	

	// Set up our parameters object
	mParams = params::InterfaceGl::create( "deltaT", Vec2i( 225, 200 ) );
	mParams->addParam( "Scene Rotation", &mSceneRotation );
	mParams->addParam( "Color", &mLineColor );
	mLineColor = Color(.3f,.5f, .6f);
	mColorList.push_back(colorFrom255(0,181,164,255));
	mColorList.push_back(colorFrom255(0,255,231,255));
	mColorList.push_back(colorFrom255(222,255,252,255));
	mColorList.push_back(colorFrom255(253,68,25,255));
	mColorList.push_back(ColorA(1,.4,1, 1));
	mColorList.push_back(ColorA(.4,1,1, 1));

	// Let's get some test data
	pointList.push_back(Vec3f(0,0,0));
	pointList.push_back(Vec3f(100,0,0));
	pointList.push_back(Vec3f(150,50,0));
	pointList.push_back(Vec3f(175,30,80));
	pointList.push_back(Vec3f(200,200,30));
	pointList.push_back(Vec3f(150,-50,60));
	pointList.push_back(Vec3f(30,-50,0));
	pointList.push_back(Vec3f(0,0,0));
	pointList.push_back(Vec3f(20,100,-200));
	
	

	pList1.push_back(Vec3f(0,0,0));
	pList2.push_back(Vec3f(0,0,0) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(100,0,0));
	pList2.push_back(Vec3f(100,0,0) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(150,50,0));
	pList2.push_back(Vec3f(150,50,0) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(175,30,80));
	pList2.push_back(Vec3f(175,30,80) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(200,200,30));
	pList2.push_back(Vec3f(200,200,30) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(150,-50,60));
	pList2.push_back(Vec3f(150,-50,60) - Vec3f(-30,-30,-30));
	pList1.push_back(Vec3f(30,-50,0));
	pList2.push_back(Vec3f(30,-50,0) - Vec3f(-30,-30,-30));
	
	pointListArray.push_back(pList1);
	pointListArray.push_back(pList2);

	frameCounter = 0;
}

float deltaTApp::constrain(float input, float min, float max){
	if(input > max)
		return max;
	if (input < min)
		return min;
	return input;
}



void deltaTApp::checkOSCMessage(const osc::Message * message){
	// Sanity check that we have our legit head message
	if (message->getAddress() == "/head" && message->getNumArgs() == 3){
		float headX = message->getArgAsFloat(0);
		float headY = message->getArgAsFloat(1);
		float headZ = message->getArgAsFloat(2);

		//setCameras(Vec3f(headX, headY, headZ), false);
	}

	if (message->getAddress() == "/forearm" && message->getNumArgs() == 7){

		
		unsigned int ID    = message->getArgAsInt32(0);
		float handX = message->getArgAsFloat(1);
		float handY = message->getArgAsFloat(2);
		float handZ = message->getArgAsFloat(3);
		float elbowX = message->getArgAsFloat(4);
		float elbowY = message->getArgAsFloat(5);
		float elbowZ = message->getArgAsFloat(6);

		//3.28 feet in a meter
		handX = handX * 3.28f * 100;
		handY = handY * 3.28f * 100 - ( ROOM_HEIGHT / 3 ) * .8f; // Offset to bring up the vertical position of the eye
		handZ = handZ * 3.28f * 100;
	
		elbowX = elbowX * 3.28f * 100;
		elbowY = elbowY * 3.28f * 100 - ( ROOM_HEIGHT / 3 ) * .8f; // Offset to bring up the vertical position of the eye
		elbowZ = elbowZ * 3.28f * 100;
	

		// Front kinects
		if (handZ > 400){
			handZ -= 1500;
			elbowZ -= 1500;
			// Make sure that hands stay in the box-ish
			handX = constrain(handX, -290, 290);
			handZ = constrain(handZ, -290, 290);
			elbowX = constrain(elbowX, -290, 290);
			elbowZ = constrain(elbowZ, -290, 290);		
		}
		// side kinects
		if (handX < -400){
			handX += 1200;
			elbowX += 1200;
			handX *= 1.1f;
			elbowX *= 1.1f;
			// Make sure that hands stay in the box-ish
			handX = constrain(handX, -290, 290);
			handZ = constrain(handZ, -290, 290);
			elbowX = constrain(elbowX, -290, 290);
			elbowZ = constrain(elbowZ, -290, 290);
		}

		// Now that we've adjusted out data, let's put it into the arrays
		Vec3f handPosition = Vec3f(handX, handY, -handZ);
		Vec3f elbowPosition = Vec3f(elbowX, elbowY, -elbowZ);
		// Sanity check, then see what we can do
		while (pointListArray.size() < ((ID + 1) * 2)){
			list<Vec3f> l0;
			pointListArray.push_back(l0);
		}

		frameCounter++;
		//if (frameCounter % 91 == 1){
			pointListArray[ID * 2].push_back(handPosition);
			pointListArray[ID * 2 + 1].push_back(elbowPosition);
		//}
	}	
}

void deltaTApp::mouseDown( MouseEvent event )
{
}

void deltaTApp::update()
{
	// Update camera position data
	gl::setMatrices(mCam);
	gl::rotate(mSceneRotation);

	// Update line position data by culling the old data
	for (unsigned int i = 0; i < pointListArray.size(); i++){
		while (pointListArray[i].size() > MAX_LINE_SIZE){
			pointListArray[i].pop_front();
		}
	}

	// Parse OSC messages!
	while( oscListener.hasWaitingMessages() ) {
		osc::Message message;
		oscListener.getNextMessage( &message );
		
		checkOSCMessage(&message);
	}

}

void deltaTApp::setupGLLights(ColorA color){
	float red = color.r;
	float green = color.g;
	float blue = color.b;
	float alpha = color.a;
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	GLfloat light_position[] = { mMousePos.x, mMousePos.y, 75.0f, 0.0f };

	GLfloat mat_specular[] = { red, green, blue, alpha };
	GLfloat mat_diffuse[] = { red, green, blue, alpha};
	GLfloat mat_shininess[] = { 50.0 };
	glShadeModel (GL_SMOOTH);

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glLightfv( GL_LIGHT0, GL_POSITION, light_position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, mat_diffuse );
}

// Draws a ribbon between points outlined by our two vector lists
void deltaTApp::drawRibbon(list<Vec3f> l0, list<Vec3f> l1){
	glFrontFace(GL_CW);
	glBegin(GL_TRIANGLE_STRIP);
	for (list<Vec3f>::iterator i = l0.begin(), j = l1.begin();
													i != l0.end() && j != l1.end(); i++, j++){
		
		// If we're not at the end or the beginning, we can set the normal easily
		if(boost::next(i) != l0.end() && i != l0.begin()){
			Vec3f current = *i;
			Vec3f last = *boost::prior(i);
			Vec3f next = *boost::next(i);
			Vec3f u = current - last;
			Vec3f v = next - last;
			Vec3f normal = Vec3f::zero();
			normal.x = (u.y * v.z) - (u.z * v.y);
			normal.y = (u.z * v.x) - (u.x * v.z);
			normal.z = (u.x * v.y) - (u.y * v.x);
			normal.normalize();
			glNormal3f(normal);
		}
		gl::vertex(*i);
		// Normal for j
		if(boost::next(j) != l1.end() && j != l1.begin()){
			Vec3f current = *j;
			Vec3f last = *boost::prior(i);
			Vec3f next = *boost::next(i);
			Vec3f u = current - last;
			Vec3f v = next - last;
			Vec3f normal = Vec3f::zero();
			normal.x = (u.y * v.z) - (u.z * v.y);
			normal.y = (u.z * v.x) - (u.x * v.z);
			normal.z = (u.x * v.y) - (u.y * v.x);
			normal.normalize();
			glNormal3f(normal);
		}
		gl::vertex(*j);

	}
	glEnd();
}


void deltaTApp::draw()
{
	// clear out the window with white
	gl::clear( Color( 1, 1, 1 ) ); 
	gl::enableAlphaBlending();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	for (int i = 0; i+1 < pointListArray.size(); i += 2){
		ColorA lightingColor = mColorList[i % mColorList.size()];
		setupGLLights(lightingColor);
		drawRibbon(pointListArray[i], pointListArray[i+1]);
	}
	mParams->draw();
}

CINDER_APP_NATIVE( deltaTApp, RendererGl )
