

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

#define APP_WIDTH		640 //2560
#define APP_HEIGHT		480 //720

#define ROOM_WIDTH 600
#define ROOM_HEIGHT 400
#define ROOM_DEPTH 600

#define FRAMES_PER_CULL 30

#define MAX_LINE_SIZE 300

// This changes how deep or shallow an offset to put the kinect data
//  Play with this if the ribbons are sticking to the front or back
#define KINECT_DEPTH_OFFSET 1500


class deltaTApp : public AppNative {
  public:
	virtual void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseMove( MouseEvent event );
	void keyDown(KeyEvent event);
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
	Vec3f mLightPosition;

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
	settings->setBorderless();
	settings->setWindowPos(0,0);
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
	mCam0.setPerspective(90.0f, getWindowAspectRatio(), 1, 10000);
	Vec3f mEye = Vec3f(0,0,300);
	Vec3f mCenter = Vec3f(0,0, 0);//ROOM_DEPTH / 2 );
	Vec3f mUp = Vec3f::yAxis();
	mCam0.lookAt( mEye, mCenter, mUp );

	mCam1.setPerspective(90.0f, getWindowAspectRatio(), 1, 10000);
	mEye = Vec3f(-300,0,0);
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
	mParams->addParam( "Position", &mLightPosition);
	mLightPosition = Vec3f(300,300,600);

	mLineColor = Color(.3f,.5f, .6f);
	mColorList.push_back(colorFrom255(0,181,164,200)); // Bluish
	mColorList.push_back(colorFrom255(170,62,78,200)); // Redish
	mColorList.push_back(colorFrom255(248,222,77,200)); // Yellowish
	mColorList.push_back(colorFrom255(254,173,82,200)); // Orangeish
	mColorList.push_back(colorFrom255(248,222,77,200)); // Yellowish
	mColorList.push_back(colorFrom255(200,51,73,200)); // Redish
	mColorList.push_back(colorFrom255(0,205,181,200)); // Blueish
	mColorList.push_back(colorFrom255(254,173,82,200)); // Orangeish

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

		setCameras(Vec3f(headX, headY, headZ), false);
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
	

		// Front kinects - only use data from these
		if (handZ > 400 && handX < 300 && handX > -300){
			handZ -= KINECT_DEPTH_OFFSET;
			elbowZ -= KINECT_DEPTH_OFFSET;
			// Make sure that hands stay in the box-ish
			handX = constrain(handX, -290, 290);
			handX = constrain(handX, -350, 290);
			handZ = constrain(handZ, -290, 290);
			elbowX = constrain(elbowX, -290, 290);
			elbowX = constrain(elbowX, -350, 290);
			elbowZ = constrain(elbowZ, -290, 290);		
		}
		// Don't use the data from the side kinects
		
		// side kinects
		else if (handX < -400 && handZ > -300 && handZ < 300){
			// Force our list to get big, which forces this hand to be someone else
			ID += 10;
			handX += KINECT_DEPTH_OFFSET;
			elbowX += KINECT_DEPTH_OFFSET;
			//handX *= 1.1f;
			//elbowX *= 1.1f;
			// Make sure that hands stay in the box-ish
			handX = constrain(handX, -290, 290);
			handZ = constrain(handZ, -290, 350);
			elbowX = constrain(elbowX, -290, 290);
			elbowZ = constrain(elbowZ, -290, 350);
		}

		else
			return;

		// Now that we've adjusted out data, let's put it into the arrays
		Vec3f handPosition = Vec3f(handX, handY, handZ);
		Vec3f elbowPosition = Vec3f(elbowX, elbowY, elbowZ);
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

void deltaTApp::keyDown( KeyEvent event )
{
	
	
	switch( event.getCode() ){
		//case KeyEvent::KEY_UP:		mMouseRightPos = Vec2f( 222.0f, 205.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_UP:		setCameras(Vec3f(mHeadCam0.mEye.x, mHeadCam0.mEye.y, mHeadCam0.mEye.z - 100), true);
									break;
		//case KeyEvent::KEY_LEFT:	mMouseRightPos = Vec2f(-128.0f,-178.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_LEFT:	setCameras(Vec3f(mHeadCam0.mEye.x - 100, mHeadCam0.mEye.y, mHeadCam0.mEye.z), true);
									break;
			//case KeyEvent::KEY_RIGHT:	mMouseRightPos = Vec2f(-256.0f, 122.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_RIGHT:	setCameras(Vec3f(mHeadCam0.mEye.x + 100, mHeadCam0.mEye.y, mHeadCam0.mEye.z), true);	break;
		//case KeyEvent::KEY_DOWN:	mMouseRightPos = Vec2f(   0.0f,   0.0f ) + getWindowCenter();	break;
		case KeyEvent::KEY_DOWN:	setCameras(Vec3f(mHeadCam0.mEye.x, mHeadCam0.mEye.y, mHeadCam0.mEye.z + 100), true);
									break;
		default: break;
	}
}

void deltaTApp::update()
{
	Vec3f topLeft = Vec3f(-ROOM_WIDTH/2, ROOM_HEIGHT/2, ROOM_DEPTH/2);
	Vec3f bottomLeft = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);
	Vec3f bottomRight = Vec3f(ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);

