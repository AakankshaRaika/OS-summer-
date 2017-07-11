#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <sstream>
#include <queue>
#include <getopt.h>
#include <iterator>
#include <unordered_map>
#include <string.h>

using namespace std;


unordered_map<int, int> pageMap;

class FrameContents {
public:
    int pageNumber;
    int age;
    FrameContents();
    ~FrameContents();

    friend bool operator<(const FrameContents& lhs, const FrameContents& rhs) {

        if (pageMap.at(lhs.pageNumber) < pageMap.at(rhs.pageNumber)) {
            return true;
        } else {
            if (pageMap.at(lhs.pageNumber) == pageMap.at(rhs.pageNumber)) {
                return (lhs.age < rhs.age);
            } else {
                return false;
            }
        }
    }
};

struct clock {
    int referenceBit;
    int page;
};

int numOfFrames = 5;
string filePath;
string replacementPolicy = "LRU-REF8";

vector<int> inFile(); //to get file
void onStart(); //start page
void initializeVector(vector<FrameContents>& frameVector);
int fifo(vector<int> pageVector, int frameNum);
int lru(vector<int> pageVector, int frameNum);
int lfu(vector<int> pageVector, int frameNum);
int optimal(vector<int> pageVector, int frameNum);
void incrementFrameAge(vector<FrameContents>& frameVector);
int lruStack(vector<int> pageVector, int frameNum);
bool checkAndReplace(stack<int>& frameStack, int frameNum, int page, int& replacemens);
void removeBottomPage(stack<int>& frameStack);
bool checkDuplicate(stack<int>& frameStack, int page);
void printResults(int pageReplacements, int optimalPageRepl, double algoTime, double optimalTime, string replacementPolicy);
vector<pair<int, string>> lru_ref8(stack<int>& frameStack, int frameNum);
int incrementVictimPointer(int currentVal, int frameNum);
int lruClock(vector<int> pageVector, int frameNum);
int lruRef8(vector<int> pageVector, int frameNum);

int main(int argc, char *argv[]) {
    int ch;
    while ((ch = getopt(argc, argv, "hf:i:r:")) != -1) {
        switch (ch) {
            case 'h':
                cout << "−h : Print a usage summary with all options and exit." << endl;
                cout << "−f available-frames : Set the number of available frames." << endl;
                cout << "−r replacement-policy : Set the page replacement policy. It can be either" << endl
                        << "FIFO (First-in-first-out)" << endl
                        << "LFU (Least-frequently-used)" << endl
                        << "LRU-STACK (Least-recently-used stack implementation)" << endl
                        << "LRU-CLOCK (Least-recently-used clock implementation –second chance alg.)." << endl
                        << "LRU-REF8 (Least-recently-used Reference-bitImplementation, using 8 reference bits)" << endl;
                cout << "−i input file : Read the page reference sequence from a specified file. If not given,the sequence should be read from STDIN (ended with ENTER)." << endl;
                exit(1);
                break;
            case 'f':
                numOfFrames = atoi(optarg);
                break;
            case 'i':
                filePath = optarg;
                break;
            case 'r':
                replacementPolicy = optarg;
                break;
        }
    }
    onStart();
    return 0;
}

//constructor

FrameContents::FrameContents() {
    age = 0;
    pageNumber = 0;
}

//deconstructor

FrameContents::~FrameContents() {

}

