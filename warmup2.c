//
//  warmup2.c
//  warmup2
//
//  Created by Xiheng Yue on 6/16/15.
//  Copyright (c) 2015 Xiheng Yue. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "my402list.h"
#include "warmup2.h"

double PrintReturnCurTime() {
    struct timeval timeVal;
    gettimeofday(&timeVal, 0);
    double curTime = timeVal.tv_sec * 1000.0 + timeVal.tv_usec / 1000.0 - initTimeVal;
    printf("%012.3fms: ", curTime);
    return curTime;
}

void EmptyList(My402List *list, int QNum) {
    while (!My402ListEmpty(list)) {
        Packet *packet = My402ListFirst(list)->obj;
        My402ListUnlink(list, My402ListFirst(list));
        PrintReturnCurTime();
        printf("p%d removed from Q%d\n", packet->packetNum, QNum);
        removedPacketCount++;
        free(packet);
    }
}

void *HandlerThread(void *arg) {
    int signal;
    sigwait(&set, &signal);
    pthread_mutex_lock(&m);
    printf("\n");

    stopPacket = 1;
    
    pthread_cancel(packetThread);
    pthread_cancel(tokenThread);
    EmptyList(Q1, 1);
    EmptyList(Q2, 2);
    pthread_cancel(serverThread);

    pthread_mutex_unlock(&m);

    free(Q1);
    free(Q2);
    
    return 0;
}

Packet *CreatePacket(int packetNum) {
    Packet *packet = (Packet *)malloc(sizeof(Packet));
    if (mode == 0) {
        packet->lambda = lambda;
        packet->mu = mu;
        packet->P = P;
    } else {
        char buf[1026];
        if (fgets(buf, sizeof(buf), fd)) {
            char* tmp;
            tmp = strtok(buf, " \n\t");
            packet->lambda = 1 / (atof(tmp) / 1000.0);
            tmp = strtok(NULL, " \n\t");
            packet->P = atoi(tmp);
            tmp = strtok(NULL, " \n\t");
            packet->mu = 1 / (atof(tmp) / 1000.0);
        }
    }
    packet->Q1ArrivalTime = 0.0;
    packet->Q1LeaveTime = 0.0;
    packet->Q2ArrivalTime = 0.0;
    packet->Q2LeaveTime = 0.0;
    packet->packetNum = packetNum;
    return packet;
}

void MovePacket(int src) {
    if (src == 1) {
        Packet *packet = My402ListFirst(Q1)->obj;
        My402ListUnlink(Q1, My402ListFirst(Q1));
        packet->Q1LeaveTime = PrintReturnCurTime();
        printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n", packet->packetNum, packet->Q1LeaveTime - packet->Q1ArrivalTime, curTokenCount);
        My402ListAppend(Q2, packet);
        packet->Q2ArrivalTime = PrintReturnCurTime();
        printf("p%d enters Q2\n", packet->packetNum);
        if (My402ListLength(Q2)) {
            pthread_cond_signal(&cv);
        }
    } else {
        Packet *packet = My402ListFirst(Q2)->obj;
        My402ListUnlink(Q2, My402ListFirst(Q2));
        packet->Q2LeaveTime = PrintReturnCurTime();
        printf("p%d leaves Q2, time in Q2 = %.3fms\n", packet->packetNum, packet->Q2LeaveTime - packet->Q2ArrivalTime);
    }
}

void *PacketThread(void *arg) {
    double preArrivalTime = 0;
    while (packetCount++ < n && !stopPacket) {
        Packet *packet = CreatePacket(packetCount);
        //usleep用的是microsecond
        double sleepTime = 1000000.0 / packet->lambda;
        usleep(sleepTime);
        packet->Q1ArrivalTime = PrintReturnCurTime();
        double interArrivalTime = packet->Q1ArrivalTime - preArrivalTime;
        preArrivalTime = packet->Q1ArrivalTime;
        totalInterArrivalTime += interArrivalTime;
        printf("p%d arrives, needs %d tokens, inter-arrival time = %.3fms", packet->packetNum, packet->P, interArrivalTime);
        if (packet->P <= B) {
            printf("\n");
            pthread_mutex_lock(&m);
            My402ListAppend(Q1, (void *)packet);
            PrintReturnCurTime();
            printf("p%d enters Q1\n", packet->packetNum);
            if (curTokenCount >= packet->P) {
                curTokenCount -= packet->P;
                MovePacket(1);
            }
            pthread_mutex_unlock(&m);
        } else {
            printf(", dropped\n");
            droppedPacketCount++;
        }
    }
    packetCount--;
    stopPacket = 1;
    //printf("im packet, im done\n");
    return 0;
}

