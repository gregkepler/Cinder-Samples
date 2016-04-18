#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TextParticlesApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	void keyDown( KeyEvent event )override;
	void update() override;
	void draw() override;
	
	void textToTexture();
	void lookAtTexture( const CameraPersp &cam, const ci::vec2 &size );
	
	double				mTimer, mTextTimer;
	gl::TextureRef		mTextTexture;
	CameraPersp			mCam;
	CameraUi			mCamUi;
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	string				mString;
	gl::FboRef			mTextFbo;
	
};

void TextParticlesApp::setup()
{
	// LOAD fonts
	
	// SET UP camera
	mCam  = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 1000.0  );
//	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
//	gl::setMatricesWindowPersp( getWindowWidth(), getWindowHeight() );
//	mCam.setEyePoint( vec3( 0, 0, 10.0 ) );
//	mCam.lookAt( vec3() );
	mCamUi = CameraUi( &mCam );
	
//	Font( loadAsset( "SourceSansPro-Bold.ttf" ), 16 );
	mFont = Font( loadAsset( "SourceSansPro-Bold.ttf" ), 48 );
	mTextureFont = gl::TextureFont::create( mFont );
	
	mTextFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), true );
}

void TextParticlesApp::textToTexture()
{

}

void TextParticlesApp::mouseDown( MouseEvent event )
{
	mCamUi.mouseDown( event );
}

void TextParticlesApp::mouseUp( MouseEvent event )
{
	mCamUi.mouseUp( event );
}

void TextParticlesApp::mouseMove( MouseEvent event )
{
//	mLastMousePos = event.getPos();
}

void TextParticlesApp::mouseWheel( MouseEvent event )
{
	mCamUi.mouseWheel( event );
}

void TextParticlesApp::mouseDrag( MouseEvent event )
{
	Rectf r	= Rectf( 0, 0, getWindowWidth(), getWindowHeight() );
	if ( r.contains( event.getPos() )) {
		mCamUi.mouseDrag( event );
	}
}


void TextParticlesApp::keyDown( KeyEvent event )
{
	// RESET a timer every time. Update in update loop. If timer reaches threshold of x seconds, explode
	
	switch( event.getCode() ){
		case KeyEvent::KEY_RETURN:
		// EXPLODE
		break;
		
		case KeyEvent::KEY_BACKSPACE:
		// REMOVE last character
		break;
		
		default:
		// ADD new character
		mString.append( string( 1, event.getChar() ) );
			CI_LOG_V( mString );
		break;
	}
}

void TextParticlesApp::update()
{
	// draw text to fbo
	
	// bind fbo
	
	gl::ScopedFramebuffer scpFbo( mTextFbo );
	gl::clear( ColorA( 0 , 0, 0, 0 ) );
	gl::ScopedViewport scpVp( ivec2(), mTextFbo->getSize() );
	gl::ScopedMatrices scpMtrx;

	lookAtTexture( mCam, mTextFbo->getSize() );
	
	vec2 stringSize = mTextureFont->measureString( mString );
	gl::ScopedMatrices scpMatrx;
	mTextureFont->drawString( mString, stringSize * vec2( -0.5, 0 ) );
	
	//

}

void TextParticlesApp::lookAtTexture( const CameraPersp &cam, const ci::vec2 &size  )
{
	auto ctx = gl::context();
	ctx->getModelMatrixStack().back() = mat4();
	ctx->getProjectionMatrixStack().back() = cam.getProjectionMatrix();
	ctx->getViewMatrixStack().back() = cam.getViewMatrix();
	ctx->getViewMatrixStack().back() *= glm::scale( vec3( 1, -1, 1 ) );									// invert Y axis so increasing Y goes down.
	ctx->getViewMatrixStack().back() *= glm::translate( vec3( size.x / 2, (float) - size.y / 2, 0 ) );	// shift origin up to upper-left corner.
}

void TextParticlesApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::drawSolidRect( Rectf( 10, 10, 100, 100 ) );
	gl::enableAlphaBlending();
	
	{
		gl::ScopedMatrices scpMtrx;
//		gl::setMatrices( mCam );
//		gl::setMatricesWindowPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 1000.0, true );
		
		// SET matrices so that by default, we are looking at a rect the size of the window
		lookAtTexture( mCam, getWindowSize() );
		gl::translate( ivec2(-getWindowCenter()) );
		
//		gl::setMatr( getWindowWidth(), getWindowHeight() );
		
//		gl::ScopedDepth scpDepth(	true );
	
		gl::ScopedColor scpColor( 1, 0, 0 );
		
		gl::drawSolidRect( Rectf( 10, 10, getWindowWidth() - 10, getWindowHeight() - 10 ) );
//		gl::translate( vec3( 0, 0, 10.0 ) );
//		gl::drawSolidRect( getWindowBounds() - getWindowCenter() );
//		Rectf boundsRect( 40, mTextureFont->getAscent() + 40, getWindowWidth() - 40, getWindowHeight() - 40 );
//		mTextureFont->drawString( "Hello", vec2() );
		/*
		{
			vec2 stringSize = mTextureFont->measureString( mString );
			gl::ScopedMatrices scpMatrx;
//			gl::translate( stringSize * vec2( -0.5 ) );
			mTextureFont->drawString( mString, stringSize * vec2( -0.5, 0 ) );
		}
		*/
		
		gl::color( Color::white() );
		gl::draw( mTextFbo->getColorTexture() );
		
//		mTextureFont->draw
//		gl::drawSolidRect( Rectf( 0, 0, 10, 10 ) );
//		gl::drawColorCube( vec3(), vec3( 10, 10, 10 ) );
	}
}

CINDER_APP( TextParticlesApp, RendererGl )
