#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Arcball.h"
#include "cinder/Sphere.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GeometryAndLightsApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
	
	gl::VboMeshRef	mVboMesh, mSphereMesh;
	gl::GlslProgRef	mGlsl, mWireframeGlsl;
	gl::BatchRef	mWireBatch, mSphereBatch;
	CameraPersp		mCam;
	CameraUi		mCamUi;
	Sphere			mBoundingSphere;
	
	vec2			mPtLightRot1;
	vec3			mPtLight1;
	Color			mPtLightCol1;
	
	
};

void prepareSettings( App::Settings* settings )
{
	settings->setWindowSize( 1024, 768 );
	settings->setHighDensityDisplayEnabled();
	settings->setMultiTouchEnabled( false );
}

vec3 getRotation( vec2 rot, float rad ){
	vec3 pos;
	pos.x = rad * sin( rot.x ) * cos( rot.y );
	pos.y = rad * sin( rot.x ) * sin( rot.y );
	pos.z = rad * cos( rot.x );
	return pos;
}

void GeometryAndLightsApp::setup()
{
	// geometry mesh
	auto s = geom::Icosphere().subdivisions( 2 );
	TriMesh::Format fmt = TriMesh::Format().positions().normals().texCoords();
	TriMesh sphereMesh( s, fmt );
	
	mPtLightRot1 = vec2( 0, 0 );
	mPtLight1 = getRotation( mPtLightRot1, 1.25 );
	mPtLightCol1 = Color(1, 1, 0);

//	Color ambient = Color8u(25, 41, 43	);
	Color ambient = Color8u(16, 32, 30 );
//	Color lightColor = Color8u( 108, 191, 163);
	Color lightColor = Color8u( 35, 161, 176);
	
	mGlsl = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
	mGlsl->uniform( "uLightDirection", vec3( 0.5, 1.0, 0.75 ) );
	mGlsl->uniform( "uLightColor", lightColor );
	mGlsl->uniform( "uAmbient", ambient );
	
	auto format = gl::GlslProg::Format()
			.vertex( loadAsset( "wireframe.vert" ) )
			.geometry( loadAsset( "wireframe.geom" ) )
			.fragment( loadAsset( "wireframe.frag" ) );
	mWireframeGlsl = gl::GlslProg::create( format );
	mWireframeGlsl->uniform( "uBrightness", 1.0f );
	mWireframeGlsl->uniform( "uViewportSize", vec2( getWindowSize() ) );
	mWireframeGlsl->uniform( "uLightDirection", vec3( 0.0, 1.0, 0.0 ) );
	mWireframeGlsl->uniform( "uLightColor", lightColor );
	mWireframeGlsl->uniform( "uAmbient", ambient );
	mWireframeGlsl->uniform( "uPointLight1", mPtLight1 );
	mWireframeGlsl->uniform( "uPointLight1Color", mPtLightCol1 );
	
	mWireBatch = gl::Batch::create( sphereMesh, mWireframeGlsl );
	mSphereBatch = gl::Batch::create( sphereMesh, mGlsl );
	
	mCam.setPerspective( 60.0f, getWindowAspectRatio(), 0.1, 10000 );
	mCamUi = CameraUi( &mCam );
	
	TriMeshRef m = TriMesh::create( geom::Sphere().radius( 1.25 ) );
	mBoundingSphere = Sphere::calculateBoundingSphere( m->getPositions<3>(), m->getNumVertices() );
	mCam = mCam.calcFraming( mBoundingSphere );
}


void GeometryAndLightsApp::mouseDown( MouseEvent event )
{
	mCamUi.mouseDown( event );
}

void GeometryAndLightsApp::mouseUp( MouseEvent event )
{
	mCamUi.mouseUp( event );
}

void GeometryAndLightsApp::mouseDrag( MouseEvent event )
{
	mCamUi.mouseDrag( event );
}

void GeometryAndLightsApp::mouseWheel( MouseEvent event )
{
	mCamUi.mouseWheel( event );
}

void GeometryAndLightsApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
		case KeyEvent::KEY_1:
			mCam = mCam.calcFraming( mBoundingSphere );
			break;
		
		case KeyEvent::KEY_2:
			mCam.lookAt( vec3( 0, 0, 1.0 ), vec3(0, 0, 0.0) );
			break;
	}
}

void GeometryAndLightsApp::update()
{
	mWireframeGlsl->uniform( "uEyeDirection", normalize(mCam.getEyePoint()) );
	
	mPtLightRot1.x += 0.005;
	mPtLightRot1.y += 0.006;
	mPtLight1 = getRotation( mPtLightRot1, 1.25 );
	mWireframeGlsl->uniform( "uPointLight1", mPtLight1 );
	
	mPtLightCol1 = Color(	cos( getElapsedSeconds() ) * 0.5 + 0.5,
							sin( getElapsedSeconds() * 0.6 ) * 0.5 + 0.5,
							1.0 - (cos( getElapsedSeconds() ) * 0.5 + 0.5));
	mWireframeGlsl->uniform( "uPointLight1Color", mPtLightCol1 );
}

void GeometryAndLightsApp::draw()
{
	gl::clear();
	
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	gl::setMatrices( mCam );
	gl::ScopedColor colorScope( Color( 1, 1, 1 ) );

	{
		// Draw point light
		gl::ScopedColor scpColor( mPtLightCol1 );
		gl::drawSphere( mPtLight1, 0.05 );
	}
	
	{
		// We're using alpha blending, so render the back side first.
		gl::ScopedBlendAlpha blendScope;
		gl::ScopedFaceCulling cullScope( true, GL_FRONT );
		gl::ScopedModelMatrix mtrx;

		mWireframeGlsl->uniform( "uBrightness", 0.5f );
		mWireBatch->draw();

		// Now render the front side.
		gl::cullFace( GL_BACK );

		mWireframeGlsl->uniform( "uBrightness", 1.0f );
		mWireBatch->draw();
		
	}
}

CINDER_APP( GeometryAndLightsApp, RendererGl( RendererGl::Options().msaa( 16 ) ), prepareSettings )
