//
//  SpiderWeb.h
//  SpiderWeb
//
//  Created by Greg Kepler on 12/18/15.
//
//

#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/CinderMath.h"
#include "glm/gtc/noise.hpp"
#include "SpiderWeb.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const float GUTTER = 10.0f;

// -----------------------------------------------------------------------------
//
// Web Ray
//
// -----------------------------------------------------------------------------

void WebRay::makePoints( ParticleRef webCenter, float avgLen, int pointCount )
{
	mWebCenter = webCenter;
	std::vector<ci::vec2> points;					// array of points along the ray line
	mRayPointAmt = pointCount;						// amount of points along the array
	
	// Find the length and angle of current and next line
	ci::vec2 diff = mStartPt->getPosition() - mEndPt->getPosition();
	mAngle = atan2(diff.y, diff.x);
	float len = distance( mStartPt->getPosition(), mEndPt->getPosition() );
	
	// Get the length of the strand. If the strand is longer than the average strand length, then use
	// that as the factor for getting the number and placement of points along the ray.
	// Multiplying by a fraction will keep the points away form the edges of the strands.
//	mStrandLength = (len < avgLen) ? len : avgLen;
	mStrandLength = len;
	float pointSpacing = ((( mStrandLength - 5.0 ) * 0.99 ) / mRayPointAmt );
	
	// find the positions of all the points that will go on the given ray
	for( int i = 0; i < mRayPointAmt-1; i++ ) {
		float someNoise = lmap( glm::perlin( vec2(mNoise, 0) ), -1.0f, 1.0f, 0.75f, 1.25f );
		vec2 pos = vec2( webCenter->getPosition().x + cos( mAngle ) * ( (i * pointSpacing) * someNoise ),
						 webCenter->getPosition().y + sin( mAngle ) * ( (i * pointSpacing) * someNoise ) );
		ParticleRef p = Particle::create( pos );
		mPoints.push_back( p );
		mNoise += 0.2;
	}
}


void WebRay::connectStrands( const std::vector<WebRayRef> &rays )
{
	float radialNoise = randFloat( 100 );
	vector<vec2> strands;
	vec2 webCenter = mWebCenter->getPosition();
	
	// add the initail points to the points array so that they at least connect if nothing else
	addRayPoint( mStartPt );
	addRayPoint( mEndPt );
	int rayCount = rays.size();
	bool stop = false;
	
	for( int i = 1; i < mRayPointAmt - 1; i++ ) {
		
		float floatI = float( i );
		
		// randomly DON'T draw a line
		int randomChance = randInt( 20 );
//		randomChance = 0;
//		randomChance = 4;
		if (randomChance == 0 || randomChance == 19 || randomChance == 18) { continue; }
		
		auto p = mPoints[i];
		WebRayRef nextStrand;
		// if not the last strand
		if( mOrder < rayCount - 1) {
			nextStrand = rays[ mOrder + 1 ];
		}
		else {
			nextStrand = rays[0];
			
			if( i == mRayPointAmt - 2 )
				stop = true;
		}
		
		float nextAngle = nextStrand->mAngle;
		vec2 diffFromCenter = (p->getPosition()) - (mWebCenter->getPosition());
		
		// Get a random distance to place the starting point at. it will be within a range above and
		// below the original point. The further away from the center, the more variation it can have
		float pointDist = getRandomPoint( diffFromCenter, floatI * 0.2, mStrandLength);	// random across
//		float pointDist = getRandomPoint( diffFromCenter, 1.0, mStrandLength);		// uniform across
		
		// make sure they are not connecting to points that don't exist
		pointDist = (pointDist > mStrandLength) ? mStrandLength : pointDist;
		
		
		ParticleRef thisPoint = Particle::create( vec2( webCenter.x + cos( mAngle ) * pointDist,
													   webCenter.y + sin( mAngle ) * pointDist ) );
		
		// next point distance
		int randomizeNext = randInt( 4 );
//		randomizeNext = 1;
		switch (randomizeNext)
		{
			case 0:
				// randomize within a range of the parallel strand point
				pointDist = distance( nextStrand->getPtByIndex( i )->getPosition(), ( mWebCenter->getPosition() ) ) + randFloat( floatI * -1.2, floatI * 1.2 );
				if( pointDist > nextStrand->mStrandLength )
					pointDist = nextStrand->mStrandLength;
				break;
			default:
				// connect directly to next strand point
				pointDist = distance( nextStrand->getPtByIndex( i )->getPosition(), ( mWebCenter->getPosition() ) );
				break;
		}

		
		// draw y shape between rays instead of straight line
		if (randomChance == 1)// || randomChance == 15 || randomChance == 16)
		{
			addYStrand( thisPoint, nextStrand->getPtByIndex( i ), nextAngle, nextStrand, pointDist);
		}
		else
		{
			if( stop )
				stop = stop;
			addStrand( thisPoint, nextAngle, nextStrand, pointDist);
		}
		
		
		// draw another line from the point
		if (randomChance == 2)
		{
			pointDist = getRandomPoint( diffFromCenter, floatI * 0.8, nextStrand->mStrandLength);
			addStrand( thisPoint, nextAngle, nextStrand, pointDist );
		}
		
		
		// draws a much longer line from the start point (which can be from either side.)
		else if (randomChance == 3)
		{
			float startAngle = (round(randFloat()) == 0) ? nextAngle : mAngle;
			
			if( startAngle == nextAngle )
			{
				auto startPoint = thisPoint;
				float strandLen = nextStrand->mStrandLength;
				float pointDist = getRandomPoint( diffFromCenter, floatI * randFloat() * 4 + 6, strandLen);
				addStrand( startPoint, startAngle, nextStrand, pointDist );
			}
			else
			{
				auto startPoint = thisPoint;
				float strandLen = nextStrand->mStrandLength;
				float pointDist = getRandomPoint( diffFromCenter, floatI * randFloat() * -4 - 6, strandLen);
				addStrand( startPoint, nextAngle, nextStrand, pointDist );
			}
		}
		radialNoise += 0.1;
	}
}


