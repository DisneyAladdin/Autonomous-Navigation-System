#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <ncurses.h>
#include <getopt.h>
#include <pthread.h>

#include "shm.h"
#include "actd.h"
#include "urgmap_lib.h"
#include "urgd_lib.h"

#define ID  3 
#define PRIH 200
#define KWP 0.20f //0.4(original)
#define KWD 0.002f
#define KWI 0.000f

#define Speed 0.2

#define ODO_K 0.99963f
#define ODO_THETA 1.5f * M_PI / 180.0f/* rad */
#define DT	  0.105
#define DEFAULT_KEY 1000
#define DEFAULT_KEY1 2000

typedef struct {
        float x, y, t;
} Robo;

typedef struct{
        float nowl, nowr, oldl, oldr;
} Wheel;

Wheel wheel;
Robo del;

float tread = 0.56;
float robo_pos_x;
float robo_pos_y;
float robo_angle;
float direction,radian, X,Y,X0,Y0 =0;
float dl,dr,dd;
int   kill_flag = 0;
float *share_X, *share_Y ;
float *robo_angle_M;	//GPSから受信した絶対位置
int   *share_dgps;
FILE  *fp, *fp1;
int   stop_flag = 1;
float shareX0 = 0, shareY0 = 0;        

void wait_keyd(void)
{
	char c;
	while(kill_flag == 0){
		c = getchar();
		switch(c){
			case'q':
			kill_flag = 1;
			break;
			case ' ':
				if(stop_flag == 1)
					stop_flag = 0;
				if(stop_flag == 0)
					stop_flag = 1;
				break;
		}
	}
}


void position_init()
{
  	actd_init();
        actd_set_acc_time(ID, 3.0f);//1.0
        actd_set_dec_time(ID, 3.0f);//1.0

        actd_read_distances(&wheel.nowl, &wheel.nowr);
        wheel.oldl = wheel.nowl;
        wheel.oldr = wheel.nowr;

        robo_pos_x = 0.0f;
        robo_pos_y = 0.0f;
        robo_angle = 0.0;

        del.x = 0.0f;
        del.y = 0.0f;
        del.t = 0.0f;
}


void robot_pos(void)
{
 	float l, r, /*dl, dr,dd*/ dt, vt, p, _d, lx, ly, lt;

	actd_read_distances(&l, &r);

        wheel.nowl = l*ODO_K;
        wheel.nowr = r;
        dl = wheel.nowl - wheel.oldl;
        dr = wheel.nowr - wheel.oldr;
        dd = (dr + dl) / 2.0f;
        dt = (dr - dl) / tread;
        vt = fabs(dt);
        p = dd / dt;
	_d = 2.0f * p * sin(dt / 2.0f);

        if (vt > ODO_THETA) {
		del.x = _d * cos(robo_angle + dt / 2.0f);
		del.y = _d * sin(robo_angle + dt / 2.0f);
                lx = robo_pos_x + del.x;
                lx = robo_pos_x + del.x;
                ly = robo_pos_y + del.y;
                robo_pos_x = lx;
                robo_pos_y = ly;
        } else {
                del.x = dd * cos(robo_angle + dt / 2.0f);
                del.y = dd * sin(robo_angle + dt / 2.0f);
                lx = robo_pos_x + del.x;
                lx = robo_pos_x + del.x;
                ly = robo_pos_y + del.y;
                robo_pos_x = lx;
                robo_pos_y = ly;
        }
        wheel.oldl = wheel.nowl;
        wheel.oldr = wheel.nowr;

	if(fabs(*share_X - shareX0) > 1 || fabs(*share_Y - shareY0) > 1){

		shareX0 = robo_pos_x = *share_X;
		shareY0 = robo_pos_y = *share_Y;
	}


        del.t = dt;
        lt = robo_angle + dt;

        if (lt > 2.0*M_PI) {
                lt -= 2.0*M_PI;
        } else if (lt < 0) {
                lt += 2.0*M_PI;
        }
        robo_angle = lt;
	printf("X=%.2f Y=%.2f x:%f y:%f ang:%f distance:%f GPS_X:%f GPS_Y:%f X0=%.2f Y0=%.2f radian=%f\n", 
X, Y, robo_pos_x, robo_pos_y, lt, (wheel.nowl+wheel.nowr)/2 ,*share_X, *share_Y, shareX0, shareY0,radian);
}










