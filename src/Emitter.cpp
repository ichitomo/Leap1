#include "Emitter.h"

#include "cinder/gl/gl.h"

using namespace ci;
using std::list;

// These are defined in HodginParticlesApp.cpp
extern void renderImage( Vec3f _loc, float _diam, Color _col, float _alpha );
extern float floorLevel;
extern bool ALLOWTRAILS, ALLOWFLOOR;
extern gl::TextureRef particleImg, emitterImg;

Emitter::Emitter()
{
	myColor = Color( 1, 1, 1 );
	loc = Vec3f::zero();
	vel = Vec3f::zero();
}

void Emitter::exist( Vec2i fingerLoc )
{
	setVelToFinger( fingerLoc );
	findVelocity();
	setPosition();
	iterateListExist();
	render();

	gl::disable( GL_TEXTURE_2D );
	if( ALLOWTRAILS )
		iterateListRenderTrails();
}

void Emitter::setVelToFinger( Vec2i fingerLoc )
{
	velToFinger.set( fingerLoc.x - loc.x, fingerLoc.y - loc.y, 0 );
}

void Emitter::findVelocity()
{
	vel += ( velToFinger - vel ) * 0.25f;
}

void Emitter::setPosition()
{
	loc += vel;

	if( ALLOWFLOOR ) {
		if( loc.y > floorLevel ) {
			loc.y = floorLevel;
			vel.y = 0;
		}
	}
}

void Emitter::iterateListExist()
{
	gl::enable( GL_TEXTURE_2D );
	particleImg->bind();

	for( list<Particle>::iterator it = particles.begin(); it != particles.end(); ) {
		if( ! it->ISDEAD ) {
			it->exist();
			++it;
		}
		else {
			it = particles.erase( it );
		}
	}
}

void Emitter::render()
{
	emitterImg->bind();
	renderImage( loc, 150, myColor, 1.0 );
}

void Emitter::iterateListRenderTrails()
{
	for( list<Particle>::iterator it = particles.begin(); it != particles.end(); ++it ) {
		it->renderTrails();
	}
}

void Emitter::addParticles( int _amt )
{
	for( int i = 0; i < _amt; i++ ) {
		particles.push_back( Particle( loc, vel ) );
	}
}