void *TokenThread(void *arg) {
    int tokenNum = 0;
    double preArrivalTime = 0;
    while (!stopPacket || !My402ListEmpty(Q1)) {
        tokenNum++;
        double sleepTime = 1000000.0 / r;
        usleep(sleepTime);
        preArrivalTime = PrintReturnCurTime();
        //lock mutex
        pthread_mutex_lock(&m);
        totalTokenCount++;
        if(curTokenCount < B) {
            curTokenCount++;
            if (curTokenCount == 1) {
                printf("token t%d arrives, token bucket now has %d token\n", tokenNum, curTokenCount);
            } else {
                printf("token t%d arrives, token bucket now has %d tokens\n", tokenNum, curTokenCount);
            }
        } else {
            droppedTokenCount++;
            printf("token t%d arrives, dropped\n", tokenNum);
        }
        if (!My402ListEmpty(Q1)) {
            Packet *packet = My402ListFirst(Q1)->obj;
            if (packet->P <= curTokenCount) {
                curTokenCount -= packet->P;
                MovePacket(1);
            }
        }
        pthread_mutex_unlock(&m);
        if (My402ListLength(Q2)) {
            pthread_cond_signal(&cv);
        }
    }
    //printf("im token, im done\n");
    //printf("%d\n%d\n", My402ListEmpty(Q1), My402ListEmpty(Q2));
    return 0;
}

void *ServerThread(void *arg) {
    while (!stopPacket || !My402ListEmpty(Q1) || !My402ListEmpty(Q2)) {
        pthread_mutex_lock(&m);
        while(My402ListEmpty(Q2)) {
            pthread_cond_wait(&cv, &m);
        }
        Packet *packet = My402ListFirst(Q2)->obj;
        MovePacket(2);
        pthread_mutex_unlock(&m);
        double serviceStartTime = PrintReturnCurTime();
        printf("p%d begins service at S, requesting %gms of service\n", packet->packetNum, 1000.0 / packet->mu);
        //该packet要求睡的时间加上从Q2出来后待的时间
        double sleepTime = 1000000.0 / packet->mu;
        printf("\nrequested sleep time: %g\n\n", sleepTime / 1000.0);
        //sleep是个一个cancellation point, 如果在这里被cancel则相当于该packet的service没有完成
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
        usleep(sleepTime);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
        double serviceEndTime = PrintReturnCurTime();
        printf("p%d departs from S, service time = %.3fms, time in system = %.3fms\n", packet->packetNum, serviceEndTime - serviceStartTime, serviceEndTime - packet->Q1ArrivalTime);
        completedPacketCount++;
        totalQ1Time += packet->Q1LeaveTime - packet->Q1ArrivalTime;
        totalQ2Time += packet->Q2LeaveTime - packet->Q2ArrivalTime;
        totalServiceTime += serviceEndTime - serviceStartTime;
        totalSysTime += serviceEndTime - packet->Q1ArrivalTime;
        totalTimeSqr += pow((serviceEndTime - packet->Q1ArrivalTime) / 1000.0, 2);
        free(packet);
        pthread_testcancel();
    }
    //printf("im server, im done\n");
    return 0;
}

