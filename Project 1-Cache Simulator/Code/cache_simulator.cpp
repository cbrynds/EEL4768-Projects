#include <iostream>
#include <fstream>
#include <ostream>
#include <string>

using namespace std;

// Preprocessor constants for each replacement policy
#define LRU 0
#define FIFO 1
#define LIFO 2
#define MRU 3
#define LFU 4

// Preprocessor constants for each write-back policy
#define WRITE_THROUGH 0
#define WRITE_BACK 1

// Data structure to implement the cache and all related methods
class Cache{
    private:
        int cacheSize, associativity, indexBits, tagBits, numSets, replacement, readCount, writeCount;
        const int blockSize, offsetBits, instructionLength;
        bool wb;
        long long unsigned int numAddresses;

        // Struct to represent a single block inside of a set
        struct Block{
            long long unsigned int tag, numAccesses;
            bool dirty;

            Block(long long unsigned int initTag, bool initDirty) : 
            tag(initTag),  
            dirty(initDirty){
                numAccesses = 0;
            }
        };

        // 2D vector to represent the L1 cache
        vector<vector<Block> > L1Cache;

    public:
        // Constructor method to initialize all constants and variables
        Cache(int size, int assoc, int replacement, bool wb) : 
        blockSize(64), 
        offsetBits(6),
        instructionLength(64)
        {
            this->cacheSize = size;
            this->replacement = replacement;
            this->wb = wb;
            this->associativity = assoc;
            this->numSets = cacheSize/(blockSize*associativity);
            this->indexBits = log2(cacheSize/(associativity*blockSize));
            this->tagBits = instructionLength - offsetBits - indexBits;
            readCount = 0;
            writeCount = 0;
            L1Cache.resize(numSets);
        }

        // Returns the number of index bits in each memory address
        int getIndexBits(){
            return indexBits;
        }

        // Returns the number of offset bits in each memory address
        int getoffsetBits(){
            return offsetBits;
        }

        // Returns the number of sets in the cache
        int getNumSets(){
            return numSets;
        }

        // Sets the total number of memory addresses to be processed
        void setNumAddresses(long long unsigned int numAddresses){
            this->numAddresses = numAddresses;
        }

        // Implements the Least Recently Used (LRU) replacement policy
        void LRUReplacement(long long unsigned int setNumber, long long unsigned int tag, bool write){
             if (L1Cache[setNumber].size() < associativity)
                    L1Cache[setNumber].insert(L1Cache[setNumber].begin(), Block(tag, write));
            // Else, replace the least recently used block
            else{
                if (L1Cache[setNumber][associativity-1].dirty && wb == WRITE_BACK)
                    writeCount++;
                L1Cache[setNumber].pop_back();
                L1Cache[setNumber].insert(L1Cache[setNumber].begin(), Block(tag, write));
            }
        }

        // Implements the First In, First Out (FIFO) replacement policy
        void FIFOReplacement(long long unsigned int setNumber, long long unsigned int tag, bool write){
            // If set it not full, place the block in the set
            if (L1Cache[setNumber].size() < associativity)
                L1Cache[setNumber].push_back(Block(tag, write));
            // Else, replace the last element in the set
            else{
                if (L1Cache[setNumber][0].dirty && wb == WRITE_BACK)
                    writeCount++;
                L1Cache[setNumber].erase(L1Cache[setNumber].begin());
                L1Cache[setNumber].push_back(Block(tag, write));
            }
        }

        // Implements the Last In, First Out (LIFO) replacement policy (BONUS)
        void LIFOReplacement(long long unsigned int setNumber, long long unsigned int tag, bool write){
            // If set it not full, place the block in the set
            if (L1Cache[setNumber].size() < associativity)
                L1Cache[setNumber].push_back(Block(tag, write));
            // Else, replace the last element in the set
            else{
                if (L1Cache[setNumber][associativity-1].dirty && wb == WRITE_BACK)
                    writeCount++;
                L1Cache[setNumber].pop_back();
                L1Cache[setNumber].push_back(Block(tag, write));
            }
        }

        // Implements the Most Recently Used (MRU) replacement policy (BONUS)
        void MRUReplacement(long long unsigned int setNumber, long long unsigned int tag, bool write){
             if (L1Cache[setNumber].size() < associativity)
                    L1Cache[setNumber].insert(L1Cache[setNumber].begin(), Block(tag, write));
            // Else, replace the least recently used block
            else{
                if (L1Cache[setNumber][0].dirty && wb == WRITE_BACK)
                    writeCount++;
                L1Cache[setNumber].erase(L1Cache[setNumber].begin());
                L1Cache[setNumber].insert(L1Cache[setNumber].begin(), Block(tag, write));
            }
        }

