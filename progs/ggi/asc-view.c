/*
    test program for the ggi-mesa driver

    Copyright (C) 1997,1998  Uwe Maurer - uwe_maurer@t-online.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/ggimesa.h>
#include <ggi/ggi.h>
#include <stdlib.h>

ggi_visual_t vis,vis_mem;

GGIMesaContext ctx;

int screen_x=GGI_AUTO,screen_y=GGI_AUTO;
ggi_graphtype bpp=GT_AUTO;

//#define ZBUFFER

//#define SMOOTH_NORMALS

void Init()
{
	GLfloat h=(GLfloat)3/4;
	GLfloat pos[4]={5,5,-20,0};
	GLfloat specular[4]={.4,.4,.4,1};
	GLfloat diffuse[4]={.3,.3,.3,1};
	GLfloat ambient[4]={.2,.2,.2,1};

	int err;

	if (ggiInit()<0)
	{
		printf("ggiInit() failed\n");
		exit(1);
	}
	ctx=GGIMesaCreateContext();
	if (ctx==NULL)
	{
		printf("Can't create Context!\n");
		exit(1);
	}

	vis=ggiOpen(NULL);
	vis_mem=ggiOpen("display-memory",NULL);
	if (vis==NULL || vis_mem==NULL)
	{
		printf("Can't open ggi_visuals!\n");
		exit(1);
	}	
	err=ggiSetGraphMode(vis,screen_x,screen_y,screen_x,screen_y,bpp);
	err+=ggiSetGraphMode(vis_mem,screen_x,screen_y,screen_x,screen_y,bpp);
	if (err)
	{
		printf("Can't set %ix%i\n",screen_x,screen_y);
		exit(1);
	}

	if (GGIMesaSetVisual(ctx,vis_mem,GL_TRUE,GL_FALSE)<0)
	{
		printf("GGIMesaSetVisual() failed!\n");
		exit(1);
	}

	GGIMesaMakeCurrent(ctx);

	glViewport(0,0,screen_x,screen_y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1,1,-h,h,1,50);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0,0,-9);
	glShadeModel(GL_FLAT);

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	glLightfv(GL_LIGHT0,GL_POSITION,pos);
	
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specular);

	#ifdef ZBUFFER
		glEnable(GL_DEPTH_TEST);
	#endif
}


#define MAX_VERTS 1000
#define MAX_TRIS  2000
#define MAX_LEN 1024
#define MAX_F 100000000

void LoadAsc(GLuint *list,char *file)
{
	FILE *fp;

	GLfloat p[MAX_VERTS][3];
	GLfloat normal[MAX_VERTS][3];
	float ncount[MAX_VERTS];
	int v[MAX_TRIS][3];
	char line[MAX_LEN];
	char *s;
	int  i,j;
	int verts,faces;
	GLuint v0,v1,v2;
	GLfloat n[3];
	GLfloat len,k;
	GLfloat min[3]={MAX_F,MAX_F,MAX_F};
	GLfloat max[3]={-MAX_F,-MAX_F,-MAX_F};
	char *coord_str[]={"X","Z","Y"};

	fp=fopen(file,"r");
	if (!fp) 
	{
		printf("Can't open %s!\n",file);	
		exit(1);
	}

	while (strncmp(fgets(line,MAX_LEN,fp),"Tri-mesh",8)) ;
	
	s=strstr(line,":")+1;
	verts=atoi(s);
	s=strstr(s,":")+1;
	faces=atoi(s);

	if (verts>MAX_VERTS)	
	{	
		printf("Too many vertices..\n");
		exit(1);
	}
	
	while (strncmp(fgets(line,MAX_LEN,fp),"Vertex list",11)) ;	

	for (i=0;i<verts;i++)
	{
		while (strncmp(fgets(line,MAX_LEN,fp),"Vertex",6)) ;	
		for (j=0;j<3;j++)
		{	
			s=strstr(line,coord_str[j])+2;
			k=atoi(s);
			if (k>max[j]) max[j]=k;
			if (k<min[j]) min[j]=k;
			p[i][j]=k;
		}
		
	}
	len=0;
	for (i=0;i<3;i++)
	{
		k=max[i]-min[i];
		if (k>len) {len=k;j=i;}
		n[i]=(max[i]+min[i])/2;
	}

	len/=2;

	for (i=0;i<verts;i++)
	{
		for (j=0;j<3;j++)
		{
			p[i][j]-=n[j];
			p[i][j]/=len;
		}
	}

	*list=glGenLists(1);
	glNewList(*list,GL_COMPILE);
	glBegin(GL_TRIANGLES);

	memset(ncount,0,sizeof(ncount));
	memset(normal,0,sizeof(normal));

	while (strncmp(fgets(line,MAX_LEN,fp),"Face list",9)) ;	
	for (i=0;i<faces;i++)
	{
		while (strncmp(fgets(line,MAX_LEN,fp),"Face",4)) ;	
		s=strstr(line,"A")+2;
		v0=v[i][0]=atoi(s);
		s=strstr(line,"B")+2;
		v1=v[i][1]=atoi(s);
		s=strstr(line,"C")+2;
		v2=v[i][2]=atoi(s);
		n[0]=((p[v1][1]-p[v0][1])*(p[v2][2]-p[v0][2]) 
			- (p[v1][2]-p[v0][2])*(p[v2][1]-p[v0][1])); 
		n[1]=((p[v1][2]-p[v0][2])*(p[v2][0]-p[v0][0]) 
			- (p[v1][0]-p[v0][0])*(p[v2][2]-p[v0][2])); 
		n[2]=((p[v1][0]-p[v0][0])*(p[v2][1]-p[v0][1]) 
			- (p[v1][1]-p[v0][1])*(p[v2][0]-p[v0][0])); 
		len=n[0]*n[0]+n[1]*n[1]+n[2]*n[2];
		len=sqrt(len);
		n[0]/=len;
		n[1]/=len;
		n[2]/=len;
	#ifdef SMOOTH_NORMALS	
		for (j=0;j<3;j++){
			normal[v[i][j]][0]+=n[0];
			normal[v[i][j]][1]+=n[1];
			normal[v[i][j]][2]+=n[2];
			ncount[v[i][j]]++;
		}
	#else
		glNormal3fv(n);
		for (j=0;j<3;j++)
			glVertex3fv(p[v[i][j]]);
	#endif
	}

	#ifdef SMOOTH_NORMALS
		for (i=0;i<verts;i++) {
			for (j=0;j<3;j++) {
				normal[i][j]/=ncount[i];
			}
		}
		for (i=0;i<faces;i++) {
			for (j=0;j<3;j++) {
				glNormal3f(normal[v[i][j]][0],
					   normal[v[i][j]][1],
					   normal[v[i][j]][2]);
				glVertex3fv(p[v[i][j]]);
			}
		}
	#endif

	glEnd();
	glEndList();
	fclose(fp);
}

double Display(GLuint l,int *maxframes)
{
	int x,y;
	GLfloat col[]={.25,0,.25,1};
	int frames=0;
	struct timeval start,stop;
	double len;
	GLfloat rotate=0;

	gettimeofday(&start,NULL);


	while(1)
	{
		glClearColor(0,0,0,0);
		glClearIndex(0);

		#ifdef ZBUFFER
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		#else
			glClear(GL_COLOR_BUFFER_BIT);
		#endif	

		glPushMatrix();
	
		glRotatef(30,1,0,0);
		glRotatef(rotate/10,0,0,1);
		glTranslatef(-6,-4,0);
		for (y=0;y<3;y++)
		{
			glPushMatrix();
			for (x=0;x<5;x++)
			{
				glPushMatrix();
				glRotatef(rotate,y+1,-x-1,0);

				col[0]=(GLfloat)(x+1)/4;
				col[1]=0;
				col[2]=(GLfloat)(y+1)/2;
				glMaterialfv(GL_FRONT,GL_AMBIENT,col);
				glCallList(l);
				glPopMatrix();
				glTranslatef(3,0,0);
			}
			glPopMatrix();
			glTranslatef(0,4,0);
		}
		glPopMatrix();
		glFinish();

		ggiPutBox(vis,0,0,screen_x,screen_y,ggiDBGetBuffer(vis,0)->read);
		rotate+=10;
		frames++;
		if (frames==(*maxframes)) break;

		if (ggiKbhit(vis))
		{
			*maxframes=frames;
			break;
		}
	}

	gettimeofday(&stop,NULL);
	len=(double)(stop.tv_sec-start.tv_sec)+
		(double)(stop.tv_usec-start.tv_usec)/1e6;	
	return len;
}

void visible(int vis)
{
	if (vis == GLUT_VISIBLE)
	  glutIdleFunc(idle);
	else
	  glutIdleFunc(NULL);
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(300, 300);
	glutCreateWindow("asc-view");
	init();
	
	glutDisplayFunc(draw);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutSpecialFunc(special);
	glutVisibilityFunc(visible);
	
	glutMainLoop();
#if 0
	GLuint l;
	char *file;
	int maxframes=0;
	double len;

	Init();

	file=(argc>1) ? argv[1] : "asc/box.asc";
	if (argc>2) maxframes=atoi(argv[2]);

	if (argc==1)
	{
		printf("usage: %s filename.asc\n",argv[0]);
	}

	LoadAsc(&l,file);

	len=Display(l,&maxframes);

	printf("\ttime: %.3f sec\n",len);
	printf("\tframes: %i\n",maxframes);
	printf("\tfps: %.3f \n",(double)maxframes/len);

	GGIMesaDestroyContext(ctx);
	ggiClose(vis);
	ggiClose(vis_mem);
	ggiExit();
#endif
	return 0;
}