void file (void)
{
	static float kyori = 0.0f;
	char str1[128] = {0};

	kyori = kyori + dd;
	if(kyori >= 0.5){
		sprintf(str1,"%f %f \n" ,robo_pos_x ,robo_pos_y);
		fprintf(fp1,"%s", str1);
		kyori = 0;
	}
}











int main(int argc, char *argv[])
{
	pthread_t tid;
	
	unsigned int key = DEFAULT_KEY;
	unsigned int key1= DEFAULT_KEY1;
	share_X		= shm_get_buf(key  ,sizeof(float));
	share_Y 	= shm_get_buf(key+1,sizeof(float));
	share_dgps 	= shm_get_buf(key+2,sizeof(int));
	robo_angle_M    = shm_get_buf(key1,sizeof(float));
        *robo_angle_M = 0;
	*share_X = *share_Y = 0;

	float target_x, target_y;//[m]
	float dx, dy, dl;
	float th, e, MV_th, vl, vr;
	//float X0 =0 , Y0 = 0;
 	static float eo = 0.0f, es = 0.0f;
	int check;	

	fp = fopen("test1.txt","r");
	fp1= fopen("test2.txt","w");
	if(fp == NULL){
		printf("ファイルをオープンできませんでした\n");
		exit(-1);
	}
	else{
		printf("ファイルをオープンできました\n");
	}
	
	position_init();	//タイヤの回転数を見る
	pthread_create(&tid,NULL,(void *)wait_keyd,NULL);
	while(kill_flag != 1){
		if(fscanf(fp," %f %f", &X, &Y) == EOF)
			break;
		if(fabs(X0 - X) > 1.0f || fabs(Y0 -Y)>1.0f ){
			target_x = X;
			target_y = Y;

			for(;;){
	       			usleep(100000);
				robot_pos();
				file();
				dx = target_x - robo_pos_x;
				dy = target_y - robo_pos_y;
				dl = sqrt(dx*dx + dy*dy);

				if (dx > 0.0f && dy >= 0.0f) {
        	        		th = atan(dy / dx);
		        	} else if (dx == 0.0f && dy >= 0.0f) {
        		        	th = M_PI / 2.0f;
				} else if (dx < 0.0f && dy >= 0.0f) {
		               		th = M_PI - atan(dy / -dx);
				} else if (dx < 0.0f && dy < 0.0f) {
        		        	th = M_PI + atan(-dy / -dx);
				} else if (dx == 0.0f && dy < 0.0f) {
		                	th = 3.0f * M_PI / 2.0f;
		        	} else {
        		        	th = 2.0f * M_PI - atan(-dy / dx);
	        		}
		
				e = th - robo_angle;
	                	if (e < -M_PI) {
        	                	e += 2.0f * M_PI;
                		} else if (e > M_PI) {
                        		e -= 2.0f * M_PI;
	                	}
        	        	MV_th  = (KWP * e) + KWD * (e - eo) + KWI * (e + es);
                		vl = Speed  - MV_th * tread/2.0f;
	                	vr = Speed  + MV_th * tread/2.0f;
        	        	eo = e;
                		es += e;
				if(vl > Speed + 0.15)
					vl = Speed + 0.15;
				if(vr > Speed + 0.15)
					vr = Speed + 0.15;
				if(vl < 0)
					vl = 0;
				if(vr < 0)
					vr = 0;
				actd_set_velocities(ID, PRIH, vl*stop_flag, vr*stop_flag);

	                	if(dl < 2.0)
        	                	break;
			}
			X0 = X;
			Y0 = Y;
			
		}
	}
	actd_set_velocities(ID, PRIH, 0.0, 0.0);
       	usleep(100000);

	return 0;
}
