#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "InstancedObjects.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class InstancingApp : public App {
  public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
	
	
  private:
	InstancedArrowsRef mArrows;
	InstancedDotsRef   mDots;
	bool			   mIsPaused = false;
	
	typedef enum { MODE_ARROWS, MODE_DOTS } Mode;
	Mode mMode;
};


void InstancingApp::setup()
{
	mArrows = InstancedArrows::create();
	mDots = InstancedDots::create();
	mMode = MODE_ARROWS;
}


void InstancingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	
		case KeyEvent::KEY_SPACE:
			mIsPaused = !mIsPaused;
			break;
		
		case KeyEvent::KEY_1:
			mMode = MODE_ARROWS;
			break;
		
		case KeyEvent::KEY_2:
			mMode = MODE_DOTS;
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
		double step  = mIsPaused ? 0.0 : timestep;
		
		switch( mMode ){
			case MODE_ARROWS:
				mArrows->update( step );
				break;
			
			case MODE_DOTS:
				mDots->update( step );
				break;
		}
	}
}


void InstancingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	switch( mMode ){
		case MODE_ARROWS:
			mArrows->draw();
			break;
		
		case MODE_DOTS:
			mDots->draw();
			break;
	}
}

CINDER_APP( InstancingApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
