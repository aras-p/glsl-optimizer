/*
 * This program is under the GNU GPL.
 * Use at your own risk.
 *
 * written by David Bucciarelli (humanware@plus.it)
 *            Humanware s.r.l.
 */

#ifndef PARTICLES_H
#define PARTICLES_H

#include <GL/gl.h>

class particle {
 protected:
  float age;         // in seconds
  float acc[3];
  float vel[3];
  float pos[3];

 public:
  particle();
  virtual ~particle() {};

  virtual void beginDraw(void) {};
  virtual void draw(void)=0;
  virtual void endDraw(void) {};

  virtual void elapsedTime(float);
  virtual void checkAge(void) {};
};

class particleSystem {
 protected:
  particle **part;

  float t;

  unsigned long particleNum;
 public:
  particleSystem();
  ~particleSystem();

  void addParticle(particle *);

  void reset(void);

  void draw(void);

  void addTime(float);
};

class rainParticle : public particle {
 protected:
  static float min[3];
  static float max[3];
  static float partLength;

  float oldpos[3];

  void init(void);
 public:
  rainParticle();

  static void setRainingArea(float, float, float,
			     float, float, float);
  static void setLength(float);
  static float getLength(void) { return partLength; };

  void beginDraw(void) { glBegin(GL_LINES); };
  void draw(void);
  void endDraw(void) { glEnd(); };

  void elapsedTime(float);

  void checkAge(void);

  void randomHeight(void);
};

#endif
