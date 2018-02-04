#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rs232c.h"
#include<math.h>
#include<pthread.h>
#include <stdio.h>
#include"shm.h"
#define DEFAULT_KEY 1000

static RS_PORT backup_attr;
float  aaa,bbb,ccc;
float  result1,result2;
float  X,Y;
FILE *fp, *fp1, *fp2;
float *share_X, *share_Y = 0;
int *share_dgps = 0;
int zokko;
int dgps;
int set_num;
int kill_flag = 0;
float offset_X;
float offset_Y;
int FLAG = 0;;
char LATITUDE[16] ;
char LONGITUDE[16];



void wait_keyd(void)					//"q"を押すと強制終了
{
	char c;
	while(kill_flag == 0){
		c = getchar();
		switch(c){
		case'q':kill_flag = 1;
		break;
		}
	}
}



void ido(char LATITUDE[16])				//緯度を計算
        {
                char hun[3];
                char doo[3];
                char byo[9];
                float d,h,b;
		memset(doo, '\0', 3);
	        memset(hun,'\0',3);
		memset(byo,'\0',9);
                strncpy(doo, LATITUDE,2);
		strncpy(hun, LATITUDE + 2,2);
		strncpy(byo, LATITUDE + 4,8);
		printf("dooは%sです\n",doo);
                printf("hunは%sです\n",hun);
                printf("byoは%sです\n",byo);
		
		d=atof(doo);
		h=atof(hun);
		b=atof(byo)*60;
		result1=d + h/60 + b/60/60;
}







void  keido(char LONGITUDE[16])				//経度を計算
{
		char doo[6];
		char hun[6];
		char byo[9];
		float d,h,b;
		memset(doo,'\0',6);
		memset(hun,'\0',6);
		memset(byo,'\0',9);
		strncpy(doo,LONGITUDE,3);
		strncpy(hun,LONGITUDE+3,2);
		strncpy(byo,LONGITUDE+5,8);               
		printf("dooは%sです\n",doo);
		printf("hunは%sです\n",hun);
		printf("byoは%sです\n",byo);
		
		d=atof(doo);
		h=atof(hun);
		b=atof(byo)*60;
		result2=d + h/60 + b/60/60;
             
}






void encodeXY(void)				//緯度、経度からX,Y座標を計算
{
        float LAT,LON;
        float a,e,f,N;

        a=6378137.000;
        f=1/298.257223563;
        e=sqrt(2*f-f*f);

        LAT=(M_PI/180)*result1;
        LON=(M_PI/180)*result2;
        N=a/sqrt(1-e*e*sin(LAT)*sin(LAT));
        X=N*cos(LAT)*cos(LON);
        Y=N*cos(LAT)*sin(LON);
}









void file (void)				//初期値を引いたX,Y座標をファイルに保存
{
        char str1[128];
	char str2[128];
        memset(str1,'0',128);
	memset(str2,'0',128);
        sprintf(str1, "%.3f  %.3f \n",(X - offset_X)*1.35 ,(Y - offset_Y)*1.35);
        sprintf(str2,"%.3f %.3f %.3f %.3f %s %s %.3f %.3f\n",X,Y,offset_X,offset_Y,LATITUDE, LONGITUDE,result1,result2);
	
	fprintf(fp, str1);
	fprintf(fp1,str2);
}




void send (void)
{
	//if(fabs((X-offset_X)*1.35 - *share_X) > 3.0 || fabs((Y-offset_Y)*1.35 - *share_Y) > 3.0){
		*share_X = (X - offset_X)*1.35;
		*share_Y = (Y - offset_Y)*1.35;
	//}
}


