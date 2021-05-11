import os
import numpy as np
from time import time
from scipy import signal
import subprocess
import matplotlib.pyplot as plt


def thread_perf(use_pthread, exec_file_name, argc, start, end, opt):
    thread_nb = []
    time_exec = []
    if use_pthread :
        exec_file_name = "./pthread_bin/" + exec_file_name
    if not use_pthread:
        exec_file_name = "./install/bin/" + exec_file_name
        os.environ["LD_LIBRARY_PATH"] = "./install/lib" + os.pathsep + os.environ.get("LD_LIBRARY_PATH", "")
        if exec_file_name[14:26] == "51-fibonacci" : #fibonacci pushed to 33 in the customized lib
            end = 33
        if exec_file_name[14:36] == "33-switch-many-cascade" : #33-switch-many-cascade pushed to 600 in the customized lib
            end = 600
    for i in range(start, end+1):
        exec = [exec_file_name]
        for j in range(argc - 1): #argc - 1
            exec.append(str(i))
        if (argc > 2) :
            exec.append(str(opt))
        print("generating graph : " + exec_file_name)
        
        for k in range (int(((i-1)*100)/(end*2))) :
            print("#", end="")
        print(" ", int(((i-1)*100 + 1)/end), "%")
        print("\n")
        t1 = time()
        subprocess.run(exec)
        t2 = time()
        
        thread_nb.append(i)
        time_exec.append(t2-t1)
    if not os.path.isdir("./graphs"):
        os.mkdir("./graphs")
        
    time_exec = signal.savgol_filter(time_exec, 21, 3)
    if use_pthread:
        plt.plot(thread_nb, time_exec, label = 'pthread')
    else:
        plt.plot(thread_nb, time_exec, label = 'custom lib')

def clean():
    subprocess.run(['make', 'clean'])

def get_perf_graphs():
    e1 = ["22-create-many-recursive", 2, 1, 1000, 0]
    #e2 = ["33-switch-many-cascade", 3, 1, 400, 5]
    e3 = ["51-fibonacci", 2, 1, 21, 0]
    e4 = ["62-mutex", 2, 1, 100, 0]
    all = [e1, e3, e4]

    subprocess.run(['make', 'pthreads'])
    clean()
    subprocess.run(['make', 'install'])


    i = 0
    for e in all:
        #Generating graphs 
        plt.title(label = e[0])
        plt.xlabel("nb threads")
        plt.ylabel("exec time (s)")
        file = e[0]
        
        thread_perf(True, *e) #Generating perfs for pthread

        thread_perf(False, *e) #Generating perfs for ustomized lib
        
        plt.legend()
        plt.savefig("./graphs/"+file+"_graph.png")
        plt.figure()
    clean()

get_perf_graphs()