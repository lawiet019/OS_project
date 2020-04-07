#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <deque>
#include <tuple>
#include <map>
#include <stack>
#include <set>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
// #include<bits/stdc++.h> 
using namespace std;

#define START 0
#define IDLE 1
#define CONTEXT_SWITCH_DOWN 2
#define CONTEXT_SWITCH_UP 3
#define RUNNING 4

#define BEGINNING 1
#define END 2

class Process{
public:
    char name;
    int remBurst, totBurst;
    int arriveTime;
    vector<int>  CPUBurstTime, IOBurstTime;
    vector<int> remCPU, remIO;
    Process(char _name, int _nBurst, int _arriveTime , vector<int> &_cpu, vector<int> &_io){
        name = _name;
        totBurst = _nBurst;
        arriveTime = _arriveTime;
        for(auto n: _cpu){
            CPUBurstTime.push_back(n);
        }
        for(auto n: _io){
            IOBurstTime.push_back(n);
        }
        reverse(CPUBurstTime.begin(), CPUBurstTime.end());
        reverse(IOBurstTime.begin(), IOBurstTime.end());
    }

    void reset(){
        remBurst = totBurst;
        for(auto n: CPUBurstTime){
            remCPU.push_back(n);
        }
        for(auto n: IOBurstTime){
            remIO.push_back(n);
        }
    }

    void completeOneBurst(){
        remCPU.pop_back();
        remBurst--;
    }

    void completeOneIO(){
        remIO.pop_back();
    }

    bool burst(){
        bool ret = (remCPU.back() == 1);
        remCPU[remBurst-1]--;
        return ret;
    }

    int getNextReady(int curTime){
        completeOneBurst();
        if(remIO.empty()) return -1;
        int ret = curTime + remIO.back();
        completeOneIO();
        return ret;
    }

    void displayProcessInfo(){
        cout<<name<<" "<<totBurst<<" "<<arriveTime<<endl;
        for(auto i: CPUBurstTime){
            cout<<i<<" ";
        }
        cout<<endl;
        for(auto i: IOBurstTime){
            cout<<i<<" ";
        }
        cout<<endl;
    }


};


struct cmp{
    bool operator() (pair<int, Process*> a, pair<int, Process*> b){
        return (a.first < b.first) || ( a.first == b.first &&  a.second->name <= b.second->name);
    }
};

// void runRoundRobin(vector<Process*> &processes){
//     resetAllProcesses(processes);
//     priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ;
//     queue<Process*> readyQueue;
//     int time = 0;
//     bool contextSwitch = false;
//     int contextSwitchRemTime = 0;
//     while(!isFinished(readyQueue, arrivePQ, ioPQ)){
        
        




//     }




// }


class Simulator{
public:
    vector<Process*> processes;
    int sliceTime, curSliceTime;
    int nProcess;
    int cpuStatus; // 0 start 1 idle 2 context switch 3 run
    Process* curProcess;
    int csTime, remCSTime;
    int curTime;
    int curProcessRunningTime;
    int rrPos;
    int finished;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ;
    deque<Process*> readyQueue;
    
    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator(int _n, int _s, int _cs, vector<Process*> &_processes){
        sliceTime = _s;
        nProcess = _n;
        cpuStatus = IDLE;
        curProcess = NULL;
        csTime = _cs;
        remCSTime = csTime;
        curTime = 0;
        curProcessRunningTime = 0;
        processes = _processes;
        rrPos = END;
        finished = 0;
    }

    void resetAll(){
        for(auto p: processes){
            p->reset();
        }
        arrivePQ.empty();
        ioPQ.empty();
    }

    void addArrivePQ(){
        for(auto p : processes){
            arrivePQ.push({p->arriveTime, p});
            cout<<"Process "<< p->name <<" [NEW] (arrival time "<< p->arriveTime <<" ms) "<< p->totBurst <<" CPU bursts"<<endl;
        }
    }
    void updateReadyQueue(){
        while(!arrivePQ.empty() && arrivePQ.top().first == curTime){
            if(rrPos == BEGINNING){
                readyQueue.push_front(arrivePQ.top().second);
            }else if(rrPos == END){
                readyQueue.push_back(arrivePQ.top().second);
            }
            arrivePQ.pop();
            cout<<"time "<< curTime <<"ms: Process "<< readyQueue.front()->name <<" arrived; added to ready queue " << printReadyQueue()<<endl;
        }
        while(!ioPQ.empty() && ioPQ.top().first == curTime){
            if(rrPos == BEGINNING){
                readyQueue.push_front(arrivePQ.top().second);
            }else if(rrPos == END){
                readyQueue.push_back(arrivePQ.top().second);
            }
            arrivePQ.pop();
        }
    }

