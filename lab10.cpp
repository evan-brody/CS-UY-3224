/// @file    lab10.cpp
/// @author  Evan Brody
/// @brief   Simulates the second chance LRU page-replacement algorithm

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <cassert>

// CONSTANTS for bitmasking
const unsigned REFERENCE = 1;
const unsigned VALID = 2;

struct PTableEntry {
    int frame = -1;
    // vr's upper six bits are unused
    // vr's two least significant bits:
    // valid = memory-resident ? (more significant)
    // reference = referenced recently ? (least significant)
    unsigned char vr = 0;
};

/// @brief Generates a random page trace of length n,
// with page numbers in [0, p)
/// @return int* A pointer to the dynamic array holding
// the page trace
int* PageTraceGenerator(int n, int p);

/// @brief Simulates the second chance LRU page-replacement algorithm.
/// @param ptrace   const int[] page trace
/// @param n        int page trace length
/// @param f        int frame count
/// @param p        int page count
/// @return         int number of page faults in simulation
int Simulate(const int* ptrace, int n, int f, int p);

/// @brief Finds and evicts a page from memory based on the second chance
//         LRU page-replacement algorithm.
/// @param pageTable    PTableEntry*
/// @param p            int length of pageTable
/// @return             int index of frame from evicted page
int FindVictim(PTableEntry* pageTable, int p);

int main(int argc, char** argv) {
    srand(time(NULL));
    if (3 != argc) {
        std::cerr << "ERROR: Provide exactly two arguments\n";
        exit(1);
    }

    int n = atoi(argv[1]); // Page trace length
    int p = atoi(argv[2]); // Page count

    if (!(n >= 16)) {
        std::cerr << "ERROR: n must be >= 16.\n";
        exit(1);
    } else if (!(p >= 8)) {
        std::cerr << "ERROR: p must be >= 8.\n";
        exit(1);
    }

    int* pageFaultCounts = new int[p - 3]; // |[4, p]| = p - 3
    int* ptrace = PageTraceGenerator(n, p);
    for (int f = 4; f <= p; f++) { // f is number of frames
        pageFaultCounts[f - 4] = Simulate(ptrace, n, f, p);
    }
    delete[] ptrace;

    // Output to csv
    std::ofstream pageFaultFile("pageFaults.csv");
    pageFaultFile << "Frames,Page Faults\n";
    for (int f = 4; f <= p; f++) {
        pageFaultFile << f << ',' << pageFaultCounts[f - 4] << '\n';
    }
    delete[] pageFaultCounts;

    return 0;
}

// Returns a random number in [0, k) \ {j}, j < k
inline int RandNotJ(int j, int k) {
    assert(j < k);
    int res = rand() % (k - 1); // [0, k - 1)
    return res < j ? res : res + 1; // [0, k) \ {j}
}

int* PageTraceGenerator(int n, int p) {
    int* ptrace = new int[n];
    ptrace[0] = rand() % p; // No restrictions on first page
    for (int i = 1; i < n; i++) {
        // Ensure ptrace[i] != ptrace[i - 1]
        ptrace[i] = RandNotJ(ptrace[i - 1], p);
    }

    return ptrace;
}

int Simulate(const int* ptrace, int n, int f, int p) {
    int numFramesOpen = f; // Initially, all frames are open
    PTableEntry* pageTable = new PTableEntry[p];
    int numPageFaults = 0;

    // Step through page trace and count page faults
    for (int i = 0; i < n; i++) {
        int pageAccessed = ptrace[i];
        PTableEntry& page = pageTable[pageAccessed];
        page.vr |= REFERENCE; // Record that the page has been referenced
        if (!(page.vr & VALID)) { // Page fault
            numPageFaults++;
            // First, check for an open frame
            if (numFramesOpen) { 
                page.frame = --numFramesOpen;
            } else { // If none are open, evict a page
                page.frame = FindVictim(pageTable, p); // Set frame number to the newly opened frame
            }
            page.vr |= VALID; // Record that the page is now valid
        }
    }
    delete[] pageTable;

    return numPageFaults;
}

int FindVictim(PTableEntry* pageTable, int p) {
    for (static int i = 0;; ++i %= p) {
        PTableEntry& possibleVictim = pageTable[i];
        // Can only be a victim if it's currently in memory
        if (!(possibleVictim.vr & VALID)) { continue; }

        // If the page hasn't been referenced recently, evict it
        if (!(possibleVictim.vr & REFERENCE)) {
            possibleVictim.vr &= ~VALID;
            int newlyOpenFrame = possibleVictim.frame;
            possibleVictim.frame = -1;
            ++i %= p; // Increment before returning
            return newlyOpenFrame;
        } else { // If it has, it deserves a second chance
            possibleVictim.vr &= ~REFERENCE;
        }
    }
}