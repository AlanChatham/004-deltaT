

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "OscListener.h"
#include "OscMessage.h"
#include "HeadCam.h"
#include <queue>
#include <vector>
#include <list>


using namespace ci;
using namespace ci::app;
using namespace std;

#define APP_WIDTH		2560
#define APP_HEIGHT		720

#define ROOM_WIDTH 300
#define ROOM_HEIGHT 200
#define ROOM_DEPTH 300

#define MAX_LINE_SIZE 100





class deltaTApp : public AppNative {
  public:
	virtual void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseMove( MouseEvent event );
	void update();
	void draw();
	void drawGuts(Area area);
	void checkOSCMessage(const osc::Message * message);
	float constrain(float input, float min, float max);
	void setupGLLights(ColorA color);
	void drawRibbon(list<Vec3f> list0, list<Vec3f> list1);
	void setCameras(Vec3f headPosition, bool fromKeyboard);


	CameraPersp mCam0;
	CameraPersp mCam1;
	CameraPersp mActivePerspCam;
	HeadCam mActiveCam;
	HeadCam mHeadCam0;
	HeadCam mHeadCam1;

	params::InterfaceGlRef mParams;
	list<Vec3f> pointList;
	vector<list<Vec3f>> pointListArray;
	Quatf mSceneRotation;
	Color mLineColor;
	Vec3f lightPosition;

	osc::Listener oscListener;

	Vec2f			mMousePos;
	list<Vec3f> pList1;
	list<Vec3f> pList2;

	vector<ColorA> mColorList;

	int frameCounter;
};

void deltaTApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
	//settings->setBorderless();
	//settings->setWindowPos(0,0);
}

void deltaTApp::setCameras(Vec3f headPosition, bool fromKeyboard = false){
	// Separate out the components	
	float headX = headPosition.x;
	float headY = headPosition.y;
	float headZ = headPosition.z;
	
	// Adjust these away from the wonky coordinates we get from the Kinect,
	//  then normalize them so that they're in units of 100px per foot

	if (!fromKeyboard){
	//3.28 feet in a meter
		headX = headX * 3.28f * 100;
		headY = headY * 3.28f * 100 - ( ROOM_HEIGHT / 3 ); // Offset to bring up the vertical position of the eye
		headZ = headZ * 3.28f * 100;
	}
	
	// Make sure that the cameras are located somewhere in front of the screens
	//  Orientation is   X
	//                   |        ------> Screen 2 x axis
	//                   |   ------ Screen 2
	//                   |   |
	//                   |   |
	//         Z----<----|---o < Screen 1, o is the origin
	//                   |   | 
	//                   v   |

	//console() << "headX: " << headX << std::endl;
	//console() << "headZ: " << headZ << std::endl;

	if (headZ > ROOM_DEPTH / 2){
		mHeadCam0.setEye(Vec3f(headX, headY, headZ));
	}
	else{
		mHeadCam0.mEye = Vec3f(0,0,1200);
	}
	if (headX < -ROOM_WIDTH / 2){
		// headZ is negative here since that axis is backwards on Screen 2
		mHeadCam1.setEye(Vec3f(headX, headY, headZ));
	}
	else{
		mHeadCam1.mEye = Vec3f(-1200,0,0);
	}
}

void deltaTApp::mouseMove( MouseEvent event )
{
	mMousePos.x = event.getX() - getWindowWidth() * 0.5f;
	mMousePos.y = getWindowHeight() * 0.5f - event.getY();
}