void onStart() {
    vector<int> holdPages;
    holdPages = inFile(); //getting pages 
    int pageReplacements, optimalPageRepl;
    time_t t, tOpt;
    double algoTime, optimalTime;
    if (replacementPolicy == "FIFO") {
        t = clock();
        pageReplacements = fifo(holdPages, numOfFrames);
        algoTime = clock() - t;
    } else if (replacementPolicy == "LFU") {
        t = clock();
        pageReplacements = lfu(holdPages, numOfFrames);
        t = clock() - t;
    } else if (replacementPolicy == "LRU-STACK") {
        t = clock();
        pageReplacements = lruStack(holdPages, numOfFrames);
        t = clock() - t;
    } else if (replacementPolicy == "LRU-CLOCK") {
        t = clock();
        pageReplacements = lruClock(holdPages, numOfFrames);
        t = clock() - t;
    } else if (replacementPolicy == "LRU-REF8") {
        t = clock();
        pageReplacements = lruRef8(holdPages, numOfFrames);
        t = clock() - t;
    }
    algoTime = (double(t) / CLOCKS_PER_SEC)*100.0;
    tOpt = clock();
    optimalPageRepl = optimal(holdPages, numOfFrames);
    tOpt = clock() - tOpt;
    optimalTime = (double(tOpt) / CLOCKS_PER_SEC)*100.0;
    printResults(pageReplacements, optimalPageRepl, algoTime, optimalTime, replacementPolicy);
}

void printResults(int pageReplacements, int optimalPageRepl, double algoTime, double optimalTime, string replacementPolicy) {
    cout << "# of page replacements with " + replacementPolicy + "\t:" << to_string(pageReplacements) << endl;
    cout << "# of page replacements with Optimal\t:" << to_string(optimalPageRepl) << endl;
    cout << "% page replacement penalty using " + replacementPolicy + "\t:" << to_string((double) (pageReplacements - optimalPageRepl) / optimalPageRepl) + "%" << endl;
    cout << endl;
    cout << "Total time to run " + replacementPolicy + " Algorithm\t:" << to_string(algoTime * 1000) + "msec" << endl;
    cout << "Total time to run Optimal Algorithm\t:" << to_string(optimalTime * 1000) + "msec" << endl;
    if (algoTime < optimalTime) {
        cout << replacementPolicy + " is " + to_string((float) (optimalTime - algoTime) / optimalTime) + "% faster than Optimal algorithm" << endl;
    } else {
        cout << replacementPolicy + " is " + to_string((float) (algoTime - optimalTime) / optimalTime) + "% slower than Optimal algorithm" << endl;
    }
}

vector<int> inFile() {
    string line = "";

    vector<int> holdFrames;
    if (filePath.length() <= 1) {
        cout << "Please enter the sequence : "; //error check?
        getline(cin, line);
        stringstream ss(line);
        int num;
        while (ss >> num) {
            holdFrames.push_back(num);
        }

    } else {
        ifstream myfile(filePath.c_str());
        if (myfile.is_open()) //opening file path to put frames in vector
        {
            int getNum = 0;
            while (myfile >> getNum) {
                holdFrames.push_back(getNum);
                //cout << getNum << '\n';
            }
            myfile.close();
        } else cout << "Unable to open file\n";
    }
    cout << "Reference String : ";
    for (int i = 0; i < holdFrames.size(); i++) {
        cout << " " << holdFrames[i];
    }
    cout << endl;
    return holdFrames;
}

int fifo(vector<int> pageVector, int frameNum) {
    queue<int> frameQueue;
    queue<int> tempQueue;
    int replaceCount = 0;
    bool duplicate = false;

    for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it) {
        if (frameQueue.size() < frameNum) {
            bool dupe = false;
            while (!frameQueue.empty()) {
                tempQueue.push(frameQueue.front());
                if (frameQueue.front() == *it) {
                    dupe = true;
                }
                frameQueue.pop();
            }
            if (dupe) {
                while (!tempQueue.empty()) {
                    frameQueue.push(tempQueue.front());
                    tempQueue.pop();
                }
            } else {
                while (!tempQueue.empty()) {
                    frameQueue.push(tempQueue.front());
                    tempQueue.pop();
                }
                frameQueue.push(*it);
            }

            cout << "miss\n";
            //++replaceCount;
        } else {
            while (!frameQueue.empty()) {
                tempQueue.push(frameQueue.front());
                if (frameQueue.front() == *it) {
                    duplicate = true;
                    cout << "hit\n";
                }
                frameQueue.pop();
            }
            if (duplicate == true) {
                while (!tempQueue.empty()) {
                    frameQueue.push(tempQueue.front());
                    tempQueue.pop();
                }
                duplicate = false;
            } else {
                cout << "replace\n";
                ++replaceCount;
                while (!tempQueue.empty()) {
                    //cout << "In queue: " << tempQueue.front();
                    frameQueue.push(tempQueue.front());
                    tempQueue.pop();
                }
                frameQueue.pop();
                frameQueue.push(*it);
            }
        }
    }

    return replaceCount;
}

