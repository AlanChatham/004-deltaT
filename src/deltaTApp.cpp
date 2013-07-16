

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include <queue>
#include <vector>
#include <list>


using namespace ci;
using namespace ci::app;
using namespace std;



#define ROOM_WIDTH 300
#define ROOM_HEIGHT 200
#define ROOM_DEPTH 300




class deltaTApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseMove( MouseEvent event );
	void update();
	void draw();
	
	
	CameraPersp mCam;
	params::InterfaceGl mParams;
	vector<Vec3f> pointList;
	Quatf mSceneRotation;
	Color mLineColor;
	ci::gl::Light *_light;	
	ci::gl::Material	*_ribbonMaterial;

	Vec2f			mMousePos;
};

void deltaTApp::mouseMove( MouseEvent event )
{
	mMousePos.x = event.getX() - getWindowWidth() * 0.5f;
	mMousePos.y = getWindowHeight() * 0.5f - event.getY();
}

void deltaTApp::setup()
{
	// Set up our camera. This probably won't move this time...
	mCam.setPerspective(60.0f, getWindowAspectRatio(), 1, 10000);
	Vec3f mEye = Vec3f(0,0,600);
	Vec3f mCenter = Vec3f(0,0, 0);//ROOM_DEPTH / 2 );
	Vec3f mUp = Vec3f::yAxis();
	mCam.lookAt( mEye, mCenter, mUp );
	

	// Set up our parameters object
	mParams = params::InterfaceGl( "Flocking", Vec2i( 225, 200 ) );
	mParams.addParam( "Scene Rotation", &mSceneRotation );
	mParams.addParam( "Color", &mLineColor );
	mLineColor = Color(.3f,.5f, .6f);

	_light = new gl::Light( gl::Light::POINT, 0 );
	//mLight = new gl::Light( gl::Light::DIRECTIONAL, 0 );
	_light->lookAt( ci::Vec3f::one() * 20, Vec3f( 0, 0, 0 ) );
	_light->setAmbient( Color( 1.0f, 1.0f, 1.0f ) );
	_light->setDiffuse( Color( 1.0f, 1.0f, 1.0f ) );
	_light->setSpecular( Color( 1.0f, 1.0f, 1.0f ) );
	_light->setShadowParams( 40.0f, 1.0f, 30.0f );
	_light->enable();


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
	
}

void deltaTApp::mouseDown( MouseEvent event )
{
}

void deltaTApp::update()
{
	// Update camera position data
	gl::setMatrices(mCam);
	gl::rotate(mSceneRotation);

	// Update line position data
}

void deltaTApp::draw()
{
	// clear out the window with white
	gl::clear( Color( 1, 1, 1 ) ); 
	gl::enableAlphaBlending();

	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	GLfloat light_position[] = { mMousePos.x, mMousePos.y, 75.0f, 0.0f };
	glLightfv( GL_LIGHT0, GL_POSITION, light_position );

	glFrontFace(GL_CW);
	glBegin(GL_TRIANGLE_STRIP);

	// Draw a line between all the points in our queue
	for (int i = 0; i < pointList.size(); i++){
		gl::vertex(pointList[i]);
		gl::vertex(pointList[i] + Vec3f(-30,-30,-30));
		
		// If we're not at the end or the beginning, we can set the normal easily
		if(i > 0 && i < pointList.size() - 1){
			Vec3f current = pointList[i];
			Vec3f last = pointList[i-1];
			Vec3f next = pointList[i+1];
			Vec3f u = current - last;
			Vec3f v = next - last;
			Vec3f normal = Vec3f::zero();
			normal.x = (u.y * v.z) - (u.z * v.y);
			normal.y = (u.z * v.x) - (u.x * v.z);
			normal.z = (u.x * v.y) - (u.y * v.x);
			normal.normalize();
			glNormal3f(normal);
		}

		//gl::drawLine(pointList[i], pointList[i+1]);
		//gl::drawCylinder(20,20,50);
	}

	glEnd();

	gl::drawSphere(Vec3f::zero(),20,20);
	mParams.draw();
}

CINDER_APP_NATIVE( deltaTApp, RendererGl )
