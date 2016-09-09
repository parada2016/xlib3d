
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef struct{ float x; float y; float z; }Point3D;
int nVertices = 0;
int ntFaces = 0;

FILE * faces, *vertices;




// esfera
void CriarEsfera (Point3D centro,  float raio, int  longitude,int latitude) {
  int i,j,n,j2, s1, s2;
  float r2,y2, angle;
  Point3D p;
  float PiOver180 = M_PI/180.0;


    for (i = 1; i < latitude; i++) {
         r2 = raio * sin((180 * (double)i / latitude)*PiOver180);
         y2 = centro.y - raio * cos((180 * (double)i / latitude *PiOver180));

        for (j = 0; j < longitude; j++) {
            angle = 360*((double)j + (double)i/2.0) / latitude;
            p.x = centro.x + r2 * cos(angle * PiOver180);
            p.y = y2;
            p.z = centro.z + r2 * sin(angle *PiOver180);
            fprintf(vertices,"%3.3f %3.3f %3.3f ", p.x, p.y, p.z);

        }
    }
    // Polos
   p.x = centro.x;
   p.y = centro.y - raio;
   p.z = centro.z;
   fprintf(vertices,"%3.3f %3.3f %3.3f ", p.x, p.y, p.z);


   p.x = centro.x;
   p.y = centro.y + raio;
   p.z = centro.z;
   fprintf(vertices,"%3.3f %3.3f %3.3f ", p.x, p.y, p.z);


n = nVertices;
   // Faces polares
    for (j = 0; j < longitude; j++) {
         j2 = (j + 1) % longitude;

         fprintf(faces,"%d %d %d ", j2, j, n-2);
         fprintf(faces,"%d %d %d ", n-3-j2, n-3-j, n-1);

    }

     // Outras faces
    for (i = 1; i < latitude - 1; i++) {

        for (j = 0; j < longitude; j++) {
             s1 = (i - 1) * longitude;
             s2 = i * longitude;
             j2 = (j + 1) % longitude;

             fprintf(faces,"%d %d %d ", s1+j, s1 + j2, s2 + j);

             fprintf(faces,"%d %d %d ", s1+j2, s2 + j2, s2 + j);

        }
    }

}

void CalcEsfera (int  longitude,int latitude) {
  ntFaces = longitude + 2 * (latitude -2)*longitude;
  nVertices = (longitude - 2)*latitude + 2;

}



int main()
{

Point3D centro =  {0,0,0};
char temp[100];
int x;
float p;


CalcEsfera(24,12);
vertices = fopen("svertices.txt","w+");
fprintf(vertices,"%d ",nVertices);
faces = fopen("sfaces.txt","w+");
fprintf(faces,"%d ",ntFaces);
CriarEsfera(centro,150,24,12);
fclose(vertices);
fclose(faces);
return(0);
}