void WebRay::addYStrand( ParticleRef thisPoint, ParticleRef nextPoint, float nextAngle, WebRayRef nextStrand, float pointDist )
{
	// pointDist should be between this strand length and the next
	float nextPointDist = distance( nextPoint->getPosition(), ( mWebCenter->getPosition() ) );
	float thisPointDist = distance( thisPoint->getPosition(), ( mWebCenter->getPosition() ) );
	
	float randPart = randFloat( 0.2, 0.8 );		// percentage between 2 points
	auto partialPoint = Particle::create( thisPoint->getPosition() + ((nextPoint->getPosition() - thisPoint->getPosition()) * vec2(randPart, randPart)) );
//	addRayPoint( partialPoint );
	mAllPoints.push_back( partialPoint );
	
	// find extra points for "y" shape
	float pointDev = randFloat( 2.0, 8.0 );
	float startAngle, endAngle;
	ParticleRef startPt, endPt;
	float startDist, endDist;
	if( randBool() ) {
		startAngle = nextAngle;
		endAngle = mAngle;
		startPt = thisPoint;
		startDist = nextPointDist;
		endDist = thisPointDist;
	}else{
		startAngle = mAngle;
		endAngle = nextAngle;
		startPt = Particle::create( mWebCenter->getPosition() + vec2( cos(endAngle) * pointDist, sin(endAngle) * pointDist) );
		startDist = thisPointDist;
		endDist = nextPointDist;
	}
	
	ParticleRef nextPointA = Particle::create( mWebCenter->getPosition() + vec2( cos(startAngle) * (startDist - pointDev), sin(startAngle) * (startDist - pointDev) ) );
	ParticleRef nextPointB = Particle::create( mWebCenter->getPosition() + vec2( cos(startAngle) * (startDist + pointDev), sin(startAngle) * (startDist + pointDev) ) );
	
	if (startAngle == nextAngle)
	{
		addRayPoint(startPt);
		nextStrand->addRayPoint(nextPointA);
		nextStrand->addRayPoint(nextPointB);
	}
	else {
		nextStrand->addRayPoint(startPt);
		addRayPoint(nextPointA);
		addRayPoint(nextPointB);
	}
	
//	vec2 acrossPt = mWebCenter->getPosition() + vec2( cos(nextAngle) * nextPointDist, sin(nextAngle) * nextPointDist);
//	auto crossPoint = Particle::create( acrossPt );
	
	startPt->connectTo( partialPoint );
	nextPointA->connectTo( partialPoint );
	nextPointB->connectTo( partialPoint );
	
	partialPoint->connectTo( startPt );
	partialPoint->connectTo( nextPointA );
	partialPoint->connectTo( nextPointB );
};