int lfu(vector<int> pageVector, int frameNum) {
    priority_queue<FrameContents> priorQueueFrame;
    FrameContents tempFrame;
    FrameContents deletePage;
    int countPages = 0;
    int replacements = 0;


    for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it) {
        cout << "*it is: " << *it << endl;
        auto search = pageMap.find(*it);
        if (search == pageMap.end()) {
            tempFrame.age = countPages;
            tempFrame.pageNumber = *it;
            pageMap.insert({*it, 1});
            if (priorQueueFrame.size() < frameNum) {
                priorQueueFrame.push(tempFrame);
                cout << "miss\n";
            } else {
                replacements++;
                priority_queue<FrameContents> tempPq;
                deletePage = priorQueueFrame.top();
                cout << "break7\n";
                cout << "break4\n";
                for (auto it = pageMap.begin(); it != pageMap.end(); ++it)
                    std::cout << " " << it->first << ":" << it->second;
                priorQueueFrame.pop();
                cout << "break2\n";
                pageMap.erase(deletePage.pageNumber);
                cout << "break3\n";
                priorQueueFrame.push(tempFrame);
                while (!priorQueueFrame.empty()) {
                    cerr << "PriorQueue contents: age: " << priorQueueFrame.top().age << " number: " <<
                            priorQueueFrame.top().pageNumber << "hash count: " <<
                            pageMap.at(priorQueueFrame.top().pageNumber) << endl;
                    tempPq.push(priorQueueFrame.top());
                    priorQueueFrame.pop();
                }
                while (!tempPq.empty()) {
                    priorQueueFrame.push(tempPq.top());
                    tempPq.pop();
                }
                cout << "replace\n";
            }
        } else {
            priority_queue<FrameContents> tempPq;
            cout << "hit\n";
            int temp = pageMap.at(*it) + 1;
            cout << "temp " << temp;
            pageMap[*it] = temp;
            //pageMap.at(*it) = pageMap.at(*it) + 1;
            while (!priorQueueFrame.empty()) {
                cerr << "PriorQueue contents: age: " << priorQueueFrame.top().age << " number: " <<
                        priorQueueFrame.top().pageNumber << "hash count: " <<
                        pageMap.at(priorQueueFrame.top().pageNumber) << endl;
                tempPq.push(priorQueueFrame.top());
                priorQueueFrame.pop();
            }
            while (!tempPq.empty()) {
                priorQueueFrame.push(tempPq.top());
                tempPq.pop();
            }
            for (auto it = pageMap.begin(); it != pageMap.end(); ++it)
                std::cout << " " << it->first << ":" << it->second;

            cout << "\nbreak1\n";
        }
        countPages++;
    }
    return replacements;
}

int lruStack(vector<int> pageVector, int frameNum) {
    stack<int> frameStack;
    int replacements = 0;

    for (vector<int>::iterator it = pageVector.begin(); it != pageVector.end(); ++it) {
        checkAndReplace(frameStack, frameNum, *it, replacements);
    }

    return replacements;
}

