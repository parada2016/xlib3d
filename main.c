#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
Display * dis;
Window win;
GC gcontext;
Atom wm_delete_window;
XEvent report;
XWindowAttributes xwatt;
int XRes,YRes,MaxX,MaxY,MinX,MinY;
#define TRUE (1==1)


unsigned long rgb2long(unsigned long r, unsigned long g, unsigned long b)
{
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}


// *********************

typedef struct { double x; double y; double z;}Ponto3D;
typedef struct { Ponto3D ox; Ponto3D oy; Ponto3D oz; }View;


XPoint Converte(Ponto3D p)
{
    XPoint pp={
    .x = p.x,
    .y = p.y
    };
    return pp;
}

void triangle(Ponto3D * vert, int nPontos)
{
    XPoint tr[3];
    tr[0] = Converte(vert[0]);
    tr[1] = Converte(vert[1]);
    tr[2] = Converte(vert[2]);
    XFillPolygon(dis,win,gcontext, tr, nPontos, Nonconvex,CoordModeOrigin);
}

void putpixel(int x, int y)
{
  XDrawPoint(dis,win,gcontext,x,y);
}

double VecDot(Ponto3D A, Ponto3D B)
{
    return(A.x * B.x + A.y*B.y + A.z*B.z);
}

static View Cam;
static double SinX, SinY, CosX, CosY, SinZ, CosZ;

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

Ponto3D TSR(Ponto3D in, int dist){
Ponto3D out;
 out.x = VecDot(in,Cam.ox);
 out.y = VecDot(in,Cam.oy);
 out.z = VecDot(in,Cam.oz) + dist;
 return(out);
}

void ProjetaVert(Ponto3D * prj, Ponto3D vert[], int n, double escala)
{
 Ponto3D p;
 Ponto3D out;
 int i;
 int Cx = MaxX >> 1;
 int Cy = MaxY >> 1;
 for(i=0; i < n; i++)
 {
   out = TSR(vert[i],100);
   p.x = Cx + escala*out.x * (double)MaxX/((double)MaxX + out.z);
   p.y = Cy - escala*out.y * (double)MaxX/((double)MaxX + out.z);
   p.z = out.z;
   prj[i] = p;
 }
}
typedef int face[3];
static Ponto3D * Vertices;
static face * Faces;
int ntFaces;
int nVertices;

void CalcEsfera (int  longitude,int latitude) {
  ntFaces = longitude + 2 * (latitude - 2)*longitude;
  nVertices = (longitude - 2)*latitude + 2;

}

// esfera
void CriarEsfera (Ponto3D centro,  float raio, int  longitude,int latitude) {
  int i,j,j2, s1, s2;
  float r2,y2, angle;
  Ponto3D p;
  float PiOver180 = M_PI/180.0;
  int count=0;


    for (i = 1; i < latitude; i++) {
         r2 = raio * sin((180 * (double)i / latitude)*PiOver180);
         y2 = centro.y - raio * cos((180 * (double)i / latitude *PiOver180));

        for (j = 0; j < longitude; j++) {
            angle = 360*((double)j + (double)i/2.0) / latitude;
            p.x = centro.x + r2 * cos(angle * PiOver180);
            p.y = y2;
            p.z = centro.z + r2 * sin(angle *PiOver180);
            Vertices[count++]  = p;

        }
    }
    // Polos
   p.x = centro.x;
   p.y = centro.y - raio;
   p.z = centro.z;
   Vertices[count++] = p;


   p.x = centro.x;
   p.y = centro.y + raio;
   p.z = centro.z;
  Vertices[count] = p;


//n = nVertices;
count = 0;
   // Faces polares
    for (j = 0; j < longitude; j++) {
         j2 = (j + 1) % longitude;

         Faces[count][0] = j2;
         Faces[count][1] = j;
         Faces[count][2] = nVertices-2;
         count++;
         Faces[count][0] = nVertices-3-j2;
         Faces[count][1] = nVertices-3-j;
         Faces[count][2] = nVertices-1;
         count++;

    }

     // Outras faces
    for (i = 1; i < latitude - 1; i++) {

        for (j = 0; j < longitude; j++) {
             s1 = (i - 1) * longitude;
             s2 = i * longitude;
             j2 = (j + 1) % longitude;
             Faces[count][0] = s1 + j;
             Faces[count][1] = s1 + j2;
             Faces[count][2] = s2 + j;
             count++;

             Faces[count][0] = s1 + j2;
             Faces[count][1] = s2 + j2;
             Faces[count][2] = s2 + j;
             count++;

        }
    }

}