ColorA colorFrom255(int r, int g, int b, int a){
	return ColorA(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

void deltaTApp::setup()
{
	// Set up our camera. This probably won't move this time...
	mCam0.setPerspective(60.0f, getWindowAspectRatio(), 1, 10000);
	Vec3f mEye = Vec3f(0,0,600);
	Vec3f mCenter = Vec3f(0,0, 0);//ROOM_DEPTH / 2 );
	Vec3f mUp = Vec3f::yAxis();
	mCam0.lookAt( mEye, mCenter, mUp );

	mCam1.setPerspective(60.0f, getWindowAspectRatio(), 1, 10000);
	mEye = Vec3f(-600,0,0);
	mCenter = Vec3f(0,0, 0);//ROOM_DEPTH / 2 );
	mUp = Vec3f::yAxis();
	mCam1.lookAt( mEye, mCenter, mUp );

	// Setup the camera for the main window
	mHeadCam0 = HeadCam( 1210.0f, getWindowAspectRatio() );
	mHeadCam0.mEye = Vec3f(-1200,0,1200);
	mHeadCam0.mEye.y = 0;
	mHeadCam0.mCenter = Vec3f(0,0, ROOM_DEPTH / 2 );

	mHeadCam1 = HeadCam( 1200.0f, getWindowAspectRatio() );
	mHeadCam1.mEye = Vec3f(-1210,0,0);
	mHeadCam1.mCenter = Vec3f(-ROOM_WIDTH / 2, 0, 0 );
	
	// Set up a listener for OSC messages
	oscListener.setup(7114);
	

	// Set up our parameters object
	mParams = params::InterfaceGl::create( "deltaT", Vec2i( 225, 200 ) );
	mParams->addParam( "Scene Rotation", &mSceneRotation );
	mParams->addParam( "Color", &mLineColor );
	mParams->addParam( "Position", &lightPosition);
	mLineColor = Color(.3f,.5f, .6f);
	mColorList.push_back(colorFrom255(0,181,164,255));
	mColorList.push_back(colorFrom255(0,255,231,255));
	mColorList.push_back(colorFrom255(222,255,252,255));
	mColorList.push_back(colorFrom255(253,68,25,255));
	mColorList.push_back(ColorA(1,.4f,1, 1));
	mColorList.push_back(ColorA(.4f,1,1, 1));

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


	Vec3f projectionEye = mHeadCam0.mEye;
	projectionEye.x = mHeadCam0.mCenter.x;
	projectionEye.y = mHeadCam0.mCenter.y;

	float zOffset = projectionEye.z - mHeadCam0.mCenter.z;
	// We have to adjust the camera to take into account that it
	//  doesn't distort enough past the edge of the screen
	float r = 0.0f; 
	float camXStorage = mHeadCam0.mEye.x;
	if (mHeadCam0.mEye.x < -300){
		r = (mHeadCam0.mEye.x + (ROOM_WIDTH / 2 )) / (mHeadCam0.mEye.z - (ROOM_DEPTH / 2));
		mHeadCam0.mEye.x += r * mHeadCam0.mEye.z;
	}

	Vec3f bottomLeft = Vec3f(-300, -200, -zOffset);
	Vec3f bottomRight = Vec3f(300, -200, -zOffset);
	Vec3f topLeft = Vec3f(-300, 200, -zOffset);

	mHeadCam0.update(projectionEye, bottomLeft, bottomRight, topLeft);
	// Restore our camera position
	mHeadCam0.mEye.x = camXStorage;

	console() << "cam0 position" << mHeadCam0.mEye << std::endl;
	console() << "projectionEye position" << projectionEye << std::endl;
	// Make sure to set it back, so updating doesn't make this fly away...
	// Now update Camera 1
	
	float xOffset = projectionEye.x - mHeadCam1.mCenter.x;
	
	// The values we pass into update for the bounds and the projectionEye need to 
	//  be coordinates relative to the camera, but mHeadCam1 is in global coordinates!
	bottomLeft = Vec3f(-300, -200, mHeadCam1.mEye.x + 300);//xOffset);
	bottomRight = Vec3f(300, -200, mHeadCam1.mEye.x + 300);//xOffset);
	topLeft = Vec3f(-300, 200, mHeadCam1.mEye.x + 300);//xOffset);
	
	projectionEye.y = mHeadCam1.mCenter.y;
	projectionEye.z = mHeadCam1.mCenter.z;

	Vec3f tempEye = mHeadCam1.mEye;
	// Again, I don't know why I've got to multiply this by 2
	mHeadCam1.mEye.x = tempEye.z;
	mHeadCam1.mEye.z = -tempEye.x;
		
	projectionEye = Vec3f(-mHeadCam1.mEye.z,0, 0);

	// Again, we have to adjust for the incorrect camera correction
	if (mHeadCam1.mEye.x > 300){
		r = (mHeadCam1.mEye.x - (ROOM_DEPTH / 2 )) / (mHeadCam1.mEye.z - (ROOM_WIDTH / 2));
		mHeadCam1.mEye.x += r * mHeadCam1.mEye.z;
	}
	
	console() << "ratio is : " << r << std::endl;
	
	mHeadCam1.update(projectionEye, bottomLeft, bottomRight, topLeft);
	
	console() << "cam1 position" << mHeadCam1.mEye << std::endl;
	console() << "projectionEye position" << projectionEye << std::endl;
	mHeadCam1.mEye.x = tempEye.x;
	mHeadCam1.mEye.z = tempEye.z;


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
	GLfloat light_position[] = { mMousePos.x, mMousePos.y, 75.0f, 0.0f };
	GLfloat light1_position[] = { lightPosition.x,lightPosition.y, lightPosition.z, 0.0f };

	GLfloat mat_specular[] = { red, green, blue, alpha };
	GLfloat mat_diffuse[] = { red, green, blue, alpha};
	GLfloat mat_shininess[] = { 50.0 };
	glShadeModel (GL_SMOOTH);

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glLightfv( GL_LIGHT0, GL_POSITION, light1_position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, mat_diffuse );
	glLightfv( GL_LIGHT1, GL_POSITION, light1_position);
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	//glEnable(GL_LIGHT1);
}

//Vec3f getTriangleNormal(Vec3f p0, Vec3f p1, Vec3f p2){

//}

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
			// If the normal is pointing away from the camera, flip it
			if(normal.z < 0){
				normal.invert();
			}
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
			// If the normal is pointing away from the camera, flip it
			if(normal.z < 0){
				normal.invert();
			}
			glNormal3f(normal);
		}
		gl::vertex(*j);

	}
	glEnd();
}


void deltaTApp::draw()
{
	Area mViewArea0 = Area(0, 0, getWindowSize().x / 2,getWindowSize().y);
	Area mViewArea1 = Area(getWindowSize().x / 2, 0, getWindowSize().x, getWindowSize().y);

	gl::clear( ColorA( 0.1f, 0.1f, 0.1f, 0.0f ), true );

	

	mActiveCam = mHeadCam1;
	mActivePerspCam = mCam0;
	drawGuts(mViewArea0);

	mActivePerspCam = mCam1;
	mActiveCam = mHeadCam0;
	drawGuts(mViewArea1);
}

void deltaTApp::drawGuts(Area area){

	

	gl::setMatricesWindow( getWindowSize(), false );
	//gl::setViewport( getWindowBounds() );
	gl::setViewport( area ); // This is what it will need to be set as
	// Update camera position data
	gl::setMatrices(mActivePerspCam);
	gl::rotate(mSceneRotation);
	

	// clear out the window with white
	gl::clear( Color( 1, 1, 1 ) ); 
	gl::enableAlphaBlending();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	for (unsigned int i = 0; i+1 < pointListArray.size(); i += 2){
		ColorA lightingColor = mColorList[i % mColorList.size()];
		setupGLLights(lightingColor);
		drawRibbon(pointListArray[i], pointListArray[i+1]);
	}

	mParams->draw();

}

CINDER_APP_NATIVE( deltaTApp, RendererGl )
