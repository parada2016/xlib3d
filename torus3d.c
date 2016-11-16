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


typedef int rec[4];

typedef struct{

Ponto3D  * VertArr;
int nVert;
rec *  RecArr;
int nFaces;
}MeshTorus;


typedef struct { Ponto3D ox; Ponto3D oy; Ponto3D oz; }View;

int AlocarTorus(double r1,double r2, int n1,MeshTorus * torus)
{
      // Create a torus
    // The torus has radius r1 < r2 and n1,n2, with n2== (n1 + 1) cross-sections.

    double  dx, dy;
    int i,j;
    double theta = -2.0*M_PI/(double)n1;
    Ponto3D p,p2;
    int n2 = n1 + 1;

    int nVert = n2*n1;
    int nFaces= (n2-1)*(n1);
    torus->nVert = nVert;
    torus->nFaces = nFaces;
    torus->VertArr = (Ponto3D *)malloc(sizeof(Ponto3D)* nVert);
    torus->RecArr = (rec *)malloc(sizeof(rec) * nFaces);
    if((torus->VertArr == NULL) || (torus->RecArr == NULL)) return(0);
     // if r1 > r2/2 the torus will look a bit weird. A don't know how to fix this yet.
    if(r1 > r2/2.0) r1 = r2/2.0;
    nVert = 0;
    for (j = 0; j < n2; j++) {
         p.x = r2 * cos(theta * (double)j);
         p.y = 0.0;
         p.z = -r2 * sin(theta * (double)j);
        for (i = 0; i < n1; i++) {
            dx = r1 * cos(theta * (double)i);
            dy = r1 * sin(theta * (double)i);
            p2.x = p.x + dx * cos(theta * (double)j);
            p2.y = p.y + dy;
            p2.z = p.z - dx * sin(theta * (double)j);
            torus->VertArr[nVert++] = p2;
        }// end for
    }//end for
    nFaces = 0;

    for (j = 0; j < n2-1; j++) {
        int s = j * n1;
        for (i = 0; i < n1; i++) {
            if (i < n1 - 1) {
                torus->RecArr[nFaces][0] = s+i+1;
                torus->RecArr[nFaces][1] = s+i;
                torus->RecArr[nFaces][2] = s+i+n1;
                torus->RecArr[nFaces][3] = s+i+n1+1;

            } else {

                torus->RecArr[nFaces][0] = s;
                torus->RecArr[nFaces][1] = s+i;
                torus->RecArr[nFaces][2] = s+i+n1;
                torus->RecArr[nFaces][3] = s+n1;

            }// end if
            nFaces++;
         }//end for

    }//end for

    return(1);

}





XPoint Converte(Ponto3D p)
{
    XPoint pp={
    .x = p.x,
    .y = p.y
    };
    return pp;
}


void rectangle(Ponto3D * vert)
{
    XPoint Face[4];
    Face[0] = Converte(vert[0]);
    Face[1] = Converte(vert[1]);
    Face[2] = Converte(vert[2]);
    Face[3] = Converte(vert[3]);
    XFillPolygon(dis,win,gcontext, Face, 4, Nonconvex,CoordModeOrigin);
}

void putpixel(int x, int y)
{
  XDrawPoint(dis,win,gcontext,x,y);
}

double VecDot(Ponto3D A, Ponto3D B)
{
    return(A.x * B.x + A.y*B.y + A.z*B.z);
}

const View DefCam={ { 0.97,-0.02,0.24 },
                    {-0.046,0.97,0.25},
                    {-0.24,-0.25,0.94}
                   };
static View Cam={   { 0.97,-0.02,0.24 },
                    {-0.046,0.97,0.25},
                    {-0.24,-0.25,0.94}
                   };


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

Ponto3D TSR(Ponto3D in, View cam, int dist){
Ponto3D out;
 out.x = VecDot(in,cam.ox);
 out.y = VecDot(in,cam.oy);
 out.z = VecDot(in,cam.oz) + dist;
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
   out = TSR(vert[i],Cam, 100);
   p.x = Cx + escala*out.x * (double)MaxX/((double)MaxX + out.z);
   p.y = Cy - escala*out.y * (double)MaxX/((double)MaxX + out.z);
   p.z = out.z;
   prj[i] = p;
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
  double dir,l=1,len,Cos;
     Ponto3D light = { 30,30,-30};
     const double contraste = 0.6;

    light = TSR(light,DefCam,150);

     len= (VecLen(VNormal)*VecLen(light));
     if(len > 0.001)
    Cos = VecDot(VNormal,light)/len;
    else return(l);
   dir = 1.0 - Cos*Cos;
   if(dir >= 0) dir = sqrt(dir);
   dir = 1 - contraste*dir;
   l = (dir > 0.0 && dir < 1.0)? dir : 0;

return(l);
}


Ponto3D NormalDaFace(Ponto3D pts[])
{
   Ponto3D N,V1,V2;
    V1 = VecSub(pts[1],pts[0]);
     V2 = VecSub(pts[3],pts[0]);
     N = VecCross(V1,V2);
     return(N);

}

static Ponto3D * tr;

void DrawTorusMesh(MeshTorus torus)
{
    Ponto3D points[4];
    Ponto3D N;
    double l;
    int i;
    ProjetaVert(tr,torus.VertArr,torus.nVert,0.7);

   for(i=0; i < torus.nFaces; i++){
     points[0] =tr[torus.RecArr[i][0]];
     points[1] =tr[torus.RecArr[i][1]];
     points[2] =tr[torus.RecArr[i][2]];
     points[3] =tr[torus.RecArr[i][3]];
     N = NormalDaFace(points);

      if(N.z >= 0){
      l = Sombra(N);
      XSetForeground(dis,gcontext,rgb2long(255*l,45*l,155*l));
      rectangle(points);
     }// end if
   }// end for

}


MeshTorus torus;

void closegraph()
{

   XCloseDisplay(dis);

     free(torus.RecArr);
     free(torus.VertArr);
     free(tr);
     printf("Bye bye...");

}


int main()
{


    XRes = 800;
    YRes = 800;
    int Raio = (int)(2.2*(double)(YRes-20)/7.0);
    int canto;
    char msg[]="Use as teclas de seta para girar o Torus (arrow keys to rotate the torus - Q to quit)";
    double angulo = 15;
    if(AlocarTorus(Raio/1.3,Raio,20,&torus)){

     tr = (Ponto3D *)malloc(sizeof(Ponto3D) * torus.nVert);
     if(tr == NULL)
     {
         printf("Erro de alocacao de vertices");
         return(1);
     }
    }
    else{

        printf("nao foi possível alocar esfera");
        return(2);

    }
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
    SetCam(2.2,angulo,-1.5);
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
                    case XK_Right:
                     angulo += 0.03;
                     SetCam(2.2,angulo,-1.5);
                     break;
                   case XK_Down:
                   case XK_Left:
                     angulo -= 0.03;
                     SetCam(2.2,angulo,-1.5);
                     break;


                }
            case Expose:
                  XDrawString(dis,win,gcontext,20,20,msg,strlen(msg));
                  XSetForeground(dis,gcontext,0);
                  canto = Raio * sqrt(2);
                  XFillRectangle(dis,win,gcontext,XRes/2 - canto,YRes/2 - canto,2*canto,2*canto);
                  DrawTorusMesh(torus);

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