    string printReadyQueue(){
        if(readyQueue.empty()) return "[Q <empty>]";
        string ret = "[Q";
        // queue<Process*> tmpQ;
        int size = readyQueue.size();
        for(int s=0; s<size; s++){
            auto cur = readyQueue.front();
            readyQueue.pop_front();
            readyQueue.push_back(cur);
            ret += (" " + cur->name);
        }
        return ret;
    }
    
    void cpuWork1ms(){
        if(cpuStatus == CONTEXT_SWITCH_DOWN){
            remCSTime --;
            if(!remCSTime){
                remCSTime = csTime;
                if(readyQueue.empty()) {
                    cpuStatus = IDLE;
                }else{
                    cpuStatus = CONTEXT_SWITCH_UP;
                }
            }
        }else if(cpuStatus == CONTEXT_SWITCH_UP){
            remCSTime --;
            if(!remCSTime){
                remCSTime = csTime;
                cpuStatus = RUNNING;
                curProcess = readyQueue.front();
                readyQueue.pop_front();
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
            }
        }else if(cpuStatus == RUNNING){
            bool isFinished = curProcess->burst();
            if(isFinished){
                int nextReady = curProcess->getNextReady(curTime);
                if(nextReady == -1) {
                    finished ++;
                    cout<<"time "<<curTime<<"ms: Process "<<curProcess->name<<" terminated "<< printReadyQueue() <<endl;
                }else{
                    ioPQ.push({nextReady, curProcess});
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" completed a CPU burst; "<< curProcess->remBurst <<" bursts to go "<<printReadyQueue()<<endl;
                }
                cpuStatus = CONTEXT_SWITCH_DOWN;
            }
        }else if(cpuStatus == IDLE){
            if(!readyQueue.empty()){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
        }
    }

    void rrSwitch(){
        curProcessRunningTime = 0;
        if(readyQueue.empty()) {
            return;
        }
        cpuStatus = CONTEXT_SWITCH_DOWN;
        readyQueue.push_back(curProcess);
        cout<<"time "<< curTime <<"ms: Time slice expired; process "<<curProcess->name<<" preempted with 80ms to go "<<printReadyQueue()<<endl;
    }


    void runRoundRobin(){
        resetAll();
        addArrivePQ();
        while (!isFinished()){
            
            updateReadyQueue();
            // cout<<curTime<<endl;
            if(cpuStatus == RUNNING) curProcessRunningTime++;
            cpuWork1ms();
            if(cpuStatus == RUNNING && curProcessRunningTime == sliceTime) rrSwitch();
            if(cpuStatus != RUNNING) curProcessRunningTime = 0;
            curTime++;
        }

    }



};

int getFuckingStupidRandomNumber(int upperbound, double lambda){

    double x = -log( drand48() ) / lambda;
    int ret = (int) x;
    if(ret <= upperbound) return ret;
    return getFuckingStupidRandomNumber(upperbound, lambda);
}

int main(int argc, char ** argv){

    int seed = atoi(argv[1]);
    double lambda = atof(argv[2]);
    int upperbound = atoi(argv[3]);
    int nProcess = atoi(argv[4]);
    int contextSwitchTime = atoi(argv[5]);
    double tau = atof(argv[6]);
    int sliceTime = atoi(argv[7]);
    int rrPos = END;
    if(argc == 9){
        if(!strcmp(argv[8], "BEGINNING")){
            rrPos = BEGINNING;
        }else if(!strcmp(argv[8], "END")){
            rrPos = END;
        }
    }
    srand48(seed);
    vector<Process*> processes;
    for(int i=0; i<nProcess; i++){
        
        int arriveTime = getFuckingStupidRandomNumber(upperbound, lambda);
        int nBurst = (int) (drand48() * 100 + 1);
        vector<int> _cpu, _io;
        for(int i=0; i<nBurst-1; i++){
            _cpu.push_back(getFuckingStupidRandomNumber(upperbound, lambda) + 1);
            _io.push_back(getFuckingStupidRandomNumber(upperbound, lambda) +1 );
        }
        _cpu.push_back(getFuckingStupidRandomNumber(upperbound, lambda));
        processes.push_back(new Process(i+'A', nBurst, arriveTime, _cpu, _io));
        // auto p = new Process()
    }
    // for(auto p: processes){
    //     p->displayProcessInfo();
    // }
    // vector<int> 
    Simulator S(nProcess, sliceTime, contextSwitchTime/2, processes);
    S.runRoundRobin();

    return 0;

}