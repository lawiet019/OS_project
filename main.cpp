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

    bool isFinished(){
        return !remCPU[remBurst-1];
    }

    void burst(){
        // bool ret = (remCPU.back() == 1);
        remCPU[remBurst-1]--;
        // return ret;
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

    int getCurrentBurstTime(){
        if(remCPU.empty()) return -1;
        return remCPU.back();
    }

};


struct cmp{
    bool operator() (pair<int, Process*> a, pair<int, Process*> b){
        return !((a.first < b.first) || ( a.first == b.first &&  a.second->name <= b.second->name));
    }
};


class Simulator_SJF{
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
    bool firstTimeRunning;
    double lambda;
    double alpha;
    unordered_map<char, int> predictedTau;
    // Process* switchOutCand;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ, readyPQ;
    // deque<Process*> readyQueue;
    unordered_set<char> preemptedProcesses;


    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator_SJF(int _n, int _s, int _cs, double _alpha, double _lambda, vector<Process*> &_processes){
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
        lambda = _lambda;
        alpha = _alpha;
        // switchOutCand = NULL;
        firstTimeRunning = false;
        for(auto p: processes){
            predictedTau[p->name] = ceil(1 / lambda);
        }
    }


    string printReadyQueue(){
        if(readyPQ.empty()) return "[Q <empty>]";
        string ret = "[Q";
        // queue<Process*> tmpQ;
        int size = readyPQ.size();
        vector<pair<int, Process*>> tmp;
        for(int s=0; s<size; s++){
            auto [burstTime, cur] = readyPQ.top();
            tmp.push_back({burstTime, cur});
            readyPQ.pop();
            ret += " ";
            ret += cur->name;
            // ret += ":";
            // ret += to_string(burstTime);
            // ret += (" " + cur->name);
        }
        ret += "]";
        for(auto p : tmp){
            readyPQ.push(p);
        }
        return ret;
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
            cout<<"Process "<< p->name <<" [NEW] (arrival time "<< p->arriveTime <<" ms) "<< p->totBurst <<" CPU burst" << ( p->totBurst == 1? "":"s") <<" (tau "<< predictedTau[p->name] <<"ms)" <<endl;
        }
    }

    bool processFinishBurst(){
        // Process* ret = NULL;
        if(cpuStatus == RUNNING && curProcess && curProcess->isFinished()){
            int nextReady = curProcess->getNextReady(curTime);
            int preTau = predictedTau[curProcess->name];
            predictedTau[curProcess->name] = ceil( alpha * curProcessRunningTime + (1 - alpha) * predictedTau[curProcess->name] );
            if(nextReady == -1) {
                finished ++;
                cout<<"time "<<curTime<<"ms: Process "<<curProcess->name <<" terminated "<< printReadyQueue() <<endl;
                
            }else{
                ioPQ.push({nextReady + csTime, curProcess});
                preemptedProcesses.erase(curProcess->name);
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << preTau << "ms)"  <<" completed a CPU burst; "<< curProcess->remBurst <<" burst"<< (curProcess->remBurst == 1? "" : "s") <<" to go "<<printReadyQueue()<<endl;
                cout<<"time "<< curTime <<"ms: Recalculated tau = "<< predictedTau[curProcess->name] <<"ms for process " << curProcess->name<< " " << printReadyQueue() <<endl;
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" switching out of CPU; will block on I/O until time "<< nextReady + csTime <<"ms "<< printReadyQueue() <<endl;
            }
            
            curProcessRunningTime = 0;
            cpuStatus = CONTEXT_SWITCH_DOWN;
            preemptedProcesses.erase(curProcess->name);
            return true;
        }
        return false;

    }

    
    void updatReadyQueueFromIOPQ(){
        while(!ioPQ.empty() && ioPQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto newArrival = ioPQ.top().second;
            auto burstTime = newArrival->getCurrentBurstTime();
            if(burstTime != -1){
                int tau = predictedTau[newArrival->name];
                readyPQ.push({tau, newArrival});
                cout<<"time "<< curTime <<"ms: Process "<< newArrival->name << " (tau " << predictedTau[newArrival->name] << "ms) "  <<"completed I/O; added to ready queue "<< printReadyQueue() <<endl;
            }
            
            ioPQ.pop();
            
        }
    }

    void updatReadyQueueFromArrivePQ(){
        while(!arrivePQ.empty() && arrivePQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto newArrival = arrivePQ.top().second;
            auto burstTime = newArrival->getCurrentBurstTime();
            if(burstTime != -1){
                int tau = predictedTau[newArrival->name];
                readyPQ.push({tau, newArrival});
                cout<<"time "<< curTime <<"ms: Process "<< newArrival->name<< " (tau " << predictedTau[newArrival->name] << "ms) " <<"arrived; added to ready queue " << printReadyQueue()<<endl;
            }
            arrivePQ.pop();
        }
    }
    
    void prepareRunningProcess(){
        if(cpuStatus == CONTEXT_SWITCH_UP && remCSTime == 0){
            curProcess = readyPQ.top().second;
            readyPQ.pop();
            
            if(!preemptedProcesses.count(curProcess->name)){
            // cout<<curProcess->remCPU.size()<<endl;
                // preemptedProcesses.insert(curProcess->name);
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name<< " (tau " << predictedTau[curProcess->name] << "ms) "  <<"started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
            }else{

                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << predictedTau[curProcess->name] << "ms) " <<"started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
            }
        }
    }


    void updateReadyQueue(){
        updatReadyQueueFromIOPQ();
        updatReadyQueueFromArrivePQ();


    }


    void cpuStatusSwitch(bool finishRunning){
        if(cpuStatus == RUNNING){
            if(finishRunning) {
                curProcess = NULL;
                cpuStatus = CONTEXT_SWITCH_DOWN;
            }
        }else if(cpuStatus == CONTEXT_SWITCH_UP){
            if(!remCSTime){
                remCSTime = csTime;
                cpuStatus = RUNNING;
            }
        }else if(cpuStatus == IDLE){
            if(!readyPQ.empty()){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
        }else if(cpuStatus == CONTEXT_SWITCH_DOWN){
            if(!remCSTime){
                remCSTime = csTime;
                if(readyPQ.empty()){
                    cpuStatus = IDLE;
                }else{
                    cpuStatus = CONTEXT_SWITCH_UP;
                }
            }
        }
    }

    void cpuRun1ms(){
        
        if(cpuStatus == RUNNING){
            
           curProcess->burst();
        }else if(cpuStatus == CONTEXT_SWITCH_UP){
           remCSTime--;
        }else if(cpuStatus == CONTEXT_SWITCH_DOWN){
            remCSTime--;
        }
    }

    void incrementProcessRunningTime(){
        if(cpuStatus == RUNNING)
            curProcessRunningTime++;
    }

    void runShortestJobFirst(){
        resetAll();
        addArrivePQ();
        while (!isFinished()){
            
            bool finishedBurst = processFinishBurst();
            // bool preemptBurst = rrPreemptSwitchDown();
            
            updateReadyQueue();
            prepareRunningProcess();
            cpuStatusSwitch(finishedBurst);
            
            curTime ++;
            // cout<<curTime<<" "<<curProcess<<endl;
            // prepareRunningProcess();
            
            cpuRun1ms();
            incrementProcessRunningTime();
            
        }
    }
};




int getFuckingStupidRandomNumber(int upperbound, double lambda){

    double x = -log( drand48() ) / lambda;
    int ret = (int) x;
    if(ret < upperbound) {
        return ret;
    }
    return getFuckingStupidRandomNumber(upperbound, lambda);
}

int main(int argc, char ** argv){

    int seed = atoi(argv[1]);
    double lambda = atof(argv[2]);
    int upperbound = atoi(argv[3]);
    int nProcess = atoi(argv[4]);
    int contextSwitchTime = atoi(argv[5]);
    double alpha = atof(argv[6]);
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
        _cpu.push_back(getFuckingStupidRandomNumber(upperbound, lambda) + 1);
        processes.push_back(new Process(i+'A', nBurst, arriveTime, _cpu, _io));
        // auto p = new Process()
    }
    // for(auto p: processes){
    //     p->displayProcessInfo();
    // }

    Simulator_SJF S(nProcess, sliceTime, contextSwitchTime/2, alpha, lambda, processes);
    S.runShortestJobFirst();

    return 0;

}