void WebRay::addStrand( ParticleRef thisPoint, float nextAngle, WebRayRef nextStrand, float pointDist )
{
	vec2 pos = mWebCenter->getPosition() + vec2( cos( nextAngle ) * pointDist, sin( nextAngle ) * pointDist );
//	vec2 pos = mWebCenter->getPosition() + vec2( cos( nextAngle - 0.1 ) * pointDist, sin( nextAngle - 0.1 ) * pointDist );
	ParticleRef nextPoint = Particle::create( pos );
	float lineLen = distance( thisPoint->getPosition(), nextPoint->getPosition() );
	
	// draws a normal single connector line
	if( lineLen > 0.1 ) {
		addRayPoint( thisPoint );
		nextStrand->addRayPoint( nextPoint );
		thisPoint->connectTo( nextPoint );
		nextPoint->connectTo( thisPoint );
	}
};


void WebRay::addRayPoint( ParticleRef p )
{
	// check to make sure we don't add any dupe points
	for( int i = 0; i < mRayPoints.size(); i++ ){
		if( mRayPoints[i] == p ){
			return;
		}
	}
	mAllPoints.push_back( p );
	mRayPoints.push_back( p );
}


void WebRay::connectRay()
{
	vec2 webCenter = mWebCenter->getPosition();
	std::sort( mRayPoints.begin(), mRayPoints.end(), [webCenter](ParticleRef p1, ParticleRef p2) {
		float dist1 = distance( webCenter, p1->getPosition() );
		float dist2 = distance( webCenter, p2->getPosition() );
		return dist1 < dist2;
	});
	
	// go through the points and create a strand and define the spring
	for( int i = 1; i < mRayPoints.size(); i++ )
	{
		auto p1 = mRayPoints[i];
		auto p2 = mRayPoints[i-1];
		p1->connectTo( p2 );
		p2->connectTo( p1 );
	}
}



// -----------------------------------------------------------------------------
//
// SpiderWeb
//
// -----------------------------------------------------------------------------

SpiderWeb::SpiderWeb( const Options &options )
{
	mOptions = options;
}


void SpiderWeb::make()
{
	// find anchors
	addAchors();
	
	// find sub anchors
	addSubAnchors();
	
	// make initial ray points
	for( auto iter = mRays.begin(); iter != mRays.end(); ++iter ){
		(*iter)->makePoints( mWebCenter, mAvgLen, mOptions.getRayPointCount() );
	}
	
	// connect rays to itself and then to each other
	for( auto iter = mRays.begin(); iter != mRays.end(); ++iter ){
		(*iter)->connectStrands( mRays );
	}
	
	for( auto iter = mRays.begin(); iter != mRays.end(); ++iter ){
		(*iter)->connectRay();
	}
	
	
	// add all calculated points and strands to master array for the main app to grab
	for( auto iter = mRays.begin(); iter != mRays.end(); ++iter )
	{
		auto points = (*iter)->getAllPoints();
		for( auto innerIter = points.begin(); innerIter != points.end(); ++innerIter )
		{
			auto p = *innerIter;
			auto neighbors = p->getNeighbors();
			addParticle( p );
			
			
		}
	}
	
	for( auto iter = mPoints.begin(); iter != mPoints.end(); ++iter )
	{
		auto p = *iter;
		auto neighbors = p->getNeighbors();
		for( auto neighborIter = neighbors.begin(); neighborIter != neighbors.end(); ++neighborIter ){
			addStrand( p, *neighborIter );
		}
	}
}


