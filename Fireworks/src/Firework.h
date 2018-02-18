//
//  Firework.h
//  Fireworks
//
//  Created by Greg Kepler on 7/3/16.
//
//


typedef std::shared_ptr<class Firework> FireworkRef;
class Firework {
  public:
	static FireworkRef create() { return std::make_shared<Firework>(); }
	
	void update( double elapsed );
	
	Firework(){};
};