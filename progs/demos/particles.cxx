/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (humanware@plus.it)
 *            Humanware s.r.l.
 */

#include <stdlib.h>

#include "particles.h"

#define vinit(a,i,j,k) {\
  (a)[0]=i;\
  (a)[1]=j;\
  (a)[2]=k;\
}

#define vadds(a,dt,b) {\
  (a)[0]+=(dt)*(b)[0];\
  (a)[1]+=(dt)*(b)[1];\
  (a)[2]+=(dt)*(b)[2];\
}

#define vequ(a,b) {\
  (a)[0]=(b)[0];\
  (a)[1]=(b)[1];\
  (a)[2]=(b)[2];\
}

#define vinter(a,dt,b,c) {\
  (a)[0]=(dt)*(b)[0]+(1.0-dt)*(c)[0];\
  (a)[1]=(dt)*(b)[1]+(1.0-dt)*(c)[1];\
  (a)[2]=(dt)*(b)[2]+(1.0-dt)*(c)[2];\
}

#define clamp(a)        ((a) < 0.0 ? 0.0 : ((a) < 1.0 ? (a) : 1.0))

#define vclamp(v) {\
  (v)[0]=clamp((v)[0]);\
  (v)[1]=clamp((v)[1]);\
  (v)[2]=clamp((v)[2]);\
}


float rainParticle::min[3];
float rainParticle::max[3];
float rainParticle::partLength=0.2f;


static float vrnd(void)
{
  return(((float)rand())/RAND_MAX);
}


particle::particle()
{
  age=0.0f;

  vinit(acc,0.0f,0.0f,0.0f);
  vinit(vel,0.0f,0.0f,0.0f);
  vinit(pos,0.0f,0.0f,0.0f);
}

void particle::elapsedTime(float dt)
{
  age+=dt;

  vadds(vel,dt,acc);

  vadds(pos,dt,vel);
}

/////////////////////////////////////////
// Particle System
/////////////////////////////////////////

particleSystem::particleSystem()
{
  t=0.0f;

  part=NULL;

  particleNum=0;
}

particleSystem::~particleSystem()
{
  if(part)
    free(part);
}

void particleSystem::addParticle(particle *p)
{
  if(!part) {
    part=(particle **)calloc(1,sizeof(particle *));
    part[0]=p;
    particleNum=1;
  } else {
    particleNum++;
    part=(particle **)realloc(part,sizeof(particle *)*particleNum);
    part[particleNum-1]=p;
  }
}

void particleSystem::reset(void)
{
  if(part)
    free(part);

  t=0.0f;

  part=NULL;

  particleNum=0;
}

void particleSystem::draw(void)
{
  if(!part)
    return;

  part[0]->beginDraw();
  for(unsigned int i=0;i<particleNum;i++)
    part[i]->draw();
  part[0]->endDraw();
}

void particleSystem::addTime(float dt)
{
  if(!part)
    return;

  for(unsigned int i=0;i<particleNum;i++) {
    part[i]->elapsedTime(dt);
    part[i]->checkAge();
  }
}

/////////////////////////////////////////
// Rain
/////////////////////////////////////////

void rainParticle::init(void)
{
  age=0.0f;

  acc[0]=0.0f;
  acc[1]=-0.98f;
  acc[2]=0.0f;

  vel[0]=0.0f;
  vel[1]=0.0f;
  vel[2]=0.0f;

  oldpos[0]=pos[0]=min[0]+(max[0]-min[0])*vrnd();
  oldpos[1]=pos[1]=max[1]+0.2f*max[1]*vrnd();
  oldpos[2]=pos[2]=min[2]+(max[2]-min[2])*vrnd();

  vadds(oldpos,-partLength,vel);
}

rainParticle::rainParticle()
{ 
  init();
}

void rainParticle::setRainingArea(float minx, float miny, float minz,
				  float maxx, float maxy, float maxz)
{
  vinit(min,minx,miny,minz);
  vinit(max,maxx,maxy,maxz);
}

void rainParticle::setLength(float l)
{
  partLength=l;
}

void rainParticle::draw(void)
{
  glColor4f(0.7f,0.95f,1.0f,0.0f);
  glVertex3fv(oldpos);

  glColor4f(0.3f,0.7f,1.0f,1.0f);
  glVertex3fv(pos);
}

void rainParticle::checkAge(void)
{
  if(pos[1]<min[1])
    init();
}

void rainParticle::elapsedTime(float dt)
{
  particle::elapsedTime(dt);

  if(pos[0]<min[0])
    pos[0]=max[0]-(min[0]-pos[0]);
  if(pos[2]<min[2])
    pos[2]=max[2]-(min[2]-pos[2]);

  if(pos[0]>max[0])
    pos[0]=min[0]+(pos[0]-max[0]);
  if(pos[2]>max[2])
    pos[2]=min[2]+(pos[2]-max[2]);

  vequ(oldpos,pos);
  vadds(oldpos,-partLength,vel);
}

void rainParticle::randomHeight(void)
{
  pos[1]=(max[1]-min[1])*vrnd()+min[1];

  oldpos[1]=pos[1]-partLength*vel[1];
}