void SpiderWeb::addAchors()
{
	int anchorCount = mOptions.getAnchorCount();
	float angleDiff = (M_PI * 2.0f) / anchorCount;
	float maxX = GUTTER;
	float minX = getWindowWidth() - GUTTER;
	float maxY = GUTTER;
	float minY = getWindowHeight() - GUTTER;
	float radiusSum = 0.0f;
	
	
	// generate anchors
	for( int i = 0; i < anchorCount; i++ )
	{
		float angle = ( i * angleDiff ) + randFloat( -0.5, 0.5 );
		float r = mOptions.getRadiusBase() * (randFloat() + 0.5);
		radiusSum += r;
		float pX = getWindowCenter().x + cos( angle ) * r;
		float pY = getWindowCenter().y + sin( angle ) * r;
		
		pX = (pX > maxX) ? pX : maxX;
		pX = (pX < minX) ? pX : minX;
		pY = (pY > maxY) ? pY : maxY;
		pY = (pY < minY) ? pY : minY;
		
		ParticleRef p = makeParticle( vec2( pX, pY ) );
		mAnchors.push_back( p );
		addParticle( p );
	}
	
	mAvgLen = radiusSum / float( anchorCount );
	mWebCenter = makeParticle( vec2( minX + (maxX - minX)/2, minY + (maxY - minY)/2 ) );
	addParticle( mWebCenter );
	
	// connect anchors to window edges
	for( auto iter = mAnchors.begin(); iter != mAnchors.end(); iter++ ) {		
		vec2 edgePoint = findEdgePoint( (*iter)->getPosition() );
		auto edge = Particle::create( edgePoint );
		addParticle( edge );
		(*iter)->connectTo( edge );
	}
}


ci::vec2 SpiderWeb::findEdgePoint( const ci::vec2 &origPos )
{
	vec2 intersection = vec2(NAN, NAN);
	vec2 centerPos = mWebCenter->getPosition();
	vec2 pos = centerPos + ( normalize( origPos - centerPos ) * vec2( 1000, 1000 ) ); // extend line out
	vec2 UL = vec2(getWindowBounds().getUL());
	vec2 UR = vec2(getWindowBounds().getUL()) + vec2( getWindowWidth(), 0 );
	vec2 LR = vec2(getWindowBounds().getLR());
	vec2 LL = vec2(getWindowBounds().getUL()) + vec2( 0, getWindowHeight() );
	int wall = 0;
	while( isnan( intersection.x ) && wall < 4 ){
		switch( wall ){
			case 0:
				intersection = intersect( centerPos, pos, UL, UR );
				break;
			case 1:
				intersection = intersect( centerPos, pos, UR, LR );
				break;
			case 2:
				intersection = intersect( centerPos, pos, LL, LR );
				break;
			case 3:
				intersection = intersect( centerPos, pos, UL, LL );
				break;
		};
		wall ++;
	}
	return intersection;
}


void SpiderWeb::addSubAnchors()
{
	float spacingNoise = randFloat( 10.0f );
	
	// between each anchor, place new points for rays to anchor to
	for( auto iter = mAnchors.begin(); iter != mAnchors.end(); ++iter ) {
	
		// get this and next anchor
		auto startAnchor	= *iter;
		auto endAnchor		= ( iter == mAnchors.end() - 1 ) ? *( mAnchors.begin() ) : *( iter + 1 );
		vec2 startPos		= startAnchor->getPosition();
		vec2 endPos			= endAnchor->getPosition();
		vec2 diff			= endPos - startPos;
		float dist			= length( diff );
		float angle			= atan2(diff.y, diff.x);
		int linePointAmt	= floor( dist / mOptions.getRaySpacing() );
		float rAngleFactor	= randFloat(0.05, 0.3);
		
		// make the curve more random so that it's not perfect
		float angleDif = M_PI * rAngleFactor;   // the random angle that will be sloped between points
		float ctrlLen = dist * rAngleFactor;	// the random distance of the control points
		
		// make the bezier curve points based on the calculated control factors
		vec2 curvePts[5];
		curvePts[0] = vec2( startPos.x, startPos.y );
		curvePts[1] = vec2( startPos.x + cos( angle + angleDif ) * ctrlLen, startPos.y + sin( angle + angleDif ) * ctrlLen );
		curvePts[2] = vec2( endPos.x   - cos( angle - angleDif ) * ctrlLen, endPos.y   - sin( angle - angleDif ) * ctrlLen );
		curvePts[3] = vec2( endPos.x, endPos.y);
		
		// Now draw the lines between bezier points
		vector<ParticleRef> bezPts;
		vector<ParticleRef> rayPoints;
		
		auto r = WebRay::create( mRays.size(), (*iter), mWebCenter, spacingNoise );
		mRays.push_back( r );
		
		for( int i = 1; i < linePointAmt; i++ )
		{
			float t = float(i) / linePointAmt;
			float pointX = bezierInterp( curvePts[0].x, curvePts[1].x, curvePts[2].x, curvePts[3].x, t );
			float pointY = bezierInterp( curvePts[0].y, curvePts[1].y, curvePts[2].y, curvePts[3].y, t );
			
			// create a new point
			auto bezP = makeParticle( vec2( pointX, pointY ) );
			bezPts.push_back( bezP );
			mSubAnchors.push_back( bezP );
			addParticle( bezP );
			
			// create rays (which contain particle vector)
			auto rayPoint = bezPts[i-1];
			auto r = WebRay::create( mRays.size(), rayPoint, mWebCenter, spacingNoise );
			mRays.push_back( r );
			
			if( i>0 ){
				rayPoints.push_back( bezP );
			}
		}
		
		// go through the points and connect
		for( int i = 0; i < linePointAmt-1; i++ )
		{
			// Finds the 1st point. Either the startPt or the first of the curve
			auto p = (i == 0) ? startAnchor : bezPts[i-1];
			auto nextP = (i == 0) ? bezPts[0] : bezPts[i];
			p->connectTo(nextP);
		}
		
		if( linePointAmt == 1 )
			startAnchor->connectTo( endAnchor );
		else if( linePointAmt > 1 )
			bezPts[linePointAmt-2]->connectTo( endAnchor );
	}
}


