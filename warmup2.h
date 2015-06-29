//
//  warmup2.h
//  warmup2
//
//  Created by Xiheng Yue on 6/16/15.
//  Copyright (c) 2015 Xiheng Yue. All rights reserved.
//

#ifndef __warmup2__warmup2__
#define __warmup2__warmup2__

//others
//packet inter-arrival time = 1 / lambda
double lambda = 0.5;
//service time = 1 / mu
double mu = 0.35;
//token inter-arrival time = 1 / r
double r = 1.5;
//token bucket size
int B = 10;
//packet required tokens
int P = 3;
//total packets count
int n = 20;

int mode = 0;

char *fileName;
FILE *fd;

//flag
int stopPacket = 0;

//Statistics
double totalQ1Time = 0;
double totalQ2Time = 0;
double totalInterArrivalTime = 0;
double totalServiceTime = 0;
double totalSysTime = 0;
double totalTime = 0;
double totalTimeSqr = 0;

//packet
int packetCount = 0;
int droppedPacketCount = 0;
int completedPacketCount = 0;
int removedPacketCount = 0;

//token
int curTokenCount = 0;
int totalTokenCount = 0;
int droppedTokenCount = 0;

//thread related
pthread_t handlerThread, packetThread, tokenThread, serverThread;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

sigset_t set;

double initTimeVal;
My402List *Q1 = NULL;
My402List *Q2 = NULL;

//function declaration
void ParseArgs(int argc, char *argv[]);
void PrintArgs();
double CurProgTime();
void FormatTime13(int time, char *buf);
void FormatTime(int time, char *buf);
void ReadLine();
void handler(int);
void PrintStat();
void InitQ();
void *HandlerThread(void *arg);

typedef struct tagPacket {
    //inter-arrival time 1 / lambda
    double lambda;
    //need 1 / mu service time
    double mu;
    //need P tokens
    int P;
    int packetNum;
    double Q1ArrivalTime;
    double Q1LeaveTime;
    double Q2ArrivalTime;
    double Q2LeaveTime;
} Packet;

#endif /* defined(__warmup2__warmup2__) */