void CreateThreads() {
    struct timeval tmpTime;
    gettimeofday(&tmpTime, 0);
    initTimeVal = tmpTime.tv_sec * 1000.0 + tmpTime.tv_usec / 1000.0;
    printf("%012.3fms: emulation begins\n", 0.0);
    
    //initialize Q1 & Q2
    Q1 = (My402List *)malloc(sizeof(My402List));
    Q2 = (My402List *)malloc(sizeof(My402List));
    My402ListInit(Q1);
    My402ListInit(Q2);
    
    //set sigmask
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    //???感觉用SIG_SETMASK也可以
    pthread_sigmask(SIG_BLOCK, &set, 0);
    
    pthread_create(&handlerThread, NULL, HandlerThread, NULL);
    pthread_create(&packetThread, NULL, PacketThread, NULL);
    pthread_create(&tokenThread, NULL, TokenThread, NULL);
    pthread_create(&serverThread, NULL, ServerThread, NULL);
    
    pthread_join(packetThread, NULL);
    pthread_join(tokenThread, NULL);
    //if (stopPacket && My402ListEmpty(Q1) && My402ListEmpty(Q2)) {
        //pthread_cancel(serverThread);
    //}
    pthread_join(serverThread, NULL);

    totalTime = PrintReturnCurTime();
    printf("emulation ends\n");
    if (mode == 1) {
        fclose(fd);
    }
}
void Usage() {
    fprintf(stderr, "(malformed command)\nusage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
    exit(EXIT_FAILURE);
}
void ParseArgs(int argc, char *argv[]) {
    if (argc % 2 != 1) {
        Usage();
    }
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-lambda") == 0) {
            lambda = atof(argv[i + 1]);
            if (lambda <= 0) {
                Usage();
            }
            if (lambda < 0.1) {
                lambda = 0.1;
            }
            i++;
        } else if (strcmp(argv[i], "-mu") == 0) {
            mu = atof(argv[i + 1]);
            if (mu <= 0) {
                Usage();
            }
            if (mu < 0.1) {
                mu = 0.1;
            }
            i++;
        } else if (strcmp(argv[i], "-r") == 0) {
            r = atof(argv[i + 1]);
            if (r <= 0) {
                Usage();
            }
            if (r < 0.1) {
                r = 0.1;
            }
            i++;
        } else if (strcmp(argv[i], "-B") == 0) {
            B = atoi(argv[i + 1]);
            if (B <= 0 || B > 2147483647) {
                Usage();
            }
            i++;
        } else if (strcmp(argv[i], "-P") == 0) {
            P = atoi(argv[i + 1]);
            if (P <= 0 || P > 2147483647) {
                Usage();
            }
            i++;
        } else if (strcmp(argv[i], "-n") == 0) {
            n = atoi(argv[i + 1]);
            if (n <= 0 || n > 2147483647) {
                Usage();
            }
            i++;
        } else if (strcmp(argv[i], "-t") == 0) {
            mode = 1;
            fileName = argv[i + 1];
            if (!(fd = fopen(fileName, "r"))) {
                fprintf(stderr, "Error: Invalid trace specification(can not open file %s)\n", fileName);
                exit(-1);
            }
            char buf[1026];
            if (fgets(buf, sizeof(buf), fd) == 0) {
                fprintf(stderr, "Error: Invalid trace specification file(input file is not in the right format)\n");
            }
            n = atoi(buf);
            i++;
        } else {
            Usage();
        }
    }
}

void PrintStat() {
    printf("\nStatistics:\n\n");
    if (packetCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        printf("\taverage packet inter-arrival time = %.6g\n", totalInterArrivalTime / packetCount / 1000.0);
    }
    if (packetCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        printf("\taverage packet service time = %.6g\n\n", totalServiceTime / completedPacketCount / 1000.0);
    }
    
    printf("\taverage number of packets in Q1 = %.6g\n", totalQ1Time / totalTime);
    printf("\taverage number of packets in Q2 = %.6g\n", totalQ2Time / totalTime);
    printf("\taverage number of packets at S = %.6g\n\n", totalServiceTime / totalTime);
    
    if (completedPacketCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        printf("\taverage time a packet spent in system = %.6g\n", totalSysTime / completedPacketCount / 1000.0);
    }
    if (completedPacketCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        double var = totalTimeSqr / completedPacketCount - pow(totalSysTime / (1000.0 * completedPacketCount), 2);
        double sd = sqrt(var);
        printf("\tstandard deviation for time spent in system = %.6g\n", sd);
    }
    //to be continued
    printf("\n");
    if (totalTokenCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        printf("\ttoken drop probability = %.6g\n", (double)droppedTokenCount / totalTokenCount);
    }
    if (packetCount == 0) {
        printf("\tN/A no packet arrived at this facility");
    } else {
        printf("\tpacket drop probability = %.6g\n\n", (double)droppedPacketCount / packetCount);
    }
}

void PrintArgs() {
    printf("Emulation Parameters:\n");
    if (mode) {
        printf("\tnumber to arrive = %d\n\tr = %g\n\tB = %d\n\ttsfile = %s\n\n", n, r, B, fileName);
    } else {
        printf("\tnumber to arrive = %d\n\tlambda = %g\n\tmu = %g\n\tr = %g\n\tB = %d\n\tP = %d\n\n", n, lambda, mu, r, B, P);
    }
}

int main(int argc, char *argv[]) {
    ParseArgs(argc, argv);
    PrintArgs();
    CreateThreads();
    PrintStat();
    return 0;
}