ParticleRef SpiderWeb::makeParticle( const ci::vec2 &pos )
{
	ParticleRef p = Particle::create( pos );
	
//	p.setWeight(particleWeight);
//	physics.addParticle(p);
	return p;
	
	// just add it to the vector of particle positions
}

void SpiderWeb::addParticle( ParticleRef particle )
{
	for( auto iter = mPoints.begin(); iter != mPoints.end(); ++iter )
	{
		if( *iter == particle ){
			return;
		}
	}
	particle->setId( mPoints.size() );
	mPoints.push_back( particle );
}

void SpiderWeb::addStrand( ParticleRef particle1, ParticleRef particle2 )
{
	auto pair = std::pair<ParticleRef, ParticleRef>( particle1, particle2 );
	
	bool found = false;
	int outer = 0;
	for( auto strandIter = mStrands.begin(); strandIter != mStrands.end(); ++strandIter ){
		int whichFound = 0;
		if( strandIter->first == particle1 ){
			whichFound = 1;
		}
		else if( strandIter->second == particle1 ){
			whichFound = 2;
		}
		if( whichFound ){
			int inner = 0;
			for( auto innerIter = strandIter + 1; innerIter != mStrands.end(); ++innerIter ){
				if( whichFound == 1 && innerIter->second == particle2 ){
					found = true;
				}
				else if(whichFound == 2 &&	innerIter->first == particle2 ){
					found = true;
				}
				
				inner++;
				if( found == true ){
					break;
				}
				
			}
			
			if( found == true ){
				break;
			}
		}
		outer++;
	}
	
	mStrands.push_back( pair );
	
	if( !found ){
		mUniqueStrands.push_back( pair );
	}
}

void SpiderWeb::update()
{
	
}

void SpiderWeb::draw() {
//	gl::enableAlphaBlending();
//	gl::color( 1.0, 1.0, 1.0, 0.8 );
	for( auto iter = mPoints.begin(); iter != mPoints.end(); ++iter ){
		(*iter)->draw();
	}
	for( auto iter = mRays.begin(); iter != mRays.end(); ++iter ){
		(*iter)->draw();
	}
}

void SpiderWeb::reset()
{
	mStrands.clear();
	mUniqueStrands.clear();
	mPoints.clear();
	mAnchors.clear();
	mSubAnchors.clear();
	mRays.clear();
	mWebCenter = nullptr;
	
	// clear points
	/*
		std::vector<std::pair<ParticleRef, ParticleRef>>	mStrands, mUniqueStrands;
	std::vector<ParticleRef>			mPoints;
	std::vector<ParticleRef>			mAnchors;
	std::vector<ParticleRef>			mSubAnchors;
	ParticleRef							mWebCenter;
	float								mAvgLen;
	std::vector<WebRayRef>				mRays;
	Options								mOptions;
	*/
}



