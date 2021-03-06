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
#include <iomanip>
#include <fstream>
#include <sstream>
// #include<bits/stdc++.h> 
using namespace std;

#define START 0
#define IDLE 1
#define CONTEXT_SWITCH_DOWN 2
#define CONTEXT_SWITCH_UP 3
#define RUNNING 4

#define BEGINNING 1
#define END 2

const bool printall = false;

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

    int getProcessTotalRunningTime(){
        int ret = 0;
        for(auto t: CPUBurstTime){
            ret += t;
        }
        return ret;
    }

    int getProcessTotalIOTime(){
        int ret = 0;
        for(auto t: IOBurstTime){
            ret += t;
        }
        return ret;
    }

};


struct cmp{
    bool operator() (pair<int, Process*> a, pair<int, Process*> b){
        return !((a.first < b.first) || ( a.first == b.first &&  a.second->name <= b.second->name));
    }
};


class Simulator_RR_FCFS{
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
    int totPreemption;
    // Process* switchOutCand;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ, preemptPQ;
    deque<Process*> readyQueue;
    unordered_set<char> preemptedProcesses;
    unordered_map<char, int> contextSwitchCnt;
    unordered_map<char, vector<int>> startTime, endTime;


    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator_RR_FCFS(int _n, int _s, int _cs, vector<Process*> _processes, int _rrPos){
        sliceTime = _s;
        nProcess = _n;
        cpuStatus = IDLE;
        curProcess = NULL;
        csTime = _cs;
        remCSTime = csTime;
        curTime = 0;
        curProcessRunningTime = 0;
        processes = _processes;
        rrPos = _rrPos;
        finished = 0;
        totPreemption = 0;
        // switchOutCand = NULL;
        firstTimeRunning = false;
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
            
            ret += " ";
            ret += cur->name;
            // ret += (" " + cur->name);
        }
        ret += "]";
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
            cout<<"Process "<< p->name <<" [NEW] (arrival time "<< p->arriveTime <<" ms) "<< p->totBurst <<" CPU burst" << ( p->totBurst == 1? "":"s") <<endl;
        }
        if(sliceTime == INT_MAX){
            cout<<"time 0ms: Simulator started for FCFS [Q <empty>]"<<endl;
        }else{
            cout<<"time 0ms: Simulator started for RR [Q <empty>]"<<endl;
        }
    }

    bool processFinishBurst(){
        // Process* ret = NULL;
        if(cpuStatus == RUNNING && curProcess && curProcess->isFinished()){
            int nextReady = curProcess->getNextReady(curTime);
            if(nextReady == -1) {
                finished ++;
                cout<<"time "<<curTime<<"ms: Process "<<curProcess->name<<" terminated "<< printReadyQueue() <<endl;
            }else{
                ioPQ.push({nextReady + csTime, curProcess});
                preemptedProcesses.erase(curProcess->name);
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" completed a CPU burst; "<< curProcess->remBurst <<" burst"<< (curProcess->remBurst == 1? "" : "s") <<" to go "<<printReadyQueue()<<endl;
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" switching out of CPU; will block on I/O until time "<< nextReady + csTime <<"ms "<< printReadyQueue() <<endl;
                }
             }
            endTime[curProcess->name].push_back( curTime);
            curProcessRunningTime = 0;
            cpuStatus = CONTEXT_SWITCH_DOWN;
            preemptedProcesses.erase(curProcess->name);
            return true;
        }
        return false;

    }

    bool checkRRPreempt(){
        // cout<<curTime<<" "<<curProcessRunningTime<<endl;
        return (cpuStatus == RUNNING && curProcessRunningTime == sliceTime);
    }

    bool rrPreemptSwitchDown(){
        bool ret = checkRRPreempt();
        if(!ret) return ret;
        if(readyQueue.empty()){
            if(curTime < 999 || printall){
                cout<<"time "<< curTime <<"ms: Time slice expired; no preemption because ready queue is empty "<< printReadyQueue() <<endl;
            }
            
            return false;
        }
        preemptedProcesses.insert(curProcess->name);
        preemptPQ.push({curTime + csTime, curProcess});
        cpuStatus = CONTEXT_SWITCH_DOWN;
        curProcessRunningTime = 0;
        if(curTime < 999 || printall){
            cout<<"time "<< curTime <<"ms: Time slice expired; process "<<curProcess->name<<" preempted with "<< curProcess->remCPU.back() <<"ms to go "<<printReadyQueue()<<endl;   
        }
        totPreemption ++;
        return ret;
    }
    
    void updatReadyQueueFromIOPQ(){
        while(!ioPQ.empty() && ioPQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto newArrival = ioPQ.top().second;
            if(rrPos == BEGINNING){
                readyQueue.push_front(ioPQ.top().second);
            }else if(rrPos == END){
                readyQueue.push_back(ioPQ.top().second);
            }
            ioPQ.pop();
            startTime[newArrival->name].push_back( curTime);
            if(curTime < 999 || printall){
                cout<<"time "<< curTime <<"ms: Process "<< newArrival->name <<" completed I/O; added to ready queue "<< printReadyQueue() <<endl;
            }
        }
    }

    void updatReadyQueueFromArrivePQ(){
        while(!arrivePQ.empty() && arrivePQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto newArrival = arrivePQ.top().second;
            if(rrPos == BEGINNING){
                readyQueue.push_front(arrivePQ.top().second);
            }else if(rrPos == END){
                readyQueue.push_back(arrivePQ.top().second);
            }
            arrivePQ.pop();
            startTime[newArrival->name].push_back( curTime);
            if(curTime < 999 || printall){
                cout<<"time "<< curTime <<"ms: Process "<< newArrival->name <<" arrived; added to ready queue " << printReadyQueue()<<endl;
            }
        }
    }
    
    void updatReadyQueueFromPreemptPQ(){
        while(!preemptPQ.empty() && preemptPQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            // auto newArrival = preemptPQ.top().second;
            if(rrPos == BEGINNING){
                readyQueue.push_front(preemptPQ.top().second);
            }else if(rrPos == END){
                readyQueue.push_back(preemptPQ.top().second);
            }
            preemptPQ.pop();
            // cout<<"time "<< curTime <<"ms: Process "<< newArrival->name <<" survived from preempt; added to ready queue " << printReadyQueue()<<endl;
        }
    }

    void updateReadyQueue(){
        updatReadyQueueFromPreemptPQ();
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
            if(!readyQueue.empty()){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
        }else if(cpuStatus == CONTEXT_SWITCH_DOWN){
            if(!remCSTime){
                remCSTime = csTime;
                if(readyQueue.empty()){
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

    void prepareRunningProcess(){
        if(cpuStatus == CONTEXT_SWITCH_UP && remCSTime == 0){
            curProcess = readyQueue.front();
            readyQueue.pop_front();
            if(!preemptedProcesses.count(curProcess->name)){
            // cout<<curProcess->remCPU.size()<<endl;
                // preemptedProcesses.insert(curProcess->name);
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
                }
            }else{
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
                }
            }
            contextSwitchCnt[curProcess->name] ++;
        }
    }


        string getStatRes(){
        string ret = "";
        unordered_map<char, int> cpuBurstTime, processWaitTime, processTurnAroundTime, ioBurstTime;
        int totCPUBursts = 0, totIOBursts = 0;
        for(auto p: processes){
            cpuBurstTime[p->name] = p->getProcessTotalRunningTime();
            ioBurstTime[p->name] = p->getProcessTotalIOTime();
            totCPUBursts += p->totBurst;
            totIOBursts += p->totBurst - 1;
            for(int i=0;  i<(int)endTime[p->name].size(); i++){
                processTurnAroundTime[p->name] += endTime[p->name][i] - startTime[p->name][i] + csTime;
                
            }
            
            processWaitTime[p->name] = processTurnAroundTime[p->name]  - cpuBurstTime[p->name];
        }
        int totBurstTime = 0, totWaitTime = 0, totTurnAroundTime = 0, totCS = 0;
        for(auto p: processes){
            char n = p->name;
            totBurstTime += cpuBurstTime[n];
            totWaitTime += processWaitTime[n];
            totTurnAroundTime += processTurnAroundTime[n];
            totCS += contextSwitchCnt[n];
        }
        totWaitTime -= (2*csTime*totCPUBursts + totPreemption * 2 * csTime);
        // cout<<totCPUBursts<<" "<<totIOBursts<<endl;
        double avgBurstTime = (double)totBurstTime / (double)totCPUBursts ;
        double avgWaitTime = (double)totWaitTime / (double)totCPUBursts;
        double avgTurnAroundTime = (double)totTurnAroundTime / (double)totCPUBursts ;
        // string strBurstTime = to_string(avgBurstTime);
        // string strWaitTime = to_string(avgWaitTime);
        // string strTurnAroundTime = to_string(avgTurnAroundTime);
        stringstream buffer;

        if(sliceTime == INT_MAX){
            ret += "Algorithm FCFS\n";   
        }else{
            ret += "Algorithm RR\n";
        }
        
        buffer << std::fixed << setprecision(3) << avgBurstTime;
        ret += "-- average CPU burst time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgWaitTime;
        ret += "-- average wait time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgTurnAroundTime;
        ret += "-- average turnaround time: " +  buffer.str() + " ms\n";
        ret += "-- total number of context switches: " +  to_string(totCS) + "\n";
        ret += "-- total number of preemptions: " +  to_string(totPreemption) + "\n";
        // cout<< ret<<endl;
        return ret;

    }


    void runRoundRobin(){
        resetAll();
        addArrivePQ();
        while (!isFinished()){
            
            bool finishedBurst = processFinishBurst();
            bool preemptBurst = rrPreemptSwitchDown();
            
            updateReadyQueue();
            prepareRunningProcess();
            cpuStatusSwitch(finishedBurst || preemptBurst);
            
            curTime ++;
            // cout<<curTime<<" "<<curProcess<<endl;
            // prepareRunningProcess();
            
            cpuRun1ms();
            incrementProcessRunningTime();
        }
        if(sliceTime == INT_MAX){
            cout<<"time "<< curTime+1 <<"ms: Simulator ended for FCFS [Q <empty>]"<<endl;
        }else{
            cout<<"time "<< curTime+1 <<"ms: Simulator ended for RR [Q <empty>]"<<endl;
        }
    }

    void runFirstComeFirstServe(){
        sliceTime = INT_MAX;
        runRoundRobin();
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
    int totPreemption;
    unordered_map<char, int> predictedTau;
    // Process* switchOutCand;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ, readyPQ;
    // deque<Process*> readyQueue;
    unordered_set<char> preemptedProcesses;
    unordered_map<char, int> contextSwitchCnt;
    unordered_map<char, vector<int>> startTime, endTime;

    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator_SJF(int _n, int _s, int _cs, double _alpha, double _lambda, vector<Process*> _processes){
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
        totPreemption = 0;
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
            auto burstTime = readyPQ.top().first;
            auto cur = readyPQ.top().second;
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
        cout<<"time 0ms: Simulator started for SJF [Q <empty>]"<<endl;
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << preTau << "ms)"  <<" completed a CPU burst; "<< curProcess->remBurst <<" burst"<< (curProcess->remBurst == 1? "" : "s") <<" to go "<<printReadyQueue()<<endl;
                    cout<<"time "<< curTime <<"ms: Recalculated tau = "<< predictedTau[curProcess->name] <<"ms for process " << curProcess->name<< " " << printReadyQueue() <<endl;
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" switching out of CPU; will block on I/O until time "<< nextReady + csTime <<"ms "<< printReadyQueue() <<endl;
                }
            }
            endTime[curProcess->name].push_back( curTime);
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< newArrival->name << " (tau " << predictedTau[newArrival->name] << "ms) "  <<"completed I/O; added to ready queue "<< printReadyQueue() <<endl;
                }
            }
            startTime[newArrival->name].push_back( curTime);
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< newArrival->name<< " (tau " << predictedTau[newArrival->name] << "ms) " <<"arrived; added to ready queue " << printReadyQueue()<<endl;
                }
            }
            startTime[newArrival->name].push_back( curTime);
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name<< " (tau " << predictedTau[curProcess->name] << "ms) "  <<"started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
                }
            }else{
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << predictedTau[curProcess->name] << "ms) " <<"started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
                }
            }
            contextSwitchCnt[curProcess->name] ++;
            
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

    string getStatRes(){
        string ret = "";
        unordered_map<char, int> cpuBurstTime, processWaitTime, processTurnAroundTime, ioBurstTime;
        int totCPUBursts = 0, totIOBursts = 0;
        for(auto p: processes){
            cpuBurstTime[p->name] = p->getProcessTotalRunningTime();
            ioBurstTime[p->name] = p->getProcessTotalIOTime();
            totCPUBursts += p->totBurst;
            totIOBursts += p->totBurst - 1;
            for(int i=0;  i<(int)endTime[p->name].size(); i++){
                processTurnAroundTime[p->name] += endTime[p->name][i] - startTime[p->name][i] + csTime;
                
            }
            
            processWaitTime[p->name] = processTurnAroundTime[p->name]  - cpuBurstTime[p->name];
        }
        int totBurstTime = 0, totWaitTime = 0, totTurnAroundTime = 0, totCS = 0;
        for(auto p: processes){
            char n = p->name;
            totBurstTime += cpuBurstTime[n];
            totWaitTime += processWaitTime[n];
            totTurnAroundTime += processTurnAroundTime[n];
            totCS += contextSwitchCnt[n];
        }
        totWaitTime -= 2*csTime*totCPUBursts;
        // cout<<totCPUBursts<<" "<<totIOBursts<<endl;
        double avgBurstTime = (double)totBurstTime / (double)totCPUBursts ;
        double avgWaitTime = (double)totWaitTime / (double)totCPUBursts;
        double avgTurnAroundTime = (double)totTurnAroundTime / (double)totCPUBursts ;
        // string strBurstTime = to_string(avgBurstTime);
        // string strWaitTime = to_string(avgWaitTime);
        // string strTurnAroundTime = to_string(avgTurnAroundTime);
        stringstream buffer;

        
        ret += "Algorithm SJF\n";
        buffer << std::fixed << setprecision(3) << avgBurstTime;
        ret += "-- average CPU burst time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgWaitTime;
        ret += "-- average wait time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgTurnAroundTime;
        ret += "-- average turnaround time: " +  buffer.str() + " ms\n";
        ret += "-- total number of context switches: " +  to_string(totCS) + "\n";
        ret += "-- total number of preemptions: " +  to_string(totPreemption) + "\n";
        // cout<< ret<<endl;
        return ret;

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
        cout<<"time "<< curTime+1 <<"ms: Simulator ended for SJF [Q <empty>]"<<endl;

    }
};


