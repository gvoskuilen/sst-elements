#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <algorithm>

/* 
 *  SLURM-specific MPI launcher for Ariel simulations
 *  Ariel forks this process which initiates mpirun
 *  Usage:
 *  mpilauncher procs pinprocs pinstring
 *
 *  where procs is the number of non-pinned processes
 *  pinprocs is the number of pinned processes
 *  pinstring is the string that Ariel would normally send to execvp
 */
int main(int argc, char *argv[]) {
    /*
     *  mpirun -H 'pin_node' -np 1 'pincmd' -- binary : -H unpin_nodes -np X binary
     */
    printf("mpilauncher starting...\n");
    
    std::array<char, 128> buffer;
    
    // Get node that SST is running on - Pin goes on that one
    gethostname(buffer.data(), 128);
    std::string pin_host = buffer.data();
    size_t pos = pin_host.find('.');
    pin_host = pin_host.substr(0,pos);

    // Now find the rest of the hosts and remove the pin_host from the list
    std::string hosts;
    const char* cmd = "scontrol show hostnames ${SLURM_JOB_NODELIST}";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            hosts += buffer.data();
    }
    std::string unpin_hosts;
    std::replace(hosts.begin(), hosts.end(), '\n', ',');
    hosts.pop_back();
    pos = hosts.find(pin_host + ",");
    if (pos != std::string::npos) {
        unpin_hosts = hosts.erase(pos, pin_host.size()+1);
    } else {
        pos = hosts.find(pin_host);
        if (pos != std::string::npos) {
            unpin_hosts = hosts.erase(pos, pin_host.size());
        } else {
            printf("Very odd...pin_host is not found in the hosts string...I want no part of this, goodbye!\n");
            exit(-1);
        }
    }

    if (unpin_hosts.empty()) // Launching on one host
        unpin_hosts = pin_host;

    int procs = atoi(argv[1]);
    int pinprocs = atoi(argv[2]);
    if (!pinprocs && !procs) {
        printf("You didn't ask the mpilauncher to launch anything...\n");
        exit(-1);
    }
    std::vector<std::string> pinstring(pinprocs, "");    // One string per pinproc
    if (pinprocs == 0)
        pinstring.push_back("");

    std::string binary = "";
    bool getbinary = false;
    std::string arg;
    for (int i = 3; i < argc; i++) {
        arg = argv[i];

        for (int j = 0; j < pinstring.size(); j++) {
            if (arg == "-c") { // insert core offset
                pinstring[j] += "-b " + std::to_string(atoi(argv[i+1]) * j);
                pinstring[j] += " ";
            }
            // Pin string
            pinstring[j] += arg;
            pinstring[j] += " ";
        }

        // Binary string
        if (getbinary) {
            binary += arg;
            binary += " ";
        }

        if (arg == "--")
            getbinary = true;
    }

    if (getbinary == false) 
        binary = pinstring[0];

    std::string mpicmd = "mpirun -oversubscribe ";
    for (size_t i = 0; i < pinprocs; i++) {
        if (i != 0) mpicmd += " : ";
        mpicmd += "-H " + pin_host + " -np 1 " + pinstring[i];
    }
    if (procs) {
        if (pinprocs) mpicmd += " : ";
        mpicmd += "-H " + unpin_hosts + " -np " + std::to_string(procs) + " " + binary;
    }

    printf("Wrapper starting...\n");
    printf("Arg to system: %s\n", mpicmd.c_str());
    system(mpicmd.c_str());
    printf("Wrapper complete...\n");
}