int optimal(vector<int> pageVector, int frameNum) {
    int replacementCount = 0, counter = 0, minCount = 10000;
    int available = frameNum;
    int frames[frameNum], distFromCurrent[frameNum], insertionOrder[frameNum];
    bool hit = false, miss = false;
    for (int i = 0; i < frameNum; i++) {
        frames[i] = -1;
    }
    int p = 0, maxDistIndex = -1, maxDist = 0;
    while (p < pageVector.size()) {
        minCount = ++counter;
        hit = miss = false;
        cout << "OPT: Page number : " << to_string(p) << " page value :" << to_string(pageVector[p]) << "  ==== ";
        for (int i = 0; i < frameNum; i++) {
            if (frames[i] == pageVector[p]) {
                cout << "OPT: it is a hit!" << endl;
                p++;
                hit = true;
                break;
            }
            if (frames[i] == -1) {
                miss = true;
                break;
            }
        }
        if (hit)
            continue;
        else
            miss = true;
        if (miss) {
            if (available > 0) {
                frames[frameNum - available] = pageVector[p];
                insertionOrder[frameNum - available] = counter;
                --available;
                ++p;
                //cout << "OPT: available frames : " << available << " p = " << p << endl;
            } else {
                replacementCount++;
                maxDist = 0;
                maxDistIndex = -1;
                for (int i = 0; i < frameNum; i++) {
                    distFromCurrent[i] = 10000;
                }
                for (int i = 0; i < frameNum; i++) {
                    for (int j = p + 1; j < pageVector.size(); j++) {
                        if (frames[i] == pageVector[j]) {
                            distFromCurrent[i] = j - p;
                            if (maxDist < distFromCurrent[i]) {
                                maxDist = distFromCurrent[i];
                                maxDistIndex = i;
                                break;
                            }
                        }
                    }
                }
                for (int i = 0; i < frameNum; i++) {
                    if (distFromCurrent[i] == 10000) {
                        if (minCount > insertionOrder[i]) {
                            maxDist = 10000;
                            maxDistIndex = i;
                            minCount = insertionOrder[i];
                        }
                    }
                }
                frames[maxDistIndex] = pageVector[p++];
                insertionOrder[maxDistIndex] = counter;
            }
        }
        cout << "frames -> ";
        for (int i = 0; i < frameNum; i++) {
            cout << " " << frames[i];
        }
        cout << endl;
    }
    return replacementCount;
}

int lruClock(vector<int> pageVector, int frameNum) {
    int replacementCount = 0, available = frameNum, p = 0, victimPointer = 0;
    int frames[frameNum], secChanceBit[frameNum];
    for (int i = 0; i < frameNum; i++) {
        frames[i] = -1;
        secChanceBit[i] = -1;
    }
    while (p < pageVector.size()) {
        bool hit = false, miss = false;
        for (int i = 0; i < frameNum; i++) {
            if (frames[i] == pageVector[p]) {
                p++;
                secChanceBit[i] = 1;
                hit = true;
                break;
            }
            if (frames[i] == -1) {
                miss = true;
                break;
            }
        }
        if (hit)
            continue;
        else
            miss = true;
        if (miss) {
            if (available > 0) {
                frames[frameNum - available] = pageVector[p];
                secChanceBit[frameNum - available] = 0;
                --available;
                ++p;
                victimPointer = incrementVictimPointer(victimPointer, frameNum);
            } else {
                replacementCount++;
                while (true) {
                    if (secChanceBit[victimPointer] == 0) {
                        frames[victimPointer] = pageVector[p];
                        secChanceBit[victimPointer] = 0;
                        break;
                    } else {
                        secChanceBit[victimPointer] = 0;
                    }
                    victimPointer = incrementVictimPointer(victimPointer, frameNum);
                }
            }
        }
        cout << "frames -> ";
        for (int i = 0; i < frameNum; i++) {
            cout << " " << frames[i];
        }
        cout << endl;
    }
    return replacementCount;
}

