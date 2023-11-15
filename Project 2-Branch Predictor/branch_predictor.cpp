#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]){
    ifstream streamFile;
    unsigned int N, M, numAddresses = 0, numEntries, numMispredictions = 0, prediction = 0, actualDecision, hashIndex;
    unsigned long long int currentAddr, curAddrMBits, GHR = 0, zeroExtendedGHR, bitMask = 0;
    string address, predictorType, streamFileLine;
    char decision;
    vector<int> bufferTable;

    if (argc != 5){
        cerr << "Error! Invalid number of arguments\n";
        exit(1);
    }

    predictorType = argv[1];
    M = atoi(argv[2]);
    N = atoi(argv[3]);

    streamFile.open(argv[4], ios::in);

    if (!streamFile.is_open()){
        cerr << "Error! File didn't open" << endl;
        exit(1);
    }

    // Set the number of entries to the buffer table and initialize the table
    numEntries = pow(2, M);

    for (int i = 0; i < numEntries; i++)
        bufferTable.push_back(2);
    
    // Initilize bit mask to be M bits
    for (int i = 0; i < M; i++){
        bitMask <<= 1;
        bitMask |= 1;
    }

    // Process all memory accesses
    while(getline(streamFile, streamFileLine))
    {
        numAddresses++;
        istringstream iss(streamFileLine);
        iss >> address >> decision;
        currentAddr = (stoull(address, nullptr, 16));

        // Extract M bits from the address
        curAddrMBits = (currentAddr >> 2) & bitMask;

        actualDecision = (decision == 't') ? 1 : 0;

        // Extend GHR to be M bits
        if (N > 0)
            zeroExtendedGHR = GHR << (M-N);
        else
            zeroExtendedGHR = GHR << (M-1);

        // Create hash value by XORing the M address bits and the GHR
        hashIndex = curAddrMBits ^ zeroExtendedGHR;

        // If entry at hash index is Weakly Taken or Strongly Taken, predict taken
        prediction = (bufferTable[hashIndex] > 1) ? 1 : 0;
        
        // If actually taken and entry isn't strongly taken, increment entry
        if (actualDecision == 1 && bufferTable[hashIndex] < 3)
            bufferTable[hashIndex]++;
        // Else if actually not taken and entry isn't strongly not taken, decrement entry
        else if (actualDecision == 0 && bufferTable[hashIndex] > 0)
            bufferTable[hashIndex]--;

        // Update number of mispredictions if prediction is false
        if (prediction != actualDecision)
            numMispredictions++;

        // Set next GHR
        GHR = GHR >> 1;

        if (N > 0)
            GHR |= (actualDecision << (N-1));
        else
            GHR = actualDecision;
    }

    streamFile.close();
    cout << M << " " << N << " " << (float)numMispredictions/numAddresses*100 << "%" << endl;
    
    return 0;
}