	mHeadCam0.update(topLeft, bottomLeft, bottomRight, 10000);

	console() << "cam0 position" << mHeadCam0.mEye << std::endl;

	topLeft = Vec3f(-ROOM_WIDTH/2, ROOM_HEIGHT/2, -ROOM_DEPTH/2);
    bottomLeft = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, -ROOM_DEPTH/2);
	bottomRight = Vec3f(-ROOM_WIDTH/2, -ROOM_HEIGHT/2, ROOM_DEPTH/2);
	
	mHeadCam1.update(topLeft, bottomLeft, bottomRight, 10000);
	
	console() << "cam1 position" << mHeadCam1.mEye << std::endl;


	// Update line position data by culling the old data
	for (unsigned int i = 0; i < pointListArray.size(); i++){
		while (pointListArray[i].size() > MAX_LINE_SIZE){
			pointListArray[i].pop_front();
		}
	}

	frameCounter++;
	// Also cull old data when enough frames pass
	if( frameCounter % FRAMES_PER_CULL == 0){
		console() << "culling..." << endl;
		for (unsigned int i = 0; i < pointListArray.size(); i++){
			if (pointListArray[i].size() > 0){
				pointListArray[i].pop_front();
			}
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
	GLfloat light1_position[] = { mLightPosition.x,mLightPosition.y, mLightPosition.z, 0.0f };

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
	Area mViewAreaLeft = Area(0, 0, getWindowSize().x / 2,getWindowSize().y);
	Area mViewAreaRight = Area(getWindowSize().x / 2, 0, getWindowSize().x, getWindowSize().y);

	gl::clear( ColorA( 1, 1, 1, 0.0f ), true );

	mActiveCam = mHeadCam1;
	mActivePerspCam = mCam1;
	drawGuts(mViewAreaLeft);

	mActivePerspCam = mCam0;
	mActiveCam = mHeadCam0;
	drawGuts(mViewAreaRight);
}

void deltaTApp::drawGuts(Area area){

	

	//gl::setMatricesWindow( getWindowSize(), false );
	//gl::setViewport( getWindowBounds() );
	gl::setViewport( area ); // This is what it will need to be set as
	// Update camera position data
	//gl::setMatrices(mActivePerspCam);
	//gl::rotate(mSceneRotation);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(mActiveCam.mProjectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mActiveCam.mCam.getModelViewMatrix());

	// clear out the window with white
	//gl::clear( Color( 1, 1, 1 ) ); 
	gl::enableAlphaBlending();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	for (unsigned int i = 0; i+1 < pointListArray.size(); i += 2){
		ColorA lightingColor = mColorList[(i/2) % mColorList.size()];
		setupGLLights(lightingColor);
		drawRibbon(pointListArray[i], pointListArray[i+1]);
	}

	// Let's add some fog!
	GLfloat density = 0.3;

	GLfloat fogColor[4] = {0.5, 0.5, 0.5, 1.0};
/*	glEnable (GL_FOG); //enable the fog
	glFogi (GL_FOG_MODE, GL_EXP2); //set the fog mode to GL_EXP2
	glFogfv (GL_FOG_COLOR, fogColor); //set the fog color to our color chosen above
	glFogf (GL_FOG_DENSITY, density); //set the density to the value above
	glHint (GL_FOG_HINT, GL_NICEST); // set the fog to look the nicest, may slow down on older cards
*/	//mParams->draw();
	//gl::drawStrokedCube(Vec3f(0,0,0),Vec3f(ROOM_WIDTH, ROOM_HEIGHT , ROOM_DEPTH ));
}

CINDER_APP_NATIVE( deltaTApp, RendererGl )