int lruRef8(vector<int> pageVector, int frameNum) {
    vector<pair<int, string>> bitmap;
    int p = 0, available = frameNum, replacementCount = 0, max = 0, maxIndex = -1;
    int frames[frameNum], bitmapLoc[frameNum];
    for (int i = 0; i < frameNum; i++) {
        frames[i] = -1;
        bitmapLoc[i] = -1;
    }
    string temp;
    while (p < pageVector.size()) {
        bool found = false, hit = false, miss = false;
        int page = pageVector[p++];

        cout << "frames (s)-> ";
        for (int i = 0; i < frameNum; i++) {
            cout << " " << frames[i];
        }
        cout << endl;
        for (int i = 0; i < bitmap.size(); i++) {
            if (get<0>(bitmap[i]) == page) {
                found = true;
                temp = get<1>(bitmap[i]);
                temp = "1" + temp.substr(0, temp.length() - 1);
            } else {
                temp = get<1>(bitmap[i]);
                temp = "0" + temp.substr(0, temp.length() - 1);
            }
            bitmap[i] = make_pair(get<0>(bitmap[i]), temp);
        }
        if (!found) {
            temp = "10000000";
            bitmap.push_back(make_pair(page, temp));
        }

        for (int i = 0; i < frameNum; i++) {
            if (frames[i] == page) {
                hit = true;
                break;
            }
            if (frames[i] == -1) {
                miss = true;
                break;
            }
        }
        if (hit)
            continue;
        else
            miss = true;
        if (miss) {
            if (available > 0) {
                frames[frameNum - available] = page;
                --available;
            } else {
                replacementCount++;
                for (int i = 0; i < frameNum; i++) {
                    for (int j = 0; j < bitmap.size(); j++) {
                        if (get<0>(bitmap[j]) == frames[i]) {
                            bitmapLoc[i] = string(get<1>(bitmap[j])).find("1");
                        }
                    }
                }
                max = 0;
                for (int i = 0; i < frameNum; i++) {
                    if (bitmapLoc[i] > max) {
                        max = bitmapLoc[i];
                        maxIndex = i;
                    }
                }

                frames[maxIndex ] = page;
            }

        }
        cout << "frames (e)-> ";
        for (int i = 0; i < frameNum; i++) {
            cout << " " << frames[i];
        }
        cout << endl;
    }
    return replacementCount;
}

int incrementVictimPointer(int currentVal, int frameNum) {
    if (currentVal + 1 == frameNum) {
        return 0;
    }
    return ++currentVal;
}

//checks for page in stack, if found, puts at top, if not, takes last out and puts replacement at top

bool checkAndReplace(stack<int>& frameStack, int frameNum, int page, int& replacements) {
    stack<int> tempStack;
    int pageCheck;
    bool pageFound = false;

    if (frameStack.size() < frameNum) {
        cout << "miss\n";
        ++replacements;
        if (checkDuplicate(frameStack, page)) {

        } else {
            frameStack.push(page);
        }
    } else {
        if (checkDuplicate(frameStack, page)) {
            cout << "hit\n";
        } else {
            ++replacements;
            cout << "miss\n";
            while (!frameStack.empty()) {
                tempStack.push(frameStack.top());
                frameStack.pop();
            }
            tempStack.pop();
            while (!tempStack.empty()) {
                frameStack.push(tempStack.top());
                tempStack.pop();
            }
            frameStack.push(page);
        }
    }
    return true;
}

bool checkDuplicate(stack<int>& frameStack, int page) {
    stack<int> tempStack;
    int checkPage;
    bool duplicate = false;

    while (!frameStack.empty()) {
        checkPage = frameStack.top();
        if (checkPage == page) {
            frameStack.pop();
            duplicate = true;
        }
        if (!frameStack.empty()) {
            tempStack.push(frameStack.top());
            frameStack.pop();
        }
    }

    while (!tempStack.empty()) {
        frameStack.push(tempStack.top());
        tempStack.pop();
    }
    if (duplicate)
        frameStack.push(page);
    return duplicate;

}
/*
//Below updates the bit string for the ref8 algo. 

vector<pair<int, string>> lru_ref8(stack<int>& frameStack, int frameNum) {
    vector<pair<int, string>> bit_Vstring;
    cout << "String size for bit_string " << bit_Vstring.size() << endl;
    string s;
    b
    for (unsigned int i = 0; frameStack.size() > 0; i++) {
        int temp = frameStack.top();
        /*if (checkDuplicate(frameStack, temp)) { //checking if the page already exists in the string vector for pages. 
            s.insert(1, (const char*) '1');
        } else {
            for (int i = 0; i < 7; i++) {
                s.push_back('0');
            }
            s.insert(1, (const char*) '1');
        }
        bit_Vstring.resize(8);
        bit_Vstring.push_back(make_pair(temp, s));
        frameStack.pop();
    }
    return bit_Vstring;
}
 */