class Simulator_SRF{
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
    int totPreemption;
    unordered_map<char, int> predictedTau;
    // Process* switchOutCand;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> preemptPQ, arrivePQ, ioPQ, readyPQ;
    // deque<Process*> readyQueue;
    unordered_set<char> preemptedProcesses;
    unordered_map<char, int> contextSwitchCnt;
    unordered_map<char, vector<int>> startTime, endTime;
    unordered_map<char, int> curTotBurst;

    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator_SRF(int _n, int _s, int _cs, double _alpha, double _lambda, vector<Process*> _processes){
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
        totPreemption = 0;
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
            auto burstTime = readyPQ.top().first;
            auto cur = readyPQ.top().second;
            tmp.push_back({burstTime, cur});
            readyPQ.pop();
            ret += " ";
            ret += cur->name;
            // ret += ":";
            // ret += to_string(burstTime);
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
        cout<<"time 0ms: Simulator started for SRT [Q <empty>]"<<endl;
    }

    bool processFinishBurst(){
        // Process* ret = NULL;
        if(cpuStatus == RUNNING && curProcess && curProcess->isFinished()){
            int nextReady = curProcess->getNextReady(curTime);
            int preTau = predictedTau[curProcess->name];
            // int cpuBurstIdx = curProcess->remCPU.size() - 1;
            // curTotBurst[curProcess->name] += curProcessRunningTime;
            // cout<<curTotBurst[curProcess->name]<<" "<<predictedTau[curProcess->name]<<endl;
            predictedTau[curProcess->name] = ceil( alpha * curTotBurst[curProcess->name] + (1 - alpha) * predictedTau[curProcess->name] );
            
            if(nextReady == -1) {
                finished ++;
                cout<<"time "<<curTime<<"ms: Process "<<curProcess->name <<" terminated "<< printReadyQueue() <<endl;
                
            }else{
                ioPQ.push({nextReady + csTime, curProcess});
                preemptedProcesses.erase(curProcess->name);
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << preTau << "ms)"  <<" completed a CPU burst; "<< curProcess->remBurst <<" burst"<< (curProcess->remBurst == 1? "" : "s") <<" to go "<<printReadyQueue()<<endl;
                    cout<<"time "<< curTime <<"ms: Recalculated tau = "<< predictedTau[curProcess->name] <<"ms for process " << curProcess->name<< " " << printReadyQueue() <<endl;
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" switching out of CPU; will block on I/O until time "<< nextReady + csTime <<"ms "<< printReadyQueue() <<endl;
                }
            }
            endTime[curProcess->name].push_back( curTime);
            curProcessRunningTime = 0;
            cpuStatus = CONTEXT_SWITCH_DOWN;
            preemptedProcesses.erase(curProcess->name);
            curTotBurst[curProcess->name] = 0;
            return true;
        }
        return false;

    }

    bool checkSRFPreempt(){
        // cout<<curTime<<" "<<curProcessRunningTime<<endl;
        return (cpuStatus == RUNNING && !readyPQ.empty() && ((readyPQ.top().first < predictedTau[curProcess->name] - curTotBurst[curProcess->name]) || (readyPQ.top().first == predictedTau[curProcess->name] - curTotBurst[curProcess->name] &&  readyPQ.top().second->name > curProcess->name )  ));
    }

    bool srfPreemptSwitchDown(){
        bool ret = checkSRFPreempt();
        if(!ret) return ret;
        int curRemTime = predictedTau[curProcess->name] - curTotBurst[curProcess->name];
        preemptedProcesses.insert(curProcess->name);
        
        
        // cout<<predictedTau[curProcess->name]<<" "<<curTotBurst[curProcess->name]<<" "<<curRemTime<<endl;
        preemptPQ.push({curRemTime, curProcess});
        // cout<<preemptPQ.top().first<<" "<<preemptPQ.top().second->name<<endl;
        cpuStatus = CONTEXT_SWITCH_DOWN;

        Process* preemptingProcess = readyPQ.top().second;
        // cout<<"time "<< curTime <<"ms: Time slice expired; process "<<curProcess->name<<" preempted with "<< curProcess->remCPU.back() <<"ms to go "<<printReadyQueue()<<endl;   
        if(curTime < 999 || printall){
            cout<<"time "<< curTime <<"ms: Process "<< preemptingProcess->name <<" (tau "<< predictedTau[preemptingProcess->name] <<"ms) completed I/O; preempting "<< curProcess->name <<" "<<printReadyQueue()<<endl;
        }
        totPreemption ++;
        // curTotBurst[curProcess->name] += curProcessRunningTime;
        curProcessRunningTime = 0;
        return ret;
    }

    void updatReadyQueueFromPreemptyPQ(){
        while(!preemptPQ.empty()){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto tau = preemptPQ.top().first;
            auto newArrival = preemptPQ.top().second;
            // cout<<"asdfasdf  "<<newArrival->name<<endl;
            auto burstTime = newArrival->getCurrentBurstTime();
            if(burstTime != -1){
                
                readyPQ.push({tau, newArrival});
                // cout<<"time "<< curTime <<"ms: Process "<< newArrival->name << " (tau " << predictedTau[newArrival->name] << "ms) "  <<"completed I/O; added to ready queue "<< printReadyQueue() <<endl;
            }
            // startTime[newArrival->name].push_back( curTime);
            preemptPQ.pop();
            
        }
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< newArrival->name << " (tau " << predictedTau[newArrival->name] << "ms) "  <<"completed I/O; added to ready queue "<< printReadyQueue() <<endl;
                }
            }
            startTime[newArrival->name].push_back( curTime);
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
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< newArrival->name<< " (tau " << predictedTau[newArrival->name] << "ms) " <<"arrived; added to ready queue " << printReadyQueue()<<endl;
                }
            }
            startTime[newArrival->name].push_back( curTime);
            arrivePQ.pop();
        }
    }
    
    // void updatReadyQueueFromPreemptPQ(){
    //     while(!preemptPQ.empty() && preemptPQ.top().first == curTime){
    //         if(cpuStatus == IDLE){
    //             cpuStatus = CONTEXT_SWITCH_UP;
    //         }
    //         auto newArrival = preemptPQ.top().second;
    //         auto burstTime = newArrival->getCurrentBurstTime();
    //         if(burstTime != -1){
    //             int tau = predictedTau[newArrival->name];
    //             readyPQ.push({tau, newArrival});
    //             // cout<<"time "<< curTime <<"ms: Process "<< newArrival->name<< " (tau " << predictedTau[newArrival->name] << "ms) " <<"arrived; added to ready queue " << printReadyQueue()<<endl;
    //         }
    //         startTime[newArrival->name].push_back( curTime);
    //         preemptPQ.pop();
    //     } 
    // }


    void prepareRunningProcess(){
        if(cpuStatus == CONTEXT_SWITCH_UP && remCSTime == 0){
            curProcess = readyPQ.top().second;
            readyPQ.pop();
            
            if(!preemptedProcesses.count(curProcess->name)){
            // cout<<curProcess->remCPU.size()<<endl;
                // preemptedProcesses.insert(curProcess->name);
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << predictedTau[curProcess->name] << "ms) " <<"started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
                }
                // cout<<"time "<< curTime <<"ms: Process "<< curProcess->name<< " (tau " << predictedTau[curProcess->name] << "ms) "  <<"started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
            }else{
                if(curTime < 999 || printall){
                    cout<<"time "<< curTime <<"ms: Process "<< curProcess->name << " (tau " << predictedTau[curProcess->name] << "ms) " <<"started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
                }
            }
            contextSwitchCnt[curProcess->name] ++;
            
        }
    }


    void updateReadyQueue(){
        updatReadyQueueFromPreemptyPQ();
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
           curTotBurst[curProcess->name] ++;
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

    string getStatRes(){
        string ret = "";
        unordered_map<char, int> cpuBurstTime, processWaitTime, processTurnAroundTime, ioBurstTime;
        int totCPUBursts = 0, totIOBursts = 0;
        for(auto p: processes){
            cpuBurstTime[p->name] = p->getProcessTotalRunningTime();
            ioBurstTime[p->name] = p->getProcessTotalIOTime();
            totCPUBursts += p->totBurst;
            totIOBursts += p->totBurst - 1;
            for(int i=0; i< (int)endTime[p->name].size(); i++){
                processTurnAroundTime[p->name] += endTime[p->name][i] - startTime[p->name][i] + csTime;
                
            }
            
            processWaitTime[p->name] = processTurnAroundTime[p->name]  - cpuBurstTime[p->name];
        }
        int totBurstTime = 0, totWaitTime = 0, totTurnAroundTime = 0, totCS = 0;
        for(auto p: processes){
            // cout<<p->name<<" "<<processTurnAroundTime[p->name]<<endl;
            char n = p->name;
            totBurstTime += cpuBurstTime[n];
            totWaitTime += processWaitTime[n];
            totTurnAroundTime += processTurnAroundTime[n];
            totCS += contextSwitchCnt[n];
        }
        totWaitTime -= (2*csTime*totCPUBursts + totPreemption * 2 * csTime);
        // cout<<totCPUBursts<<" "<<totIOBursts<<endl;
        double avgBurstTime = (double)totBurstTime / (double)totCPUBursts ;
        double avgWaitTime = (double)totWaitTime / (double)totCPUBursts;
        double avgTurnAroundTime = (double)totTurnAroundTime / (double)totCPUBursts ;
        // string strBurstTime = to_string(avgBurstTime);
        // string strWaitTime = to_string(avgWaitTime);
        // string strTurnAroundTime = to_string(avgTurnAroundTime);
        stringstream buffer;

        
        ret += "Algorithm SRT\n";
        buffer << std::fixed << setprecision(3) << avgBurstTime;
        ret += "-- average CPU burst time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgWaitTime;
        ret += "-- average wait time: " +  buffer.str() + " ms\n";
        buffer.str("");
        buffer << std::fixed << setprecision(3) << avgTurnAroundTime;
        ret += "-- average turnaround time: " +  buffer.str() + " ms\n";
        ret += "-- total number of context switches: " +  to_string(totCS) + "\n";
        ret += "-- total number of preemptions: " +  to_string(totPreemption) + "\n";
        // cout<< ret<<endl;
        return ret;

    }

    void runShortestRemainingTimeFirst(){
        resetAll();
        addArrivePQ();
        while (!isFinished()){
            
            bool finishedBurst = processFinishBurst();
            
            updateReadyQueue();
            // updateReadyQueue();
            bool preemptBurst = srfPreemptSwitchDown();
            
            prepareRunningProcess();
            cpuStatusSwitch(finishedBurst || preemptBurst);
            
            curTime ++;
            // cout<<curTime<<" "<<curProcess<<endl;
            // prepareRunningProcess();
            
            cpuRun1ms();
            incrementProcessRunningTime();
            
        }
        cout<<"time "<< curTime + 1 <<"ms: Simulator ended for SRT [Q <empty>]"<<endl;
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
    
    Simulator_SRF S_SRT(nProcess, sliceTime, contextSwitchTime/2, alpha, lambda, processes);
    Simulator_SJF S_SJF(nProcess, sliceTime, contextSwitchTime/2, alpha, lambda, processes);
    Simulator_RR_FCFS S_RR(nProcess, sliceTime, contextSwitchTime/2, processes, rrPos);
    Simulator_RR_FCFS S_FCFS(nProcess, sliceTime, contextSwitchTime/2, processes, END);
    S_FCFS.runFirstComeFirstServe();
    cout<<endl;
    S_SJF.runShortestJobFirst();
    cout<<endl;
    S_SRT.runShortestRemainingTimeFirst();
    cout<<endl;
    S_RR.runRoundRobin();

    ofstream outfile;
    outfile.open("simout.txt");
    outfile << S_FCFS.getStatRes();
    outfile << S_SJF.getStatRes();
    outfile << S_SRT.getStatRes();
    outfile << S_RR.getStatRes();
    // cout<<S_SRT.getStatRes()<<endl;

    return 0;

}
