#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "InstancedArrows.h"

using namespace ci;
using namespace ci::app;
using namespace std;


// ------------------------------------------------------------------------------------------------- Main app

class InstancingApp : public App {
  public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
	
	
  private:
	InstancedArrowsRef mArrows;
	bool			   mIsPaused = false;
};


void InstancingApp::setup()
{
	mArrows = InstancedArrows::create();
}


void InstancingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	
	case KeyEvent::KEY_SPACE:
		mIsPaused = !mIsPaused;
		break;
	}
}


void InstancingApp::update()
{
	// USE a fixed time step for a steady 60 updates per second.
	static const double timestep = 1.0 / 60.0;

	// KEEP TRACK of time.
	static double time = getElapsedSeconds();
	static double accumulator = 0.0;

	// CALCULATE elapsed time since last frame.
	double elapsed = getElapsedSeconds() - time;
	time += elapsed;

	
	accumulator += math<double>::min( elapsed, 0.1 ); // prevents 'spiral of death'
	while( accumulator >= timestep ) {
		accumulator -= timestep;
		mArrows->update( mIsPaused ? 0.0 : timestep );
	}
}


void InstancingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	mArrows->draw();
}

CINDER_APP( InstancingApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
