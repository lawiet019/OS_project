
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
    // Process* switchOutCand;
    priority_queue<pair<int, Process*>, vector<pair<int, Process*>>, cmp> arrivePQ, ioPQ, preemptPQ;
    deque<Process*> readyQueue;
    unordered_set<char> preemptedProcesses;


    bool isFinished(){
        return (finished == nProcess);
    }

    Simulator_RR_FCFS(int _n, int _s, int _cs, vector<Process*> &_processes){
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
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" completed a CPU burst; "<< curProcess->remBurst <<" burst"<< (curProcess->remBurst == 1? "" : "s") <<" to go "<<printReadyQueue()<<endl;
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" switching out of CPU; will block on I/O until time "<< nextReady + csTime <<"ms "<< printReadyQueue() <<endl;
            }
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
            cout<<"time "<< curTime <<"ms: Time slice expired; no preemption because ready queue is empty "<< printReadyQueue() <<endl;
            return false;
        }
        preemptedProcesses.insert(curProcess->name);
        preemptPQ.push({curTime + csTime, curProcess});
        cpuStatus = CONTEXT_SWITCH_DOWN;
        curProcessRunningTime = 0;
        cout<<"time "<< curTime <<"ms: Time slice expired; process "<<curProcess->name<<" preempted with "<< curProcess->remCPU.back() <<"ms to go "<<printReadyQueue()<<endl;   
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
            cout<<"time "<< curTime <<"ms: Process "<< newArrival->name <<" completed I/O; added to ready queue "<< printReadyQueue() <<endl;
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
            cout<<"time "<< curTime <<"ms: Process "<< newArrival->name <<" arrived; added to ready queue " << printReadyQueue()<<endl;
        }
    }
    
    void updatReadyQueueFromPreemptPQ(){
        while(!preemptPQ.empty() && preemptPQ.top().first == curTime){
            if(cpuStatus == IDLE){
                cpuStatus = CONTEXT_SWITCH_UP;
            }
            auto newArrival = preemptPQ.top().second;
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
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" started using the CPU for "<< curProcess->remCPU.back() <<"ms burst " << printReadyQueue() <<endl;
            }else{
                cout<<"time "<< curTime <<"ms: Process "<< curProcess->name <<" started using the CPU with "<< curProcess->remCPU.back() <<"ms burst remaining " << printReadyQueue() <<endl;
            }
        }
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
    }

    void runFirstComeFirstServe(){
        sliceTime = INT_MAX;
        runRoundRobin();
    }


};