        // Implements the Least Frequency Used (LFU) replacement policy (BONUS)
        void LFUReplacement(long long unsigned int setNumber, long long unsigned int tag, bool write){
            // If set it not full, place the block in the set
            if (L1Cache[setNumber].size() < associativity)
                L1Cache[setNumber].push_back(Block(tag, write));
            // Else, replace the last element in the set
            else{
                int LFUIndex = 0, leastAccesses = numeric_limits<int>::max();
                for (int i = 0; i < associativity; i++){
                    if (L1Cache[setNumber][i].numAccesses < leastAccesses){
                        leastAccesses = L1Cache[setNumber][LFUIndex].numAccesses;
                        LFUIndex = i;
                    }
                }
                if (L1Cache[setNumber][LFUIndex].dirty && wb == WRITE_BACK)
                    writeCount++;
                L1Cache[setNumber][LFUIndex] = Block(tag, write);
            }
        }

        // Reads a given memory address and handles cache misses, replacement policies
        void memoryRead(long long unsigned int setNumber, long long unsigned int tag){
            // Check all blocks in the given set to see if a tag match is found
            for (int blockNumber = 0; blockNumber < L1Cache[setNumber].size(); blockNumber++){
                // Only check block tag if data is valid
                if (tag == L1Cache[setNumber][blockNumber].tag){
                    // If LRU policy (or MRU) is used, move memory element to the front of the set
                    if (replacement == LRU || replacement == MRU)
                        rotate(L1Cache[setNumber].begin(), L1Cache[setNumber].begin()+blockNumber, L1Cache[setNumber].begin()+blockNumber+1);
                    // If LFU policy is used, update use frequency
                    else if (replacement == LFU)
                        L1Cache[setNumber][blockNumber].numAccesses++;
                    return;
                }
            }
            // Block not found, cache miss
            readCount++;

            // Replacement Policy
            switch(replacement){
                case LRU:
                    LRUReplacement(setNumber, tag, 0);
                    break;
                case FIFO:
                    FIFOReplacement(setNumber, tag, 0);
                    break;
                case LIFO:
                    LIFOReplacement(setNumber, tag, 0);
                    break;
                case MRU:
                    MRUReplacement(setNumber, tag, 0);
                    break;
                case LFU:
                    LFUReplacement(setNumber, tag, 0);
                    break;
                default:
                    FIFOReplacement(setNumber, tag, 0);
                    break;
            }         
        }

        // Writes to a given memory address and handles cache misses, write-back policies
        void memoryWrite(long long unsigned int setNumber, long long unsigned int tag){

            // Check all blocks in the given set to see if a tag match is found
            for (int blockNumber = 0; blockNumber < L1Cache[setNumber].size(); blockNumber++){
                // Only check block tag if data is valid
                if (tag == L1Cache[setNumber][blockNumber].tag){                 
                        // If write-back policy is used
                        if (wb == WRITE_BACK)
                            L1Cache[setNumber][blockNumber].dirty = 1;
                        // Else if write-through policy is used
                        else
                            writeCount++;

                        // If LRU (or MRU) policy is used, move memory element to the front of the set
                        if (replacement == LRU || replacement == MRU)
                            rotate(L1Cache[setNumber].begin(), L1Cache[setNumber].begin()+blockNumber, L1Cache[setNumber].begin()+blockNumber+1);
                        // If LFU policy is used, update use frequency
                        else if (replacement == LFU)
                            L1Cache[setNumber][blockNumber].numAccesses++;
                        return;
                }
            }
            // Block not found, cache miss
            readCount++;
            
            // Update write count if write-through policy is used
            if (wb == WRITE_THROUGH)
                writeCount++;

            // Replacement Policy
            switch(replacement){
                case 0:
                    LRUReplacement(setNumber, tag, 1);
                    break;
                case 1:
                    FIFOReplacement(setNumber, tag, 1);
                    break;
                case 2:
                    LIFOReplacement(setNumber, tag, 1);
                    break;
                case 3:
                    MRUReplacement(setNumber, tag, 1);
                    break;
                case 4:
                    LFUReplacement(setNumber, tag, 1);
                    break;
                default:
                    FIFOReplacement(setNumber, tag, 1);
                    break;
            }    
        }