Ponto3D VecSub(Ponto3D A, Ponto3D B){
Ponto3D r={
        .x = (A.x - B.x),
        .y = (A.y - B.y),
         .z = (A.z - B.z)
      };
return(r);
}

Ponto3D VecCross(Ponto3D A, Ponto3D B){
Ponto3D r={
       .x= (A.y*B.z - B.y*A.z),
       .y= (A.z*B.x - B.z*A.x),
       .z= (A.x*B.y - B.x*A.y),
      };
return(r);
}

double VecLen(Ponto3D A){
  return(sqrt(A.x * A.x + A.y * A.y + A.z * A.z));
}

double Sombra(Ponto3D VNormal){
 double normalLen = VecLen(VNormal);
 double normalAngleRad = cos(VNormal.z/normalLen);
 double l = 1 - 2.7*(normalAngleRad / M_PI);
	l = (l > 0.0 && l < 1.0)? l : 0.3;

return(l);
}

static Ponto3D * tr;

void Desenha()
{


    Ponto3D points[3];
    Ponto3D N,V1,V2;
    double l;
    int i;
    ProjetaVert(tr,Vertices,nVertices,1.5);

   for(i=0; i < ntFaces; i++){
     points[0] =tr[Faces[i][0]];
     points[1] =tr[Faces[i][1]];
     points[2] =tr[Faces[i][2]];
     V1 = VecSub(points[1],points[0]);
     V2 = VecSub(points[0],points[2]);
     N = VecCross(V1,V2);

      if(N.z < 0){
      l = Sombra(N);
      XSetForeground(dis,gcontext,rgb2long(255*l,155*l,100*l));
      triangle(points,3);
     }
  }

}

void closegraph()
{

   XCloseDisplay(dis);
    free(Vertices);
    free(Faces);
     free(tr);
     printf("Bye bye...");

}


int main()
{
    Ponto3D centro = {0,0,0};
    XRes = 640;
    YRes = 480;
    char msg[]="Use as teclas de seta para girar a esfera";
    double angulo = 0;
    if(NULL != (dis = XOpenDisplay(NULL)))
    {
        win = XCreateSimpleWindow(dis,RootWindow(dis, 0),1,1,XRes,YRes,1,
                                  BlackPixel(dis,0), WhitePixel(dis,0));
        gcontext = XCreateGC(dis,win,0,0);
        XSelectInput(dis, win, StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask);
        wm_delete_window =XInternAtom (dis, "WM_DELETE_WINDOW", False);
        XSetWMProtocols (dis,win, &wm_delete_window, 1);
        XGetWindowAttributes(dis, win, &xwatt);
        MaxX = xwatt.width - 1;
        MaxY = xwatt.height - 1;
        MinX = xwatt.x;
        MinY = xwatt.y;
        XFlush(dis);
        XMapWindow(dis, win);
        XFlush(dis);
    }
    else exit(1);
    SetCam(0,0,0);
    CalcEsfera(24,12);
    Faces = (face *)malloc(sizeof(face) * ntFaces * 3);
    Vertices = (Ponto3D *)malloc(sizeof(Ponto3D) * nVertices * 3);
     tr = (Ponto3D *)malloc(sizeof(Ponto3D) * nVertices);
    CriarEsfera(centro,150,24,12);
    atexit(closegraph);
    while(TRUE)
    {
        if(XPending(dis) > 0)
        {
            XNextEvent(dis,&report);
            switch(report.type)
            {
            case KeyPress:
                switch(XLookupKeysym(&report.xkey, 0)){
                  case XK_q:
                  case XK_Q:

                     return(0);
                     break;
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
                  XDrawString(dis,win,gcontext,30,30,msg,strlen(msg));
                  Desenha();
                  break;
            case ClientMessage:
                if((Atom)report.xclient.data.l[0] == wm_delete_window)

                    return(0);
            break;

            }
        }
        XFlush(dis);
    }

    return(0);
}
