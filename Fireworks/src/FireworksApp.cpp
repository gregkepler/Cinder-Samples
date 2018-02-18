#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Firework.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// http://www.theverge.com/2013/7/4/4489980/how-do-fireworks-work


class FireworksApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	
	void launchFirework();
	
	enum FireColor {
		STEEL,		// yellow
		CHARCOAL,	// orange
		STRONTIUM,	// red
		BARIUM,		// green
		COPPER		// blue
	};
	
	std::vector<FireworkRef> mFireworks;
	
	bool mIsPaused = false;
	
	
	// Mortar - explosion 1
	// - fuse
	// - separate package
	
	// Internal - explosion 2 - for each "star"
	
};

void FireworksApp::setup()
{
}

void FireworksApp::mouseDown( MouseEvent event )
{
	launchFirework();
}

void FireworksApp::launchFirework()
{
	// add a new firework to the vector
	// - type
	// - color array
	// - position
	// - launch power  - how much kinetic energy / gunpowder
	// -
	
	auto fw = Firework::create();
	mFireworks.push_back( std::move( fw ) );
}

void FireworksApp::update()
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
		
		for( auto iter = mFireworks.begin(); iter != mFireworks.end(); ++iter ){
			(*iter)->update( step );
		}
	}
}

void FireworksApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::drawSolidRect( Rectf(0 , 0, 100, 100 ) );
}

CINDER_APP( FireworksApp, RendererGl )