int main(int argc, char *argv[])
{
	pthread_t tid;
  	int i, j, fd;
	int buf_len;
	int k[20];
	float X0=0;
	float Y0=0;
  	char buf[256];
	char MEASURE_utc[16];
	char NS[16];
	char EW[16];
	char DGPS[16];
	char SET_NUM[16];
	unsigned int key = DEFAULT_KEY;
	share_X = shm_get_buf(key, sizeof(float));
	share_Y = shm_get_buf(key+1, sizeof(float));
 	share_dgps = shm_get_buf(key+2, sizeof(int));	
	fd = open("/dev/ttyUSB_gps", O_RDWR);
  	if (fd < 0) {
  		printf("Can't Open Port\n");
		exit(-1);
  
	} 


 	fp = fopen("../auto_move/database.txt","w");
	fp1 = fopen("test2.txt","w");
	fp2 = fopen("test3.txt","w");
        if(fp == NULL){
        printf("ファイルをオープンできませんでした\n");
        return 1;
        }
        else{
            printf("ファイルをオープンできました\n");
        }



  	rs_get_attr(fd, &backup_attr);
  	rs_set_cflag(fd, IGNPAR);
  	rs_set_cflag(fd, B19200 | CS8 | CREAD | CLOCAL);
  	rs_set_icanon(fd);
  
	pthread_create(&tid,NULL,(void*)wait_keyd,NULL);
	while(kill_flag !=1){	 
		memset(MEASURE_utc, '\0', 16);
		memset(LATITUDE,    '\0', 16);
		memset(NS,          '\0', 16);
		memset(LONGITUDE,   '\0', 16);
		memset(EW,          '\0', 16);
		memset(DGPS,        '\0', 16);
		memset(SET_NUM,     '\0', 16);
    		
		sprintf(buf, "$JASC\0");
    		rs_write(fd, buf, 5);
    		rs_read(fd, buf, 256);
		if(buf[0] == '$' && buf[1] == 'G' && buf[2] == 'P'){
			sleep(1); 
			j = 0;
			buf_len = strlen(buf);
			fprintf(fp2, "%s\n", buf);
			for(i = 0; i < buf_len; i++){
				if(buf[i] == ','){
					k[j] = i;
					j++;
				}
				if(j > 15)
					break;
			}
			if(j > 10){
				strncpy(MEASURE_utc, buf + k[0] + 1, k[1] - k[0] - 1);
				strncpy(LATITUDE,    buf + k[1] + 1, k[2] - k[1] - 1);
				strncpy(NS,          buf + k[2] + 1, k[3] - k[2] - 1);
				strncpy(LONGITUDE,   buf + k[3] + 1, k[4] - k[3] - 1);
				strncpy(EW,          buf + k[4] + 1, k[5] - k[4] - 1);
				strncpy(DGPS,        buf + k[5] + 1, k[6] - k[5] - 1);
				strncpy(SET_NUM,     buf + k[6] + 1, k[7] - k[6] - 1);
				printf("時刻:%.2f 緯度:%s %s 経度:%s %s 測位モード:%s 受信衛星数:%s\n", atof(MEASURE_utc)+90000,LATITUDE,NS,LONGITUDE,EW,DGPS,SET_NUM);				
			}
    			//printf("out:%s\n", buf);
 			dgps = atoi(DGPS);
			set_num = atoi(SET_NUM);
			if(dgps >1 && set_num > 1)		//測位モードが０以上かつ受信衛生数が１以上のとき
			{					//緯度、経度を計算
				ido(LATITUDE);
        			keido(LONGITUDE);
				encodeXY();
				//file();	
				//send();

				if( FLAG == 0 && dgps == 2){		//FLAGが０かつ測位モードが２以上の時
					offset_X = X;			//その瞬間のXをoffset_X, Yをoffset_Yとおく
					offset_Y = Y;			//つまりスタート地点の座標
					X0       =offset_X;
					Y0	 =offset_Y;
					FLAG = 1;			//FLAGを　１　に変更
			        }
        	        	if(FLAG == 1){
					send();
					if(fabs(X-offset_X)*1.35<2||fabs(Y-offset_Y)*1.35<2|| fabs(X-X0)*1.35 > 1|| fabs(Y-Y0)*1.35 > 1){
					file();	
					X0 = X;
					Y0 = Y;
					}
					

				}
			}
    
        		printf("X座標は%lf  Y座標は%lfです\n",(X-offset_X)*1.35, (Y-offset_Y)*1.35 );
		}

	}
        
	fclose(fp);
	fclose(fp1);
	fclose(fp2);
  	rs_set_attr(fd, &backup_attr);
	rs_close(fd);
}


