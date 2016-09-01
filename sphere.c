#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
typedef struct { double x,y,z; } Point3D;
typedef struct { Point3D ox, oy, oz; } View;

static int MaxX = 640;
static int MaxY = 480;
Display *dis;
Window  win;
GC  gcontext;
XEvent report;
 Atom wm_delete_window;
 XWindowAttributes watt;
typedef int face[3];
 static Point3D * nodes;
static face * tfaces;
int nVertices = 0;
int ntFaces = 0;


 void initgraphics(){
 dis = XOpenDisplay(NULL);
 win = XCreateSimpleWindow(dis,RootWindow(dis, 0),1,1,MaxX,MaxY,1,BlackPixel(dis,0), WhitePixel(dis,0));
 gcontext = XCreateGC(dis,win,0,0);
 XSelectInput(dis, win, StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask);
 wm_delete_window =XInternAtom (dis, "WM_DELETE_WINDOW", False);
 XSetWMProtocols (dis,win, &wm_delete_window, 1);
 XGetWindowAttributes(dis, win, &watt);
 XFlush(dis);
 XMapWindow(dis, win);
 XFlush(dis);
}

static View Cam;

static double SinX, SinY, CosX, CosY, SinZ, CosZ;

void putpixel(int x, int y){ XDrawPoint(dis,win,gcontext, x,y);}
void fillpoly(XPoint verts[],int n) { XFillPolygon(dis,win,gcontext,verts,n,Nonconvex,CoordModeOrigin); }


double VecDot(Point3D A, Point3D B)
{
    return(A.x * B.x + A.y*B.y + A.z*B.z);
}

void  SetCam(double ax,double ay, double az){
SinX = sin(ax);  SinY = sin(ay);  SinZ = sin(az);
CosX = cos(ax);  CosY = cos(ay);  CosZ = cos(az);

Cam.ox.x= (CosY * CosX);
Cam.ox.y= (CosY * SinX);
Cam.ox.z= SinY;

Cam.oy.x= (- CosZ * SinX - SinZ * SinY * CosX);
Cam.oy.y= (CosZ * CosX - SinZ * SinY * SinX);
Cam.oy.z= (SinZ * CosY);

Cam.oz.x= (SinZ * SinX - CosZ * SinY * CosX);
Cam.oz.y= (- SinZ * CosX - CosZ * SinY * SinX);
Cam.oz.z= (CosZ * CosY);
}

Point3D TSR(Point3D in, int dist){
Point3D out;
 out.x = VecDot(in,Cam.ox);
 out.y = VecDot(in,Cam.oy);
 out.z = VecDot(in,Cam.oz) + dist;
 return(out);
}


void Project(Point3D * prj, Point3D vert[], int n, double scale)
{
 Point3D p;
 Point3D out;
 int i;
 int Cx = MaxX >> 1;
 int Cy = MaxY >> 1;
 for(i=0; i < n; i++)
 {
   out = TSR(vert[i],100);
   p.x = Cx + scale*out.x * (double)MaxX/((double)MaxX + out.z);
   p.y = Cy - scale*out.y * (double)MaxX/((double)MaxX + out.z);
   p.z = out.z;
   prj[i] = p;
 }
}

void readfromfile()
{
 FILE * svert, * sfaces;
// PRIMEIRO VÉRTICES
 svert = fopen("svertices.txt","r");
 int f,v;
 if(svert == NULL)
 {
   printf("Erro no arquivo!");exit(1);
 }
Point3D p;
char x[100], y[100],z[100];
 nVertices = 0;
 fscanf(svert,"%d",&v);
 nodes = (Point3D *)malloc(sizeof(Point3D) * v * 3);
 while(!feof(svert) )
 {
  fscanf(svert,"%s %s %s",x,y,z);
  p.x = atof(x);
  p.y = atof(y);
  p.z = atof(z);
  nodes[nVertices++] = p;
}

fclose(svert);
// AGORA AS FACES
sfaces = fopen("sfaces.txt", "r");
if(sfaces == NULL)
{

printf("Erro no arquivo"); exit(1);
}
ntFaces = 0;
fscanf(sfaces,"%d",&f);
tfaces = (face *)malloc(sizeof(face) * f * 3);
while(!feof(sfaces))
{
fscanf(sfaces,"%s %s %s",x,y,z);
 tfaces[ntFaces][0] = atoi(x);
 tfaces[ntFaces][1] = atoi(y);
 tfaces[ntFaces][2] = atoi(z);
 ntFaces += 1;
}
 fclose(sfaces);
 ntFaces = f;
 nVertices = v;
}

Point3D VecSub(Point3D A, Point3D B){
Point3D r={
        .x = (A.x - B.x),
        .y = (A.y - B.y),
         .z = (A.z - B.z)
      };
return(r);
}

Point3D VecCross(Point3D A, Point3D B){
Point3D r={
       .x= (A.y*B.z - B.y*A.z),
       .y= (A.z*B.x - B.z*A.x),
       .z= (A.x*B.y - B.x*A.y),
      };
return(r);
}

double VecLen(Point3D A){
  return(sqrt(A.x * A.x + A.y * A.y + A.z * A.z));
}

double Sombra(Point3D VNormal){
 double normalLen = VecLen(VNormal);
 double normalAngleRad = cos(VNormal.z/normalLen);
 double l = 1 - 2.7*(normalAngleRad / M_PI);
	l = (l > 0.0 && l < 1.0)? l : 0.3;

return(l);
}

unsigned long rgb2long(unsigned long r, unsigned long g, unsigned long b)
{
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

static Point3D * tr;
void Desenha()
{
    Point3D * tr = (Point3D *)malloc(sizeof(Point3D) * nVertices);
    XPoint face[3];
    Point3D points[3];
    Point3D N,V1,V2;
    double l;
    int i,j;
    Project(tr,nodes,nVertices,1.5);

   for(i=0; i < ntFaces; i++){
     points[0] =tr[tfaces[i][0]];
     points[1] =tr[tfaces[i][1]];
     points[2] =tr[tfaces[i][2]];
     V1 = VecSub(points[1],points[0]);
     V2 = VecSub(points[0],points[2]);
     N = VecCross(V1,V2);

      if(N.z < 0){
      l = Sombra(N);
      XSetForeground(dis,gcontext,rgb2long(255*l,155*l,100*l));
      for(j=0; j < 3; j++){
        face[j].x = points[j].x;
        face[j].y = points[j].y;
      }
       fillpoly(face,3);
     }
  }
   free(tr);
}


int main()
{
initgraphics();
double angulo = 0;
SetCam(0,0,0);
readfromfile();
  while(1){
    XNextEvent(dis,&report);
    switch(report.type)
    {
    case KeyPress:
    switch(XLookupKeysym(&report.xkey, 0))
    {
      case XK_Q:
      case XK_q:
          XCloseDisplay(dis);
          return(0);
      case XK_Up:
          angulo += 0.03;
          SetCam(angulo,angulo,-angulo);
          break;

       case XK_Down:
          angulo -= 0.03;
          SetCam(angulo,angulo,-angulo);
          break;
    }
    case Expose:
         XClearWindow(dis,win);
         Desenha();
         break;
    case ClientMessage:
         if((Atom)report.xclient.data.l[0] == wm_delete_window)
           XCloseDisplay(dis);
           return(0);
         break;


    }






  }
free(tfaces);
free(nodes);

return(0);
}