        // Prints the information related to the cache implemented in each run - For debugging purposes
        void printCacheInfo(){
            cout << "Cache Size: " << cacheSize << endl;
            cout << "Cache Associativity: ";

            if (cacheSize / blockSize == associativity)
                cout << "Direct Mapped" << endl;
            else if (associativity == 1)
                cout << "Fully Associative" << endl;
            else
                cout << associativity << "-Way Set Associative" << endl;
            
            cout << "Replacement Policy: ";
            if (replacement)
                cout << "FIFO\n";
            else
                cout << "LRU\n";
        
            cout << "Write-back Policy: ";
            if (wb)
                cout << "Write-Back\n";
            else
                cout << "Write-Through\n";

            cout << "# Index Bits: " << indexBits << endl;
            cout << "# Tag Bits: " << tagBits << endl;
        }

        // Prints the contents of each set in the cache - For debugging purposes
        void printCacheContents(){
            for (int setIndex = 0; setIndex < numSets; setIndex++){
                cout << "Set #" << setIndex << " Contents: \n";
                for (int blockIndex = 0; blockIndex < L1Cache[setIndex].size(); blockIndex++){
                    cout << "\tBlock #" << setIndex << "-" << blockIndex << " Tag: " << L1Cache[setIndex][blockIndex].tag \
                    << "\t| Dirtiness: " << L1Cache[setIndex][blockIndex].dirty << endl; 
                }
            }
        }

        // Prints the contents of each block in a given set - For debugging purposes
        void printSetContents(int setIndex){
                cout << "Set #" << setIndex << " Contents: \n";
                for (int blockIndex = 0; blockIndex < L1Cache[setIndex].size(); blockIndex++){
                    cout << "\tBlock #" << setIndex << "-" << blockIndex << " Tag: " << L1Cache[setIndex][blockIndex].tag \
                    << "\t| Dirtiness: " << L1Cache[setIndex][blockIndex].dirty << endl; 
                }
        }

        // Prints the overall number of writes, reads, and the miss ratio at the end of the test case
        void printMemStats(){
            cout << "Miss ratio " << (readCount / (float)numAddresses) << endl;
            cout << "write " << writeCount << endl;
            cout << "read " << readCount << endl;
        }

        // Appends the overall number of writes, reads, and the miss ratio to the end of a text file called "simulation-results.txt" - For debugging purposes
        void appendMemStats(char *argv[]){
            ofstream outputFile;

            outputFile.open("simulation-results.txt", ios::app);

            if (!outputFile.is_open()){
                cerr << "Failed to open the output file for writing." << endl;
                return;
            }
            outputFile << "Results for: ";
            for (int i = 0; i < 6; i++)
                outputFile << argv[i] << " ";

            outputFile << endl << (readCount / (float)numAddresses) << endl;
            outputFile << writeCount << endl;
            outputFile << readCount << endl << endl;

            outputFile.close();
        }
};

int main(int argc, char* argv[])
{
    ifstream traceFile;
    string traceFilepath, traceFileLine;
    long long unsigned int memAddr, numAddresses = 0;

    // If any CL arguments are missing, exit program
    if (argc != 6)
    {
        cout << "Error! Invalid number of arguments\n";
        exit(0);
    }

    Cache myCache(stoi(argv[1]), stoi(argv[2]), stoi(argv[3]), stoi(argv[4]));

    traceFilepath = argv[5];
    traceFile.open(traceFilepath, ios::in);

    if (!traceFile.is_open()){
        cout << "Error! File not opened";
        exit(0);
    }

    // Process all memory accesses
    while(getline(traceFile, traceFileLine))
    {
        numAddresses++;
        string memAddrString = traceFileLine.substr(1);
        memAddr = stoull(memAddrString, nullptr, 16);

        memAddr = memAddr >> myCache.getoffsetBits();
        long long unsigned int tag = memAddr >> myCache.getIndexBits();
        long long unsigned int index = memAddr - (tag << myCache.getIndexBits());
        long long unsigned int setNumber = index % myCache.getNumSets();

        // Process memory write
        if (traceFileLine[0] == 'W')
            myCache.memoryWrite(setNumber, tag);
        // Process memory read
        else
            myCache.memoryRead(setNumber, tag);
    }

    myCache.setNumAddresses(numAddresses);

    myCache.printMemStats();

    traceFile.close();

    return 0;
}